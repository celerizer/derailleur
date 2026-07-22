#include "DolphinGuest.h"

#include <QRetro.h>

/* Applies one Wii control layout to a single player's joypad. */
static void applyWiiControl(QRetroInputJoypad &jp, dr_wii_control control)
{
  /* Remove existing remaps */
  jp.clearButtonRemaps();
  jp.clearAnalogStickRemaps();
  jp.clearAnalogAxisRemaps();
  jp.setAnalogSensitivity(1.0f);

  switch (control)
  {
  case DR_WII_CONTROL_INVALID:
  case DR_WII_CONTROL_UPRIGHT:
    /* Held Wii Remote: the stick drives the d-pad. (Invalid falls back here.) */
    jp.setAnalogStickToDigitalPad(true);
    break;
  case DR_WII_CONTROL_SIDEWAYS_BUTTONS:
    /* 2 = A, 1 = B, flip dpad 90 degrees left */
    jp.setAnalogStickToDigitalPad(true);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_Y);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_X);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_A);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_X, RETRO_DEVICE_ID_JOYPAD_B);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_UP);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_LEFT);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_DOWN);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_RIGHT);
    break;
  case DR_WII_CONTROL_SIDEWAYS_MOTION:
    jp.setAnalogStickToDigitalPad(false);
    /* Motion tilt is twitchy at full range; halve the stick. */
    jp.setAnalogSensitivity(0.5f);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_Y);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_B, RETRO_DEVICE_ID_JOYPAD_X);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_Y, RETRO_DEVICE_ID_JOYPAD_A);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_X, RETRO_DEVICE_ID_JOYPAD_B);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_LEFT, RETRO_DEVICE_ID_JOYPAD_UP);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_DOWN, RETRO_DEVICE_ID_JOYPAD_LEFT);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_RIGHT, RETRO_DEVICE_ID_JOYPAD_DOWN);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_UP, RETRO_DEVICE_ID_JOYPAD_RIGHT);
    /* Held sideways: rotate the left stick (tilt) 90 degrees to match.
     * X <- -Y, Y <- X. */
    jp.setAnalogAxisRemap(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,
      RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y, -1);
    jp.setAnalogAxisRemap(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,
      RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X, 1);
    break;
  case DR_WII_CONTROL_POINTER:
    /* Pointer aiming: keep the sticks analog and swap them both ways, so the
     * core's left stick (the pointer) reads the physical right stick and the
     * core's right stick reads the physical left. */
    jp.setAnalogStickToDigitalPad(false);
    jp.setAnalogStickRemap(RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_INDEX_ANALOG_LEFT);
    jp.setAnalogStickRemap(RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_INDEX_ANALOG_RIGHT);
    break;
  case DR_WII_CONTROL_WAGGLE:
    /* Shake: the stick drives the d-pad and A stands in for the shake (R2). */
    jp.setAnalogStickToDigitalPad(true);
    jp.setButtonRemap(RETRO_DEVICE_ID_JOYPAD_A, RETRO_DEVICE_ID_JOYPAD_R2);
    break;
  default:
    break;
  }
}

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
    applyWiiControl(joypads[slot], control);
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
    applyWiiControl(joypads[i], control);
    log(DR_LOG_INFO, qPrintable(QString("[control] port %1: %2 (all)")
      .arg(i).arg(dr_wii_control_name(control))));
  }
}
