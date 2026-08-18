#include "MarioPartyGcnHost.h"

#include <QRetro.h>
#include <QRetroDirectories.h>
#include <QString>

#include <asm/gecko/mp4.h>

MarioPartyGcnHost::MarioPartyGcnHost(const DrGcnHostConfig &config, QObject *parent)
  : DrHost(parent)
  , m_config(config)
{
  m_core = new QRetro();
  m_ownCore = true;
  m_gamePath = config.game; // so gamePath() yields the ROM (used for the netplay save name)
  if (!m_core->loadCore(config.core.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load core: %1").arg(config.core.c_str())));
    m_valid = false;
  }

  /* Read/write the save from the derailleur save dir (default cwd/save). Set before
   * loadContent so the core sees it when it reads its memory card. */
  m_core->directories()->set(
    QRetroDirectories::Save, dr_save_directory().toUtf8().constData());

  if (!m_core->loadContent(config.game.c_str()))
  {
    log(DR_LOG_ERROR, qPrintable(QString("failed to load content: %1").arg(config.game.c_str())));
    m_valid = false;
  }

  connect(m_core, &QRetro::frameEnd, this, [this]() { run(); }, Qt::DirectConnection);
}

void MarioPartyGcnHost::stampCave(void)
{
  //if (!m_config.cheats.cave || !m_config.cheats.cave_addr)
  //  return;

  for (unsigned i = 0; i <= MP4_CAVE_SIZE; i++)
    writeu8(MP4_CAVE[i], MP4_CAVE_ADDR + i);
}

void MarioPartyGcnHost::run(void)
{
  const bool firstFrame = !m_cheatsInstalled;
  m_cheatsInstalled = true; // so firstFrame is true only once; the gate below is per-120

  if (firstFrame || m_core->frames() % 120 == 0)
  {
    stampCave();

    /* Reroll a fresh candidate pool each cycle (every 120 frames ~= 2s) -- otherwise
     * minigameCandidates() just returns the same cached five -- then stamp the names
     * into the MP4 title block (5 slots x 32 bytes), mirroring MarioPartyN64Host. */
    if (m_MinigameSource)
    {
      m_MinigameSource->rerollMinigames();
      const std::array<DrMinigameCandidate, 5> &cands =
        m_MinigameSource->minigameCandidates(DR_MINIGAME_4P);

      for (unsigned slot = 0; slot < 5; slot++)
      {
        const char *name =
          (cands[slot].minigame && cands[slot].minigame->name) ? cands[slot].minigame->name : "";
        const size_t addr = MP4_TITLE_BLOCK + slot * 32;

        unsigned j = 0;

        writeu8(0x0B, addr + j++);

        if (cands[slot].minigame->flags.flags.lucky)
        {
          writeu8(0x1e, addr + j++);
          writeu8(0x07, addr + j++);
        }
        else if (cands[slot].minigame->flags.flags.lucky)
        {
          writeu8(0x1e, addr + j++);
          writeu8(0x03, addr + j++);
        }

        for (const char *p = name; *p && j < 31; p++)
        {
          unsigned char chara = static_cast<unsigned char>(*p);

          if (chara == ' ')
            chara = 0x10; /* thin space */
          else if (chara == '.') chara = 0x85;
          else if (chara == ',') chara = 0x82;
          else if (chara == '\'') chara = 0x5C;
          else if (chara == '-') chara = 0x3D;
          else if (chara == '!') chara = 0xC2;
          else if (chara == '?') chara = 0xC3;
          else if (chara == '_') chara = 0x86;
          else if (chara == '&') chara = 0x7E;
          else if (chara == '%') chara = 0x7C;
          else if (chara == ':') chara = 0x7B;

          writeu8(chara, addr + j++);
        }
        writeu8(0x00, addr + j); /* null-terminate */
      }
    }
  }

  writeu32(0x4809A5F4, 0x800A23EC);
  writeu32(0x60380001, 0x80031084);
  writeu32(0x4E800020, 0x80031088);

  writeu16(0x0054, 0x8009BC92); // ExecBattle skip mini-game
  writeu16(0x0054, 0x800A218A); // ExecMGSetup skip mini-game
  writeu16(0x0012, 0x8012EF12); // Force a sound group for results screen

  /* Skeleton: the state machine is in place but no transitions are driven yet. */
  switch (m_State)
  {
  case DR_GCN_HOST_STATE_INVALID:
    break;
  case DR_GCN_HOST_STATE_BEFORE_BOARD:
    break;
  case DR_GCN_HOST_STATE_BOARD:
    break;
  case DR_GCN_HOST_STATE_BEFORE_ROULETTE:
    break;
  case DR_GCN_HOST_STATE_ROULETTE:
    break;
  case DR_GCN_HOST_STATE_AFTER_ROULETTE:
    break;
  case DR_GCN_HOST_STATE_MINIGAME:
    break;
  case DR_GCN_HOST_STATE_SIZE:
    break;
  }
}

void MarioPartyGcnHost::writeResults(DrGuest *guest)
{
  (void)guest;
}

void MarioPartyGcnHost::clearResults(void)
{
}
