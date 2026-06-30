#ifndef DR_NETPLAY_H
#define DR_NETPLAY_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QString>
#include <QWaitCondition>
#include <atomic>
#include <cstdint>

#include "QRetroInput.h"

class DrInputStore;
class QRetro;
class QRetroInputBackend;
class QTcpServer;
class QTcpSocket;

/* Wire message types: [1-byte type][fixed-size payload]. */
typedef enum
{
  DR_NETPLAY_PACKET_HANDSHAKE    = 0x01, /* server -> client: { peerIndex, peerCount } */
  DR_NETPLAY_PACKET_START        = 0x02, /* server -> client: begin lockstep */
  DR_NETPLAY_PACKET_INPUT        = 0x03, /* either direction: DrNetplayPacket */
  DR_NETPLAY_PACKET_CANDIDATES   = 0x04, /* server -> client: candidate (guest, minigame) pairs */
  DR_NETPLAY_PACKET_SET_DELAY    = 0x05, /* any peer (relayed): new input delay, 1 byte */
  DR_NETPLAY_PACKET_RESYNC_BEGIN = 0x06, /* server -> clients: { context } stop for resync */
  DR_NETPLAY_PACKET_RESYNC_STATE = 0x07, /* server -> clients: [u32 len][u64 frame][zstate] */
  DR_NETPLAY_PACKET_VERSION      = 0x08  /* client -> server: build hash on connect */
} dr_netplay_packet_type;

/* Maximum peers in a session (also the per-frame input array width). */
#define DR_NETPLAY_MAX_PEERS 4

/* Maximum QRetro contexts (host + guests) tracked per peer. */
#define DR_NETPLAY_MAX_CONTEXTS 16

/* A hard resync jumps the per-context frame counter forward by this margin so
 * the post-resync timeline can never collide with stale in-flight packets from
 * before the resync (which sit within a few frames of the old counter). */
#define DR_NETPLAY_RESYNC_MARGIN 600

/* Input packet payload: quint64 + quint8 + quint8 + quint16 + 6 * qint16. */
#define DR_NETPLAY_PACKET_PAYLOAD_SIZE (8 + 1 + 1 + 2 + 6 * 2)

/* Minigame candidates: this many slots, each a pair of quint16 (guest index,
 * minigame index). */
#define DR_NETPLAY_CANDIDATE_SLOTS 5
#define DR_NETPLAY_CANDIDATES_PAYLOAD_SIZE (DR_NETPLAY_CANDIDATE_SLOTS * 2 * 2)

/* Fixed, null-padded git short hash exchanged on connect for version checks. */
#define DR_NETPLAY_VERSION_HASH_LEN 16

#pragma pack(push, 1)
struct DrNetplayPacket
{
  uint64_t frame; // per-context netplay frame counter (not the libretro counter)
  uint8_t peerIndex;
  uint8_t context; // which QRetro context (host or a guest) this input is for
  uint16_t bitmask;
  int16_t leftX, leftY;
  int16_t rightX, rightY;
  int16_t l2, r2;
};
#pragma pack(pop)

/**
 * Handles TCP networking, input delay and lockstep synchronization for netplay,
 * writing committed frames into a DrInputStore.
 *
 * Every peer runs the same derailleur instance; "server" is purely a network
 * coordination role (peer index 0) and is unrelated to DrHost. Clients connect
 * to the server in a star topology; the server relays each peer's input to the
 * others so all peers observe every peer's input for every frame.
 *
 * Threading: frameBegin is emitted on a QRetro timing thread, while all sockets
 * live on the main thread. onFrameBegin() therefore records its own input,
 * marshals network sends to the main thread, and blocks the timing thread on a
 * wait condition until the expected frame is complete (filled by the main
 * thread's socket slots) — genuinely pausing the core rather than busy-spinning.
 *
 * Each QRetro context (the host and each guest core) keeps its own frame counter
 * so boot/load-time differences between peers don't desync them; only the
 * foreground context (set via setActiveContext) is gated at any time.
 */
class DrNetplay : public QObject
{
  Q_OBJECT

public:
  explicit DrNetplay(DrInputStore *store, QObject *parent = nullptr);

  void hostSession(quint16 port, int playerCount);
  void joinSession(const QString &address, quint16 port);

  /// Clears every per-context netplay frame counter and pending input. Called
  /// at session start. Uses internal counters, never the libretro counter.
  void resetFrameCounter();

  /// Selects the foreground QRetro context (host or a guest) — the only context
  /// that is gated. Called at each foreground switch (post-warmup host, or a
  /// minigame load), which acts as that context's per-context sync point.
  void setActiveContext(QRetro *core);

  /// Freezes the currently active context at the current frame: its core stops
  /// advancing (retro_run is held off in onFrameBegin) until it is re-activated.
  /// Connect this to minigameRequested with a DirectConnection so every peer
  /// freezes the host on the same lockstep frame before the async minigame
  /// launch can run a different number of host frames on each peer.
  void freezeActiveContext();

  void setInputDelay(int frames) { m_InputDelay = frames; }
  int inputDelay() const { return m_InputDelay.load(); }
  void setTimeout(int ms) { m_TimeoutMs = ms; }

  /// Changes the input delay (buffer frames) locally and, during a session,
  /// propagates it to every peer (the server relays). Safe to call mid-session:
  /// each context back-fills any send gap so no peer stalls. Callable from the
  /// main (UI) thread.
  void changeInputDelay(int frames);

  bool isServer() const { return m_IsServer; }
  bool hasSession() const { return m_Server != nullptr || !m_Sockets.isEmpty(); }

  /// Wakes any timing thread parked in waitForFrame so it can exit. Call before
  /// tearing down cores (e.g. on window close) to avoid blocking shutdown.
  void abort();

  /// Server only: tells every connected client which game to start (a dr_game
  /// value), then begins lockstep locally.
  void startGame(int gameId);

  /// Server only: forces a hard resync of the active context — every client
  /// stops, receives the host's (compressed) savestate, loads it and resumes
  /// in lockstep from a fresh sync point. Safe to call mid-session.
  void requestHardResync();

  /// Server only: broadcasts the chosen minigame candidates to clients as
  /// opaque (guestIndex, minigameIndex) pairs (-1 = none). Always exactly 5.
  void sendCandidates(const QList<QPair<int, int>> &candidates);

  /// Re-targets an existing physical (e.g. SDL3) backend so it samples local
  /// hardware into our private joypad array instead of a core's array.
  void setLocalSource(QRetroInputBackend *backend);

  /// Installs a QRetroInputBackendShared on `core` and gates its frameBegin.
  void attachCore(QRetro *core);

  bool sessionActive() const { return m_Active; }

signals:
  void sessionStarted(int peerIndex, int peerCount);
  void sessionError(const QString &reason);
  void peerCountChanged(int connected, int total);
  /// Emitted on a client when the server selects a game (a dr_game value).
  void startGameRequested(int gameId);
  /// Emitted on a client with the server's chosen candidate (guest, minigame)
  /// index pairs (-1 = none).
  void candidatesReceived(QList<QPair<int, int>> candidates);
  /// Emitted when the input delay changes (locally or from a peer) so the UI
  /// can reflect the new value. Does not re-broadcast.
  void inputDelayChanged(int frames);
  /// Diagnostic log; level matches DrLogger::message (DR_LOG_*).
  void logMessage(unsigned level, const QString &msg);

private slots:
  void onNewConnection();
  void onSocketReadyRead();
  void onSocketDisconnected();

private:
  // Frame coordination (timing thread).
  void onFrameBegin(int context);
  void runResync(int context);
  void sampleLocal();
  bool isFrameCompleteLocked(int context, quint64 frame) const;
  bool waitForFrame(int context, quint64 frame);
  void commitMergedFrame(int context, quint64 frame);

  // Networking (main thread).
  void primeContext(int context);
  void recordPacket(const DrNetplayPacket &p);
  void sendInput(const DrNetplayPacket &p);
  void writeMessage(QTcpSocket *sock, quint8 type, const QByteArray &payload);
  void broadcast(quint8 type, const QByteArray &payload, QTcpSocket *except = nullptr);
  void writeVarMessage(QTcpSocket *sock, quint8 type, const QByteArray &payload);
  void broadcastVar(quint8 type, const QByteArray &payload, QTcpSocket *except = nullptr);
  void handleMessage(QTcpSocket *sock, quint8 type, const QByteArray &payload);
  void dropSession(const QString &reason);

  static quint64 frameKey(int context, quint64 frame);
  static int payloadLength(quint8 type);
  static QByteArray encodePacket(const DrNetplayPacket &p);
  static DrNetplayPacket decodePacket(const QByteArray &b);
  static DrNetplayPacket packetFromJoypad(
    QRetroInputJoypad &jp, int peerIndex, int context, quint64 frame);
  static void applyPacketToJoypad(const DrNetplayPacket &p, QRetroInputJoypad &jp);

  DrInputStore *m_Store = nullptr;

  QRetroInputBackend *m_LocalSource = nullptr;
  QRetroInputJoypad m_LocalPads[QRETRO_INPUT_DEFAULT_MAX_JOYPADS];
  QRetroInputJoypad m_CommitPads[DR_NETPLAY_MAX_PEERS];

  QTcpServer *m_Server = nullptr;
  QList<QTcpSocket *> m_Sockets;
  QHash<QTcpSocket *, QByteArray> m_RxBuf;
  int m_NextAssignIndex = 1;

  bool m_IsServer = false;
  int m_PeerIndex = 0;
  int m_PeerCount = 1;
  std::atomic<int> m_InputDelay{ 2 };
  int m_TimeoutMs = 10000;

  bool m_Active = false;
  bool m_Abort = false;

  // Per-context state. attachCore() assigns each QRetro a context id in a
  // deterministic order (host first, then guests), identical across peers.
  QHash<QRetro *, int> m_ContextIds;
  QRetro *m_Contexts[DR_NETPLAY_MAX_CONTEXTS] = {}; // context id -> core, for state I/O
  int m_ContextCount = 0;
  int m_ActiveContext = -1;
  int m_FrozenContext = -1; // context held (retro_run paused) until re-activated
  quint64 m_CtxFrame[DR_NETPLAY_MAX_CONTEXTS] = {};   // next netplay frame to consume per context
  quint64 m_CtxBarrier[DR_NETPLAY_MAX_CONTEXTS] = {}; // per-context sync-point frame (waits w/o timeout)
  quint64 m_CtxSend[DR_NETPLAY_MAX_CONTEXTS] = {};    // next local frame to send per context (delay pipeline)

  quint64 m_LocalFrame = 0; // singleplayer frame counter

  struct FrameInputs
  {
    DrNetplayPacket pkts[DR_NETPLAY_MAX_PEERS] = {};
    bool have[DR_NETPLAY_MAX_PEERS] = {};
  };
  QMutex m_RecvMutex;
  QWaitCondition m_FrameReady;
  QHash<quint64, FrameInputs> m_Received;

  // Hard resync. m_ResyncActive is read on the timing thread; the rest is guarded
  // by m_RecvMutex. The host serializes the active core and ships it; clients park
  // until the state arrives, load it, and resume from m_ResyncFrame.
  std::atomic<bool> m_ResyncActive{ false };
  int m_ResyncCtx = -1;
  bool m_ResyncStateReady = false;
  quint64 m_ResyncFrame = 0;
  QByteArray m_ResyncState; // decompressed savestate awaiting load (client)
};

#endif
