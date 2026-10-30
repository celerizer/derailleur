#include "DiddyKongRacing.h"
#include "DrPaths.h"

#include <QCryptographicHash>
#include <QFile>
#include <QRetro.h>

/* USA Rev 1 (us.v80). Symbols and field offsets from:
 * https://github.com/DavidSM64/Diddy-Kong-Racing
 * ver/symbols/symbol_addrs.us.v80.txt, src/thread3_main.c, include/structs.h.
 * Never use this map with the original USA release or PAL ROMs. */
namespace
{
constexpr size_t DKR_GAME_MODE = 0x80123A6C;
constexpr size_t DKR_SETTINGS = 0x80123A90;
constexpr size_t DKR_RACERS_BY_PORT = 0x8011B46C;
constexpr size_t DKR_LEVEL_TABLE = 0x801216FC;

bool ramPointer(uint32_t address, size_t bytes)
{
  return address >= 0x80000000 && address <= 0x80800000 - bytes;
}

const dr_mp_minigame_t DKR_MINIGAMES[] = {
  { "DKR: Ancient Lake", DR_MINIGAME_4P, 5, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Fossil Canyon", DR_MINIGAME_4P, 3, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Jungle Falls", DR_MINIGAME_4P, 29, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Hot Top Volcano", DR_MINIGAME_4P, 7, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Everfrost Peak", DR_MINIGAME_4P, 13, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Walrus Cove", DR_MINIGAME_4P, 6, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Snowball Valley", DR_MINIGAME_4P, 9, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Frosty Village", DR_MINIGAME_4P, 28, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Whale Bay", DR_MINIGAME_4P, 8, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Crescent Island", DR_MINIGAME_4P, 10, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Pirate Lagoon", DR_MINIGAME_4P, 4, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Treasure Caves", DR_MINIGAME_4P, 30, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Windmill Plains", DR_MINIGAME_4P, 20, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Greenwood Village", DR_MINIGAME_4P, 18, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Boulder Canyon", DR_MINIGAME_4P, 19, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Haunted Woods", DR_MINIGAME_4P, 31, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Spacedust Alley", DR_MINIGAME_4P, 17, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Darkmoon Caverns", DR_MINIGAME_4P, 32, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Spaceport Alpha", DR_MINIGAME_4P, 15, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Star City", DR_MINIGAME_4P, 33, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Fire Mountain", DR_MINIGAME_4P, 11, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Icicle Pyramid", DR_MINIGAME_4P, 27, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Darkwater Beach", DR_MINIGAME_4P, 26, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { "DKR: Smokey Castle", DR_MINIGAME_4P, 25, 0xFF, DR_NO_QUIRKS, DR_FLAG_NO_DIFFICULTY },
  { nullptr, DR_MINIGAME_INVALID, 0xFF, 0xFF, DR_NO_QUIRKS, DR_NO_FLAGS }
};
}

DiddyKongRacing::DiddyKongRacing(QObject *parent)
  : DrGuestN64(parent)
{
  QFile file(DrPaths::instance().romsDirectory() + "/" + rom());
  if (!file.open(QIODevice::ReadOnly) ||
      QCryptographicHash::hash(file.readAll(), QCryptographicHash::Sha1).toHex() !=
        "6d96743d46f8c0cd0edb0ec5600b003c89b93755")
  {
    m_valid = false;
    log(DR_LOG_WARN, "Diddy Kong Racing requires the USA (En,Fr) Rev 1 big-endian ROM");
  }
  m_retro->init(coreId(), rom(), false);
  for (unsigned port = 0; port < 4; port++)
  {
    core()->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_B);
    core()->input()->remapButton(port, RETRO_DEVICE_ID_JOYPAD_X, RETRO_DEVICE_ID_JOYPAD_L2);
  }
}

const dr_mp_minigame_t *DiddyKongRacing::minigames() const
{
  return DKR_MINIGAMES;
}

void DiddyKongRacing::doApplyGameData(const DrGameData &data)
{
  m_course = -1;
  for (const auto &game : DKR_MINIGAMES)
    if (data.minigame && game.name && game.minigame_id == data.minigame->minigame_id)
      m_course = game.minigame_id;
  if (m_course < 0)
    return;

  core()->cheatReset();
  core()->reset();
  m_frames = 0;
  m_readyFrames = 0;
  m_winnerIndex = -1;
  m_launchPending = true;
  m_viewportReset = false;
  m_racersReady = false;
  // DKR creates human racers first and CPU racers afterwards. Compact the
  // host's humans into native slots, routing their original controller ports.
  core()->input()->clearPortRoutes();
  unsigned slot = 0;
  for (unsigned i = 0; i < 4; i++)
    if (data.players[i].control_type != DR_CONTROL_TYPE_CPU)
    {
      m_slotToIndex[slot] = static_cast<int>(i);
      core()->input()->setPortRoute(slot++, data.players[i].control_port);
    }
  m_humanCount = slot;
  for (unsigned i = 0; i < 4; i++)
    if (data.players[i].control_type == DR_CONTROL_TYPE_CPU)
    {
      m_slotToIndex[slot] = static_cast<int>(i);
      core()->input()->setPortRoute(slot++, -1);
    }
  startMinigame();
  muteForFrames(1800);
  if (!m_humanCount)
  {
    log(DR_LOG_WARN, "Diddy Kong Racing requires at least one human player");
    m_launchPending = false;
    finishMinigameInFrames(1);
  }
}

void DiddyKongRacing::launch()
{
  uint32_t settings = 0, table = 0;
  uint8_t vehicle = 0;
  if (m_retro->readu32(&settings, DKR_SETTINGS) != DR_OK || !ramPointer(settings, 0x114) ||
      m_retro->readu32(&table, DKR_LEVEL_TABLE) != DR_OK || !ramPointer(table, 6 * 34) ||
      m_retro->readu8(&vehicle, table + m_course * 6 + 2) != DR_OK)
    return;

  vehicle &= 0xF;
  if (vehicle > 2)
    return;
  m_retro->writeu32(1, 0x800DFA38); // tracks mode
  m_retro->writeu32(m_humanCount, 0x800DFA3C); // active players
  m_retro->writeu32(m_humanCount - 1, 0x80123A80); // viewport count minus one
  m_retro->writeu32(1, 0x800E0990); // two-player Tracks mode: four racers
  m_retro->writeu32(0, 0x800E0318); // no magic codes (including adventure 2P)
  m_retro->writeu32(0, 0x8011B474); // no time trial
  for (unsigned slot = 0; slot < 8; slot++)
  {
    // Unique native characters in racer order; no duplicates in the grid.
    m_retro->writeu8(slot, settings + 0x54 + 0x18 * slot + 5);
    m_retro->writeu8(slot, settings + 0x54 + 0x18 * slot + 6);
    if (slot < 4)
      m_retro->writeu8(vehicle, 0x80126F80 + slot);
  }

  // Return MENU_RESULT_FLAGS_200 | level ID. The game's normal menu transition
  // unloads the background and loads a race with the correct default vehicle.
  // Use the cheat API so Mupen also invalidates translated instruction blocks.
  // track_setup_racers normally chooses eight racers for one human and
  // three for three humans. Change only those count assignments to four.
  // 8000CF7C: addiu t5, zero, 8 -> 4 (default gNumRacers).
  // 8000D01C: lw t5, 0x144(sp) -> addiu t5, zero, 4 (3/4-player count).
  // The game then spawns/initialises its own AI with the correct flags.
  const QByteArray cheat = QString(
    "8100CF7C 240D+8100CF7E 0004+8100D01C 240D+8100D01E 0004+"
    "810819F4 03E0+810819F6 0008+810819F8 2402+810819FA %1")
    .arg(0x200 | m_course, 4, 16, QLatin1Char('0')).toLatin1();
  core()->cheatSet(0, true, cheat.constData());
  m_launchPending = false;
}

bool DiddyKongRacing::racerAddress(unsigned slot, uint32_t *address)
{
  uint32_t racers = 0, object = 0;
  return m_retro->readu32(&racers, DKR_RACERS_BY_PORT) == DR_OK && ramPointer(racers, 16) &&
    m_retro->readu32(&object, racers + slot * 4) == DR_OK && ramPointer(object, 0x68) &&
    m_retro->readu32(address, object + 0x64) == DR_OK && ramPointer(*address, 0x224);
}

void DiddyKongRacing::run()
{
  if (!m_minigameActive || finishPending())
    return;
  ++m_frames;
  if (!m_racersReady && m_frames >= 1800)
  {
    log(DR_LOG_ERROR, "Diddy Kong Racing failed to enter the selected event");
    core()->cheatReset();
    finishMinigame();
    return;
  }
  uint32_t mode = 0;
  if (m_retro->readu32(&mode, DKR_GAME_MODE) != DR_OK)
    return;
  if (m_launchPending)
  {
    // Let the logos/menu initialise all heaps and settings before transitioning.
    if (m_frames >= 360 && mode == 1)
      launch();
  }
  else if (mode == 0)
  {
    if (!m_viewportReset)
    {
      // The normal track-menu unload disables its custom camera viewport.
      // Our direct transition skips that menu cleanup, so drop the inherited
      // menu/background flags before rendering the race (camDisableUserView).
      for (unsigned slot = 0; slot < 4; slot++)
        m_retro->writeu32(0, 0x800DD5D4 + slot * 0x34 + 0x30);
      m_viewportReset = true;
    }
    uint32_t racers[4] = {};
    for (unsigned slot = 0; slot < 4; slot++)
    {
      int16_t player = -1;
      if (!racerAddress(slot, &racers[slot]) ||
          (!m_racersReady && (m_retro->reads16(&player, racers[slot]) != DR_OK ||
                            player != (slot < m_humanCount ? static_cast<int16_t>(slot) : -1))))
        return;
    }
    if (!m_racersReady)
    {
      // Racer objects exist before level_load finishes. Wait for its remaining
      // initialisation before reading results.
      if (++m_readyFrames < 90)
        return;
      m_racersReady = true;
      muteForFrames(1);
    }
    for (unsigned slot = 0; slot < 4; slot++)
    {
      int16_t place = 0;
      int8_t finished = 0;
      if (m_retro->reads16(&place, racers[slot] + 0x1AC) == DR_OK && place == 1 &&
          m_retro->reads8(&finished, racers[slot] + 0x1D8) == DR_OK && finished)
      {
        // Eliminated battle racers finish in places 4, 3, 2. Only the surviving
        // racer's final place 1 wins; the first eliminated racer never wins.
        m_winnerIndex = m_slotToIndex[slot];
        finishMinigameInFrames(180);
        return;
      }
    }
  }
}

dr_minigame_result_t DiddyKongRacing::minigameResult(unsigned index)
{
  return { index < 4 && static_cast<int>(index) == m_winnerIndex ? 10 : 0, 0 };
}
