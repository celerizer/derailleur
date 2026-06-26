#include "DrNetplay.h"

#include <QDataStream>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

#include <QRetro.h>
#include <libretro.h>

#include "DrCommon.h"
#include "DrInputStore.h"
#include "QRetroInputBackendShared.h"

// Wire message types: [1-byte type][fixed-size payload].
static constexpr quint8 k_MsgHandshake = 0x01;  // server -> client: { peerIndex, peerCount }
static constexpr quint8 k_MsgStart = 0x02;      // server -> client: begin lockstep
static constexpr quint8 k_MsgInput = 0x03;      // either direction: DrNetplayPacket
static constexpr quint8 k_MsgCandidates = 0x04; // server -> client: 5 (guest, minigame) pairs

// quint64 + quint8 + quint8 + quint16 + 6 * qint16
static constexpr int k_PacketPayloadSize = 8 + 1 + 1 + 2 + 6 * 2;

// 5 candidate slots, each a pair of quint16 (guest index, minigame index).
static constexpr int k_CandidateSlots = 5;
static constexpr int k_CandidatesPayloadSize = k_CandidateSlots * 2 * 2;

DrNetplay::DrNetplay(DrInputStore *store, QObject *parent)
  : QObject(parent)
  , m_Store(store)
{
}

/* ── Session setup ─────────────────────────────────────────────── */

void DrNetplay::hostSession(quint16 port, int playerCount)
{
  m_IsServer = true;
  m_PeerIndex = 0;
  m_PeerCount = qBound(1, playerCount, k_MaxPeers);
  m_NextAssignIndex = 1;

  m_Server = new QTcpServer(this);
  connect(m_Server, &QTcpServer::newConnection, this, &DrNetplay::onNewConnection);
  if (!m_Server->listen(QHostAddress::Any, port))
  {
    dropSession(QString("netplay: failed to bind port %1: %2").arg(port).arg(
      m_Server->errorString()));
    return;
  }

  /* The session does not begin on connection; it begins when the host selects
   * a game via startGame(). */
  emit peerCountChanged(1, m_PeerCount);
}

void DrNetplay::startGame(int gameId)
{
  if (!m_IsServer)
    return;

  QByteArray payload;
  payload.append(static_cast<char>(gameId));
  broadcast(k_MsgStart, payload);

  m_Active = true;
  resetFrameCounter();
  emit logMessage(DR_LOG_INFO,
    QString("netplay: server starting game %1, peers=%2, sockets=%3")
      .arg(gameId)
      .arg(m_PeerCount)
      .arg(m_Sockets.size()));
  emit sessionStarted(m_PeerIndex, m_PeerCount);
}

void DrNetplay::sendCandidates(const QList<QPair<int, int>> &candidates)
{
  if (!m_IsServer)
    return;

  QByteArray payload;
  QDataStream s(&payload, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  for (int i = 0; i < k_CandidateSlots; i++)
  {
    quint16 guest = 0xFFFF, minigame = 0xFFFF;
    if (i < candidates.size() && candidates[i].first >= 0 && candidates[i].second >= 0)
    {
      guest = static_cast<quint16>(candidates[i].first);
      minigame = static_cast<quint16>(candidates[i].second);
    }
    s << guest << minigame;
  }
  broadcast(k_MsgCandidates, payload);
}

void DrNetplay::setActiveContext(QRetro *core)
{
  const int ctx = m_ContextIds.value(core, -1);
  m_ActiveContext = ctx;
  if (!m_Active || ctx < 0)
    return;

  /* Becoming the foreground context is this context's per-context sync point.
   * The frame counter is monotonic and only advances on gated frames, so it is
   * already equal across peers — we do NOT reset it (that would reuse frame
   * numbers) and we do NOT clear m_Received (that would wipe a peer's prime that
   * already arrived). We just mark the current frame as a barrier, which is
   * allowed to wait indefinitely for the other peer's warmup/load, and re-prime
   * the input-delay pipeline from here. */
  {
    QMutexLocker lock(&m_RecvMutex);
    /* Re-activating a frozen context (e.g. returning to the host after a
     * minigame) unfreezes it; switching to a different context leaves the old
     * one frozen so its core stays put until we come back. */
    if (m_FrozenContext == ctx)
      m_FrozenContext = -1;
    m_CtxBarrier[ctx] = m_CtxFrame[ctx];
    m_FrameReady.wakeAll();
  }
  primeContext(ctx);
  emit logMessage(DR_LOG_INFO,
    QString("netplay: active context = %1 (barrier frame %2)").arg(ctx).arg(m_CtxFrame[ctx]));
}

void DrNetplay::freezeActiveContext()
{
  QMutexLocker lock(&m_RecvMutex);
  m_FrozenContext = m_ActiveContext;
  m_FrameReady.wakeAll();
}

void DrNetplay::joinSession(const QString &address, quint16 port)
{
  m_IsServer = false;

  auto *sock = new QTcpSocket(this);
  m_Sockets.append(sock);
  connect(sock, &QTcpSocket::readyRead, this, &DrNetplay::onSocketReadyRead);
  connect(sock, &QTcpSocket::disconnected, this, &DrNetplay::onSocketDisconnected);
  connect(sock, &QAbstractSocket::errorOccurred, this,
    [this, sock](QAbstractSocket::SocketError) { dropSession(sock->errorString()); });
  sock->connectToHost(address, port);
}

void DrNetplay::resetFrameCounter()
{
  m_LocalFrame = 0;
  m_ActiveContext = -1;
  QMutexLocker lock(&m_RecvMutex);
  for (int i = 0; i < k_MaxContexts; i++)
  {
    m_CtxFrame[i] = 0;
    m_CtxBarrier[i] = 0;
  }
  m_Received.clear();
  m_FrameReady.wakeAll();
}

void DrNetplay::setLocalSource(QRetroInputBackend *backend)
{
  m_LocalSource = backend;
  if (m_LocalSource)
    m_LocalSource->init(m_LocalPads, DrInputStore::k_MaxPorts);
}

void DrNetplay::attachCore(QRetro *core)
{
  if (!core || m_ContextIds.contains(core) || m_ContextCount >= k_MaxContexts)
    return;

  const int ctx = m_ContextCount++;
  m_ContextIds.insert(core, ctx);

  auto *backend = new QRetroInputBackendShared(m_Store, core);
  backend->init(core->input()->joypads(), core->input()->maxUsers());
  core->input()->setBackend(backend);

  connect(
    core, &QRetro::frameBegin, this, [this, ctx]() { onFrameBegin(ctx); }, Qt::DirectConnection);
}

/* ── Frame coordination (timing thread) ────────────────────────── */

void DrNetplay::onFrameBegin(int context)
{
  if (!m_Active)
  {
    /* Singleplayer passthrough: sample all local controllers and commit. */
    sampleLocal();
    m_Store->commitFrame(m_LocalFrame++, m_LocalPads);
    return;
  }

  /* Freeze: hold this context here (before retro_run) until it is re-activated.
   * Triggered deterministically from the lockstep timeline (minigameRequested),
   * so every peer stops the host on the exact same frame rather than running a
   * different number of host frames before the async launch pauses it. */
  {
    QMutexLocker lock(&m_RecvMutex);
    while (context == m_FrozenContext && m_Active && !m_Abort)
      m_FrameReady.wait(&m_RecvMutex, 100);
    if (!m_Active || m_Abort)
      return;
  }

  /* During a session only the foreground context is gated; other (paused or
   * preloading) contexts simply leave the store as-is. */
  if (context != m_ActiveContext)
    return;

  const quint64 frame = m_CtxFrame[context];

  if ((m_DebugTick++ % 120) == 0)
    emit logMessage(DR_LOG_INFO,
      QString("netplay tick: ctx=%1 peer=%2/%3 frame=%4 store=%5")
        .arg(context)
        .arg(m_PeerIndex)
        .arg(m_PeerCount)
        .arg(frame)
        .arg(m_Store->currentFrame()));

  /* Sample this peer's controller and send it for frame + delay. The delay
   * gives the network m_InputDelay frames of slack before any stall. */
  sampleLocal();
  DrNetplayPacket mine =
    packetFromJoypad(m_LocalPads[0], m_PeerIndex, context, frame + m_InputDelay);
  recordPacket(mine);
  sendInput(mine);

  if (!waitForFrame(context, frame))
  {
    const QString err =
      QString("netplay: timed out waiting for ctx %1 frame %2").arg(context).arg(frame);
    QMetaObject::invokeMethod(this, [this, err]() { dropSession(err); }, Qt::QueuedConnection);
    return;
  }

  commitMergedFrame(context, frame);
  m_CtxFrame[context] = frame + 1;
}

void DrNetplay::sampleLocal()
{
  if (m_LocalSource)
    m_LocalSource->poll();
  /* Finalize each joypad's bitmask from its digital buttons. */
  for (unsigned i = 0; i < DrInputStore::k_MaxPorts; i++)
    m_LocalPads[i].poll();
}

bool DrNetplay::isFrameCompleteLocked(int context, quint64 frame) const
{
  auto it = m_Received.constFind(frameKey(context, frame));
  if (it == m_Received.constEnd())
    return false;
  for (int i = 0; i < m_PeerCount; i++)
    if (!it->have[i])
      return false;
  return true;
}

bool DrNetplay::waitForFrame(int context, quint64 frame)
{
  QElapsedTimer timer;
  timer.start();

  /* The barrier frame — the first frame after a sync point (warmup done, state
   * load, returning to the host) — may legitimately block for a long time while
   * the other peer finishes its own warmup/load. Don't time out there; just
   * pause until it arrives (or the socket actually drops, which is detected
   * separately). Subsequent frames use the normal timeout so a genuinely hung
   * peer is still caught. */
  const bool barrier = (frame == m_CtxBarrier[context]);

  /* Block the emulation (timing) thread on a wait condition until the frame's
   * input arrives. This genuinely pauses the core — no busy-spin — and the main
   * thread wakes us as soon as a packet is recorded. */
  QMutexLocker lock(&m_RecvMutex);
  while (!isFrameCompleteLocked(context, frame))
  {
    if (!m_Active || m_Abort)
      return false;
    if (!barrier && timer.hasExpired(m_TimeoutMs))
      return false;
    /* Re-check at least every 100ms so a session drop is noticed promptly. */
    m_FrameReady.wait(&m_RecvMutex, 100);
  }
  if (timer.elapsed() > 4)
    emit logMessage(DR_LOG_INFO, QString("netplay: stalled %1ms on ctx %2 frame %3")
                                   .arg(timer.elapsed())
                                   .arg(context)
                                   .arg(frame));
  return true;
}

void DrNetplay::commitMergedFrame(int context, quint64 frame)
{
  FrameInputs fi;
  {
    QMutexLocker lock(&m_RecvMutex);
    fi = m_Received.value(frameKey(context, frame));
    m_Received.remove(frameKey(context, frame));
  }

  for (int i = 0; i < k_MaxPeers; i++)
  {
    DrNetplayPacket p = fi.pkts[i];
    if (i >= m_PeerCount || !fi.have[i])
      p = DrNetplayPacket{ frame, static_cast<uint8_t>(i), static_cast<uint8_t>(context), 0, 0, 0,
        0, 0, 0, 0 };
    applyPacketToJoypad(p, m_CommitPads[i]);
  }

  m_Store->commitFrame(frame, m_CommitPads);
}

/* ── Networking (main thread) ──────────────────────────────────── */

void DrNetplay::onNewConnection()
{
  while (m_Server->hasPendingConnections())
  {
    QTcpSocket *sock = m_Server->nextPendingConnection();

    if (m_Active || m_NextAssignIndex >= m_PeerCount)
    {
      sock->disconnectFromHost();
      sock->deleteLater();
      continue;
    }

    const int index = m_NextAssignIndex++;
    m_Sockets.append(sock);
    connect(sock, &QTcpSocket::readyRead, this, &DrNetplay::onSocketReadyRead);
    connect(sock, &QTcpSocket::disconnected, this, &DrNetplay::onSocketDisconnected);

    QByteArray payload;
    payload.append(static_cast<char>(index));
    payload.append(static_cast<char>(m_PeerCount));
    writeMessage(sock, k_MsgHandshake, payload);

    emit peerCountChanged(m_Sockets.size() + 1, m_PeerCount);
  }
}

void DrNetplay::onSocketReadyRead()
{
  auto *sock = qobject_cast<QTcpSocket *>(sender());
  if (!sock)
    return;

  QByteArray &buf = m_RxBuf[sock];
  buf.append(sock->readAll());

  for (;;)
  {
    if (buf.isEmpty())
      break;
    const quint8 type = static_cast<quint8>(buf.at(0));
    const int len = payloadLength(type);
    if (len < 0)
    {
      dropSession(QString("netplay: bad message type 0x%1").arg(type, 2, 16, QChar('0')));
      return;
    }
    if (buf.size() < 1 + len)
      break;

    const QByteArray payload = buf.mid(1, len);
    buf.remove(0, 1 + len);
    handleMessage(sock, type, payload);
  }
}

void DrNetplay::handleMessage(QTcpSocket *sock, quint8 type, const QByteArray &payload)
{
  switch (type)
  {
  case k_MsgHandshake:
    if (payload.size() >= 2)
    {
      m_PeerIndex = static_cast<quint8>(payload.at(0));
      m_PeerCount = static_cast<quint8>(payload.at(1));
      emit logMessage(DR_LOG_INFO,
        QString("netplay: client assigned peer %1 of %2").arg(m_PeerIndex).arg(m_PeerCount));
    }
    break;

  case k_MsgStart:
  {
    const int gameId = payload.isEmpty() ? 0 : static_cast<quint8>(payload.at(0));
    m_Active = true;
    resetFrameCounter();
    emit logMessage(DR_LOG_INFO,
      QString("netplay: client received start, game %1, peers=%2").arg(gameId).arg(m_PeerCount));
    emit sessionStarted(m_PeerIndex, m_PeerCount);
    emit startGameRequested(gameId);
    break;
  }

  case k_MsgInput:
  {
    const DrNetplayPacket p = decodePacket(payload);
    recordPacket(p);
    /* The server relays each peer's input to all other peers. */
    if (m_IsServer)
      broadcast(k_MsgInput, payload, sock);
    break;
  }

  case k_MsgCandidates:
  {
    QList<QPair<int, int>> candidates;
    QDataStream s(payload);
    s.setByteOrder(QDataStream::LittleEndian);
    for (int i = 0; i < k_CandidateSlots; i++)
    {
      quint16 guest = 0xFFFF, minigame = 0xFFFF;
      s >> guest >> minigame;
      candidates.append({ guest == 0xFFFF ? -1 : static_cast<int>(guest),
        minigame == 0xFFFF ? -1 : static_cast<int>(minigame) });
    }
    emit candidatesReceived(candidates);
    break;
  }

  default:
    break;
  }
}

void DrNetplay::onSocketDisconnected()
{
  if (m_Active)
  {
    dropSession("netplay: peer disconnected");
    return;
  }

  /* Pre-start: a client dropped while still in the lobby. */
  if (auto *sock = qobject_cast<QTcpSocket *>(sender()))
  {
    m_Sockets.removeAll(sock);
    m_RxBuf.remove(sock);
    sock->deleteLater();
    if (m_IsServer && m_NextAssignIndex > 1)
      m_NextAssignIndex--;
    emit peerCountChanged(m_Sockets.size() + 1, m_PeerCount);
  }
}

void DrNetplay::primeContext(int context)
{
  /* Prime this peer's own input for the delay frames starting at the context's
   * current (barrier) frame, so every peer can complete those frames before
   * real input arrives after a sync point. */
  const quint64 base = m_CtxFrame[context];
  for (int f = 0; f < m_InputDelay; f++)
  {
    DrNetplayPacket p{};
    p.frame = base + static_cast<quint64>(f);
    p.peerIndex = static_cast<uint8_t>(m_PeerIndex);
    p.context = static_cast<uint8_t>(context);
    recordPacket(p);
    sendInput(p);
  }
}

void DrNetplay::recordPacket(const DrNetplayPacket &p)
{
  if (p.peerIndex >= k_MaxPeers)
    return;
  QMutexLocker lock(&m_RecvMutex);
  FrameInputs &fi = m_Received[frameKey(p.context, p.frame)];
  fi.pkts[p.peerIndex] = p;
  fi.have[p.peerIndex] = true;
  m_FrameReady.wakeAll();
}

void DrNetplay::sendInput(const DrNetplayPacket &p)
{
  const QByteArray payload = encodePacket(p);
  /* Marshal to the main thread, where the sockets live. */
  QMetaObject::invokeMethod(
    this,
    [this, payload]() {
      if (m_IsServer)
        broadcast(k_MsgInput, payload);
      else if (!m_Sockets.isEmpty())
        writeMessage(m_Sockets.first(), k_MsgInput, payload);
    },
    Qt::QueuedConnection);
}

void DrNetplay::writeMessage(QTcpSocket *sock, quint8 type, const QByteArray &payload)
{
  if (!sock)
    return;
  QByteArray msg;
  msg.append(static_cast<char>(type));
  msg.append(payload);
  sock->write(msg);
}

void DrNetplay::broadcast(quint8 type, const QByteArray &payload, QTcpSocket *except)
{
  for (QTcpSocket *sock : m_Sockets)
    if (sock != except)
      writeMessage(sock, type, payload);
}

void DrNetplay::dropSession(const QString &reason)
{
  if (!m_Active && !m_Server && m_Sockets.isEmpty())
    return;

  m_Active = false;

  for (QTcpSocket *sock : m_Sockets)
  {
    sock->disconnect(this);
    sock->abort();
    sock->deleteLater();
  }
  m_Sockets.clear();
  m_RxBuf.clear();

  if (m_Server)
  {
    m_Server->close();
    m_Server->deleteLater();
    m_Server = nullptr;
  }

  {
    QMutexLocker lock(&m_RecvMutex);
    m_Received.clear();
    /* Wake any timing thread parked in waitForFrame so it exits promptly. */
    m_FrameReady.wakeAll();
  }

  emit sessionError(reason);
}

void DrNetplay::abort()
{
  QMutexLocker lock(&m_RecvMutex);
  m_Abort = true;
  m_FrameReady.wakeAll();
}

/* ── Packet (de)serialization and joypad mapping ───────────────── */

quint64 DrNetplay::frameKey(int context, quint64 frame)
{
  return (static_cast<quint64>(context) << 48) | (frame & 0xFFFFFFFFFFFFull);
}

int DrNetplay::payloadLength(quint8 type)
{
  switch (type)
  {
  case k_MsgHandshake:
    return 2;
  case k_MsgStart:
    return 1;
  case k_MsgInput:
    return k_PacketPayloadSize;
  case k_MsgCandidates:
    return k_CandidatesPayloadSize;
  default:
    return -1;
  }
}

QByteArray DrNetplay::encodePacket(const DrNetplayPacket &p)
{
  QByteArray b;
  QDataStream s(&b, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  s << static_cast<quint64>(p.frame) << static_cast<quint8>(p.peerIndex)
    << static_cast<quint8>(p.context) << static_cast<quint16>(p.bitmask)
    << static_cast<qint16>(p.leftX) << static_cast<qint16>(p.leftY) << static_cast<qint16>(p.rightX)
    << static_cast<qint16>(p.rightY) << static_cast<qint16>(p.l2) << static_cast<qint16>(p.r2);
  return b;
}

DrNetplayPacket DrNetplay::decodePacket(const QByteArray &b)
{
  DrNetplayPacket p{};
  QDataStream s(b);
  s.setByteOrder(QDataStream::LittleEndian);
  quint64 frame = 0;
  quint8 peerIndex = 0;
  quint8 context = 0;
  quint16 bitmask = 0;
  qint16 lx = 0, ly = 0, rx = 0, ry = 0, l2 = 0, r2 = 0;
  s >> frame >> peerIndex >> context >> bitmask >> lx >> ly >> rx >> ry >> l2 >> r2;
  p.frame = frame;
  p.peerIndex = peerIndex;
  p.context = context;
  p.bitmask = bitmask;
  p.leftX = lx;
  p.leftY = ly;
  p.rightX = rx;
  p.rightY = ry;
  p.l2 = l2;
  p.r2 = r2;
  return p;
}

DrNetplayPacket DrNetplay::packetFromJoypad(
  QRetroInputJoypad &jp, int peerIndex, int context, quint64 frame)
{
  DrNetplayPacket p{};
  p.frame = frame;
  p.peerIndex = static_cast<uint8_t>(peerIndex);
  p.context = static_cast<uint8_t>(context);
  p.bitmask = static_cast<uint16_t>(jp.bitmask());
  p.leftX = jp.analogStick(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X);
  p.leftY = jp.analogStick(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y);
  p.rightX = jp.analogStick(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
  p.rightY = jp.analogStick(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);
  p.l2 = jp.analogButton(RETRO_DEVICE_ID_JOYPAD_L2);
  p.r2 = jp.analogButton(RETRO_DEVICE_ID_JOYPAD_R2);
  return p;
}

void DrNetplay::applyPacketToJoypad(const DrNetplayPacket &p, QRetroInputJoypad &jp)
{
  for (unsigned id = 0; id <= RETRO_DEVICE_ID_JOYPAD_R3; id++)
    jp.setDigitalButton(id, (p.bitmask >> id) & 1);
  jp.setAnalogStick(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, p.leftX);
  jp.setAnalogStick(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, p.leftY);
  jp.setAnalogStick(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X, p.rightX);
  jp.setAnalogStick(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y, p.rightY);
  jp.setAnalogButton(RETRO_DEVICE_ID_JOYPAD_L2, p.l2);
  jp.setAnalogButton(RETRO_DEVICE_ID_JOYPAD_R2, p.r2);
}
