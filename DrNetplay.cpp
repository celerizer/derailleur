#include "DrNetplay.h"

#include <QDataStream>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QTcpServer>
#include <QTcpSocket>

#include <QRetro.h>
#include <libretro.h>

#include "DrCommon.h"
#include "DrInputStore.h"
#include "QRetroInputBackendShared.h"

#ifndef DR_GIT_HASH
#define DR_GIT_HASH unknown
#endif
#define DR_STRINGIZE_(x) #x
#define DR_STRINGIZE(x) DR_STRINGIZE_(x)
#define DR_NETPLAY_BUILD_HASH DR_STRINGIZE(DR_GIT_HASH)

DrNetplay::DrNetplay(DrInputStore *store, QObject *parent)
  : QObject(parent)
  , m_Store(store)
{
}

void DrNetplay::hostSession(quint16 port, int playerCount)
{
  m_IsServer = true;
  m_PeerIndex = 0;
  m_PeerCount = qBound(1, playerCount, DR_NETPLAY_MAX_PEERS);
  m_NextAssignIndex = 1;

  m_Server = new QTcpServer(this);
  connect(m_Server, &QTcpServer::newConnection, this, &DrNetplay::onNewConnection);
  if (!m_Server->listen(QHostAddress::Any, port))
  {
    dropSession(QString("netplay: failed to bind port %1: %2").arg(port).arg(
      m_Server->errorString()));
    return;
  }

  emit peerCountChanged(1, m_PeerCount);
}

void DrNetplay::startGame(int gameId)
{
  if (!m_IsServer)
    return;

  /* Seed the shared PRNG so every peer rolls identical minigame candidates, and
   * ship the seed alongside the game so clients seed the same. */
  const quint32 seed = QRandomGenerator::global()->generate();
  dr_srand(seed);

  QByteArray payload;
  payload.append(static_cast<char>(gameId));
  {
    QDataStream s(&payload, QIODevice::WriteOnly | QIODevice::Append);
    s.setByteOrder(QDataStream::LittleEndian);
    s << seed;
  }
  broadcast(DR_NETPLAY_PACKET_START, payload);

  /* The server's buffer setting is authoritative at session start. */
  QByteArray delayPayload;
  delayPayload.append(static_cast<char>(m_InputDelay.load()));
  broadcast(DR_NETPLAY_PACKET_SET_DELAY, delayPayload);

  m_Active = true;
  resetFrameCounter();
  emit logMessage(DR_LOG_INFO,
    QString("netplay: server starting game %1, peers=%2, sockets=%3")
      .arg(gameId)
      .arg(m_PeerCount)
      .arg(m_Sockets.size()));
  emit sessionStarted(m_PeerIndex, m_PeerCount);
}

void DrNetplay::changeInputDelay(int frames)
{
  frames = qBound(0, frames, 30);
  if (frames == m_InputDelay.load())
    return;
  m_InputDelay = frames;
  emit inputDelayChanged(frames);

  /* Outside a session this is just a local default. During one, push it to the
   * peers; each context's send loop back-fills the gap so nobody stalls. */
  if (!m_Active)
    return;

  QByteArray payload;
  payload.append(static_cast<char>(frames));
  if (m_IsServer)
    broadcast(DR_NETPLAY_PACKET_SET_DELAY, payload);
  else if (!m_Sockets.isEmpty())
    writeMessage(m_Sockets.first(), DR_NETPLAY_PACKET_SET_DELAY, payload);
}

void DrNetplay::requestHardResync()
{
  if (!m_IsServer || !m_Active || m_ActiveContext < 0)
    return;
  if (m_ResyncActive.load())
    return;

  const int ctx = m_ActiveContext;

  /* Tell the clients to stop and await state, then arm ourselves. The timing
   * thread picks this up (interrupting any stall) and does the serialize. */
  QByteArray payload;
  payload.append(static_cast<char>(ctx));
  broadcast(DR_NETPLAY_PACKET_RESYNC_BEGIN, payload);

  m_ResyncCtx = ctx;
  {
    QMutexLocker lock(&m_RecvMutex);
    m_ResyncStateReady = false;
    m_ResyncState.clear();
    m_Received.clear();
    m_ResyncActive = true;
    m_FrameReady.wakeAll();
  }
  emit logMessage(DR_LOG_INFO, QString("netplay: hard resync requested for ctx %1").arg(ctx));
}

void DrNetplay::sendCandidates(const QList<QPair<int, int>> &candidates)
{
  if (!m_IsServer)
    return;

  QByteArray payload;
  QDataStream s(&payload, QIODevice::WriteOnly);
  s.setByteOrder(QDataStream::LittleEndian);
  for (int i = 0; i < DR_NETPLAY_CANDIDATE_SLOTS; i++)
  {
    quint16 guest = 0xFFFF, minigame = 0xFFFF;
    if (i < candidates.size() && candidates[i].first >= 0 && candidates[i].second >= 0)
    {
      guest = static_cast<quint16>(candidates[i].first);
      minigame = static_cast<quint16>(candidates[i].second);
    }
    s << guest << minigame;
  }
  broadcast(DR_NETPLAY_PACKET_CANDIDATES, payload);
}

void DrNetplay::setActiveContext(QRetro *core)
{
  const int ctx = m_ContextIds.value(core, -1);
  if (!m_Active || ctx < 0)
  {
    m_ActiveContext = ctx;
    return;
  }

  /* Becoming the foreground context is this context's per-context sync point.
   * The frame counter is monotonic and only advances on gated frames, so it is
   * already equal across peers — we do NOT reset it (that would reuse frame
   * numbers) and we do NOT clear m_Received (that would wipe a peer's prime that
   * already arrived). We just mark the current frame as a barrier, which is
   * allowed to wait indefinitely for the other peer's warmup/load, and re-prime
   * the input-delay pipeline from here. */
  {
    QMutexLocker lock(&m_RecvMutex);
    m_CtxBarrier[ctx] = m_CtxFrame[ctx];
  }
  /* Prime fills m_CtxSend before we publish this context as active, so the
   * timing thread never runs the send loop with a stale pipeline cursor. */
  primeContext(ctx);
  {
    /* Unfreeze and publish the new foreground in one atomic step, then wake
     * everyone. This ordering matters: a context parked in the freeze loop must
     * not wake until it is *already* the active context. If we unfroze first and
     * published second (with priming's network marshaling in between), the
     * freshly-unfrozen thread would exit the freeze loop while m_ActiveContext
     * still pointed at the outgoing context, fall through the "switched away"
     * early-return in onFrameBegin, and skip gating its barrier frame — running
     * one ungated frame and desyncing by one. Switching to a different context
     * leaves the old one frozen so its core stays put until we come back. */
    QMutexLocker lock(&m_RecvMutex);
    if (m_FrozenContext == ctx)
      m_FrozenContext = -1;
    m_ActiveContext = ctx;
    m_FrameReady.wakeAll();
  }
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
  for (int i = 0; i < DR_NETPLAY_MAX_CONTEXTS; i++)
  {
    m_CtxFrame[i] = 0;
    m_CtxBarrier[i] = 0;
    m_CtxSend[i] = 0;
  }
  m_Received.clear();
  m_ResyncActive = false;
  m_ResyncStateReady = false;
  m_ResyncState.clear();
  m_ResyncCtx = -1;
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
  if (!core || m_ContextIds.contains(core) || m_ContextCount >= DR_NETPLAY_MAX_CONTEXTS)
    return;

  const int ctx = m_ContextCount++;
  m_ContextIds.insert(core, ctx);
  m_Contexts[ctx] = core;

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

  /* Stay inside onFrameBegin (holding off retro_run) until a frame is gated or
   * the session ends. The loop lets a hard resync, which can interrupt a stalled
   * waitForFrame, be serviced here on the timing thread before the core advances. */
  for (;;)
  {
    /* Hard resync: the core is quiescent here (between frames), so serialize /
     * unserialize run inline. runResync replaces this context's state from the
     * host and re-arms the frame counter + barrier before we gate again. */
    if (m_ResyncActive.load() && context == m_ResyncCtx)
      runResync(context);
    if (!m_Active || m_Abort)
      return;

    const quint64 frame = m_CtxFrame[context];

    /* Sample this peer's controller and send it for every frame from the send
     * cursor up to frame + delay. Normally that is exactly one frame; when the
     * delay was just increased it back-fills the newly opened gap so no peer
     * stalls, and when decreased it sends nothing until the buffered frames
     * drain. The delay therefore gives the network `delay` frames of slack and
     * can be retuned live. */
    sampleLocal();
    const quint64 target = frame + static_cast<quint64>(m_InputDelay.load());
    while (m_CtxSend[context] <= target)
    {
      DrNetplayPacket mine =
        packetFromJoypad(m_LocalPads[0], m_PeerIndex, context, m_CtxSend[context]);
      recordPacket(mine);
      sendInput(mine);
      m_CtxSend[context]++;
    }

    if (waitForFrame(context, frame))
    {
      commitMergedFrame(context, frame);
      m_CtxFrame[context] = frame + 1;
      return;
    }

    /* waitForFrame bailed. If a resync was just requested it interrupted the
     * wait deliberately — loop to service it without running retro_run. */
    if (m_ResyncActive.load())
      continue;

    /* The foreground switched away from this context — stop gating it, don't
     * treat it as a timeout. */
    if (context != m_ActiveContext)
      return;

    const QString err =
      QString("netplay: timed out waiting for ctx %1 frame %2").arg(context).arg(frame);
    QMetaObject::invokeMethod(this, [this, err]() { dropSession(err); }, Qt::QueuedConnection);
    return;
  }
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

  /**
   * For the first frame after a context switch, don't time out as this may
   * wait for a long time depending on peer hardware.
   */
  const bool barrier = (frame == m_CtxBarrier[context]);

  /* Block the timing thread on a wait condition until the frame's input arrives */
  QMutexLocker lock(&m_RecvMutex);
  while (!isFrameCompleteLocked(context, frame))
  {
    if (!m_Active || m_Abort)
      return false;
    /* A hard resync interrupts the wait so onFrameBegin can reload state. */
    if (m_ResyncActive.load() && context == m_ResyncCtx)
      return false;
    /* The foreground switched away (e.g. a minigame ended and we returned to the
     * host). This context is no longer gated, so stop waiting for input that will
     * never arrive instead of timing out and dropping the session. */
    if (context != m_ActiveContext)
      return false;
    if (!barrier && timer.hasExpired(m_TimeoutMs))
      return false;
    /* Re-check at least every 100ms so a session drop is noticed promptly. */
    m_FrameReady.wait(&m_RecvMutex, 100);
  }
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

  for (int i = 0; i < DR_NETPLAY_MAX_PEERS; i++)
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
    writeMessage(sock, DR_NETPLAY_PACKET_HANDSHAKE, payload);

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

    /* The resync state is the one variable-length message: [type][u32 len][..]. */
    if (type == DR_NETPLAY_PACKET_RESYNC_STATE)
    {
      if (buf.size() < 1 + 4)
        break;
      quint32 len = 0;
      {
        QDataStream s(buf.mid(1, 4));
        s.setByteOrder(QDataStream::LittleEndian);
        s >> len;
      }
      if (buf.size() < static_cast<int>(1 + 4 + len))
        break;
      const QByteArray payload = buf.mid(5, static_cast<int>(len));
      buf.remove(0, static_cast<int>(5 + len));
      handleMessage(sock, type, payload);
      continue;
    }

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
  case DR_NETPLAY_PACKET_HANDSHAKE:
    if (payload.size() >= 2)
    {
      m_PeerIndex = static_cast<quint8>(payload.at(0));
      m_PeerCount = static_cast<quint8>(payload.at(1));
      emit logMessage(DR_LOG_INFO,
        QString("netplay: client assigned peer %1 of %2").arg(m_PeerIndex).arg(m_PeerCount));

      /* Report our build so the server can warn about a version mismatch. The
       * hash is fixed-width and null-padded to match the wire framing. */
      const QByteArray v =
        QByteArray(DR_NETPLAY_BUILD_HASH).leftJustified(DR_NETPLAY_VERSION_HASH_LEN, '\0', true);
      writeMessage(sock, DR_NETPLAY_PACKET_VERSION, v);
    }
    break;

  case DR_NETPLAY_PACKET_VERSION:
  {
    const QByteArray theirs(payload.constData()); // null-padded -> trims to the hash
    const QByteArray ours(DR_NETPLAY_BUILD_HASH);
    if (theirs != ours)
      emit logMessage(DR_LOG_WARN,
        QString("netplay: peer build %1 does not match this build %2 — desyncs are likely")
          .arg(QString::fromLatin1(theirs.constData()))
          .arg(QString::fromLatin1(ours.constData())));
    else
      emit logMessage(DR_LOG_INFO,
        QString("netplay: peer build %1 verified").arg(QString::fromLatin1(theirs.constData())));
    break;
  }

  case DR_NETPLAY_PACKET_START:
  {
    const int gameId = payload.isEmpty() ? 0 : static_cast<quint8>(payload.at(0));
    /* Seed the shared PRNG with the server's seed so candidate rolls match. */
    quint32 seed = 0;
    if (payload.size() >= 5)
    {
      QDataStream s(payload.mid(1, 4));
      s.setByteOrder(QDataStream::LittleEndian);
      s >> seed;
    }
    dr_srand(seed);
    m_Active = true;
    resetFrameCounter();
    emit logMessage(DR_LOG_INFO,
      QString("netplay: client received start, game %1, peers=%2").arg(gameId).arg(m_PeerCount));
    emit sessionStarted(m_PeerIndex, m_PeerCount);
    emit startGameRequested(gameId);
    break;
  }

  case DR_NETPLAY_PACKET_INPUT:
  {
    const DrNetplayPacket p = decodePacket(payload);
    recordPacket(p);
    /* The server relays each peer's input to all other peers. */
    if (m_IsServer)
      broadcast(DR_NETPLAY_PACKET_INPUT, payload, sock);
    break;
  }

  case DR_NETPLAY_PACKET_CANDIDATES:
  {
    QList<QPair<int, int>> candidates;
    QDataStream s(payload);
    s.setByteOrder(QDataStream::LittleEndian);
    for (int i = 0; i < DR_NETPLAY_CANDIDATE_SLOTS; i++)
    {
      quint16 guest = 0xFFFF, minigame = 0xFFFF;
      s >> guest >> minigame;
      candidates.append({ guest == 0xFFFF ? -1 : static_cast<int>(guest),
        minigame == 0xFFFF ? -1 : static_cast<int>(minigame) });
    }
    emit candidatesReceived(candidates);
    break;
  }

  case DR_NETPLAY_PACKET_SET_DELAY:
  {
    const int d = payload.isEmpty() ? m_InputDelay.load() : static_cast<quint8>(payload.at(0));
    m_InputDelay = d;
    emit inputDelayChanged(d);
    /* The server relays the change on to the other clients. */
    if (m_IsServer)
      broadcast(DR_NETPLAY_PACKET_SET_DELAY, payload, sock);
    break;
  }

  case DR_NETPLAY_PACKET_RESYNC_BEGIN:
  {
    /* Pause the current context and await serialized data (a savestate) from the host */
    const int ctx = payload.isEmpty() ? m_ActiveContext : static_cast<quint8>(payload.at(0));
    m_ResyncCtx = ctx;
    {
      QMutexLocker lock(&m_RecvMutex);
      m_ResyncStateReady = false;
      m_ResyncState.clear();
      m_Received.clear();
      m_ResyncActive = true;
      m_FrameReady.wakeAll();
    }
    emit logMessage(DR_LOG_INFO, QString("netplay: hard resync incoming for ctx %1").arg(ctx));
    break;
  }

  case DR_NETPLAY_PACKET_RESYNC_STATE:
  {
    quint64 frame = 0;
    {
      QDataStream s(payload.left(8));
      s.setByteOrder(QDataStream::LittleEndian);
      s >> frame;
    }
    const QByteArray raw = qUncompress(payload.mid(8));
    QMutexLocker lock(&m_RecvMutex);
    m_ResyncFrame = frame;
    m_ResyncState = raw;
    m_ResyncStateReady = true;
    m_FrameReady.wakeAll();
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
  const int delay = m_InputDelay.load();
  for (int f = 0; f < delay; f++)
  {
    DrNetplayPacket p{};
    p.frame = base + static_cast<quint64>(f);
    p.peerIndex = static_cast<uint8_t>(m_PeerIndex);
    p.context = static_cast<uint8_t>(context);
    recordPacket(p);
    sendInput(p);
  }
  /* Next real (sampled) input goes one past the primed window. */
  m_CtxSend[context] = base + static_cast<quint64>(delay);
}

void DrNetplay::runResync(int context)
{
  QRetro *core = (context >= 0 && context < DR_NETPLAY_MAX_CONTEXTS) ? m_Contexts[context] : nullptr;
  if (!core)
  {
    m_ResyncActive = false;
    return;
  }

  if (m_IsServer)
  {
    /* The host is the source of truth: serialize the live core (inline on this
     * timing thread, between frames) and ship it to the clients. The new frame
     * counter is pushed forward by DR_NETPLAY_RESYNC_MARGIN so it cannot collide with any
     * stale in-flight packets. */
    const quint64 newFrame = m_CtxFrame[context] + DR_NETPLAY_RESYNC_MARGIN;
    const size_t sz = core->serializeSize();
    QByteArray raw(static_cast<int>(sz), '\0');
    if (sz == 0 || !core->serialize(raw.data(), sz))
    {
      emit logMessage(DR_LOG_ERROR, "netplay: hard resync serialize failed");
      m_ResyncActive = false;
      return;
    }

    QByteArray payload;
    {
      QDataStream s(&payload, QIODevice::WriteOnly);
      s.setByteOrder(QDataStream::LittleEndian);
      s << static_cast<quint64>(newFrame);
    }
    payload.append(qCompress(raw));
    QMetaObject::invokeMethod(
      this, [this, payload]() { broadcastVar(DR_NETPLAY_PACKET_RESYNC_STATE, payload); },
      Qt::QueuedConnection);

    emit logMessage(DR_LOG_INFO,
      QString("netplay: hard resync sent (%1 bytes) at frame %2").arg(raw.size()).arg(newFrame));

    {
      QMutexLocker lock(&m_RecvMutex);
      m_CtxFrame[context] = newFrame;
      m_CtxBarrier[context] = newFrame;
      m_ResyncActive = false;
      m_FrameReady.wakeAll();
    }
    primeContext(context);
  }
  else
  {
    /* Client: park here until the host's state arrives, load it, then jump to the
     * host's frame and re-prime so the next gate is a clean barrier. */
    QByteArray state;
    quint64 frame = 0;
    {
      QMutexLocker lock(&m_RecvMutex);
      while (!m_ResyncStateReady && m_Active && !m_Abort && m_ResyncActive.load())
        m_FrameReady.wait(&m_RecvMutex, 100);
      if (!m_ResyncStateReady)
      {
        m_ResyncActive = false; // session ended/aborted before state arrived
        return;
      }
      state = m_ResyncState;
      frame = m_ResyncFrame;
      m_ResyncStateReady = false;
      m_ResyncState.clear();
    }

    if (state.isEmpty() ||
        !core->unserialize(state.constData(), static_cast<size_t>(state.size())))
      emit logMessage(DR_LOG_ERROR, "netplay: hard resync unserialize failed");

    {
      QMutexLocker lock(&m_RecvMutex);
      m_CtxFrame[context] = frame;
      m_CtxBarrier[context] = frame;
      m_ResyncActive = false;
      m_FrameReady.wakeAll();
    }
    primeContext(context);
    emit logMessage(DR_LOG_INFO, QString("netplay: hard resync applied at frame %1").arg(frame));
  }
}

void DrNetplay::recordPacket(const DrNetplayPacket &p)
{
  if (p.peerIndex >= DR_NETPLAY_MAX_PEERS)
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
        broadcast(DR_NETPLAY_PACKET_INPUT, payload);
      else if (!m_Sockets.isEmpty())
        writeMessage(m_Sockets.first(), DR_NETPLAY_PACKET_INPUT, payload);
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

void DrNetplay::writeVarMessage(QTcpSocket *sock, quint8 type, const QByteArray &payload)
{
  if (!sock)
    return;
  QByteArray msg;
  msg.append(static_cast<char>(type));
  QDataStream s(&msg, QIODevice::WriteOnly | QIODevice::Append);
  s.setByteOrder(QDataStream::LittleEndian);
  s << static_cast<quint32>(payload.size());
  msg.append(payload);
  sock->write(msg);
}

void DrNetplay::broadcastVar(quint8 type, const QByteArray &payload, QTcpSocket *except)
{
  for (QTcpSocket *sock : m_Sockets)
    if (sock != except)
      writeVarMessage(sock, type, payload);
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
    /* Release any timing thread parked in a resync wait, then disarm. */
    m_ResyncActive = false;
    m_ResyncStateReady = false;
    m_ResyncState.clear();
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
  case DR_NETPLAY_PACKET_HANDSHAKE:
    return 2;
  case DR_NETPLAY_PACKET_START:
    return 5; // 1-byte game id + 4-byte PRNG seed
  case DR_NETPLAY_PACKET_INPUT:
    return DR_NETPLAY_PACKET_PAYLOAD_SIZE;
  case DR_NETPLAY_PACKET_CANDIDATES:
    return DR_NETPLAY_CANDIDATES_PAYLOAD_SIZE;
  case DR_NETPLAY_PACKET_SET_DELAY:
    return 1;
  case DR_NETPLAY_PACKET_RESYNC_BEGIN:
    return 1;
  case DR_NETPLAY_PACKET_VERSION:
    return DR_NETPLAY_VERSION_HASH_LEN;
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
