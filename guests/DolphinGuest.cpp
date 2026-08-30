#include "DolphinGuest.h"

#include <QRetro.h>

void DolphinGuest::applyControlRemap(dr_emulation_quirk_t quirks, const dr_player_t players[4])
{
  QRetro *retro = core();
  if (!retro || !retro->input())
    return;

  /* The quirks carry two layouts: the solo player uses `control`, the trio uses
   * `control_team`. A zero `control_team` means everyone shares `control` (all
   * non-split mini-games). */
  const dr_wii_control solo = static_cast<dr_wii_control>(quirks.dolphin.control);
  dr_wii_control team = static_cast<dr_wii_control>(quirks.dolphin.control_team);
  if (team == DR_WII_CONTROL_INVALID)
    team = solo;

  QRetroInputJoypad *joypads = retro->input()->joypads();
  for (unsigned i = 0; i < 4; i++)
  {
    /* In-game slot from the controller port (Mario Party assigns ports
     * non-linearly), falling back to the board index if out of range. */
    unsigned slot = static_cast<unsigned>(players[i].control_port - DR_CONTROL_PORT_P1);
    if (slot >= 4)
      slot = i;
    const bool isSolo = players[i].team_type == DR_TEAM_TYPE_1V3_SOLO;
    const dr_wii_control control = isSolo ? solo : team;
    dr_apply_wii_control(joypads[slot], control);
    log(DR_LOG_INFO, qPrintable(QString("[control] port %1: %2 (%3)")
      .arg(slot).arg(dr_wii_control_name(control)).arg(isSolo ? "solo" : "team")));
  }
}

void DolphinGuest::applyControlProfile(dr_wii_control control)
{
  QRetro *retro = core();
  if (!retro || !retro->input())
    return;

  QRetroInputJoypad *joypads = retro->input()->joypads();
  for (unsigned i = 0; i < 4; i++)
  {
    dr_apply_wii_control(joypads[i], control);
    log(DR_LOG_INFO, qPrintable(QString("[control] port %1: %2 (all)")
      .arg(i).arg(dr_wii_control_name(control))));
  }
}
