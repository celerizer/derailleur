#include "DrGuestWii.h"

#include <QRetro.h>

DrGuestWii::DrGuestWii(const QString &subdir, const std::string &disc, QObject *parent)
  : DrGuestDolphin(new DrRetroWii(subdir), disc, true, parent)
{
  /* Select the controller type on every port before the first frame */
  connect(core(), &QRetro::onCoreStart, this, [this]() {
    const char *desc = DrRetroWii::controllerDesc(m_controllerType);
    QRetro *c = core();
    unsigned port;

    if (!desc)
      return;

    const std::vector<QRetroControllerPort> &ports = c->input()->controllerPorts();
    for (port = 0; port < 4 && port < ports.size(); port++)
    {
      bool found = false;

      for (const QRetroControllerType &type : ports[port].types)
      {
        if (type.desc == desc)
        {
          c->input()->setSelectedControllerType(port, type.id);
          found = true;
          break;
        }
      }
      log(found ? DR_LOG_INFO : DR_LOG_WARN, qPrintable(QString("port %1 controller: %2%3")
        .arg(port + 1).arg(desc).arg(found ? "" : " (not offered by the core)")));
    }
  }, Qt::DirectConnection);
}

void DrGuestWii::applyControlRemap(dr_emulation_quirk_t quirks, const dr_player_t players[4])
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

  for (unsigned i = 0; i < 4; i++)
  {
    /* In-game slot from the controller port (Mario Party assigns ports
     * non-linearly), falling back to the board index if out of range. */
    unsigned slot = players[i].control_port;
    if (slot >= 4)
      slot = i;
    const bool isSolo = players[i].team_type == DR_TEAM_TYPE_1V3_SOLO;
    const dr_wii_control control = isSolo ? solo : team;
    dr_apply_wii_control(retro, slot, control);
    log(DR_LOG_INFO, qPrintable(QString("[control] port %1: %2 (%3)")
      .arg(slot).arg(dr_wii_control_name(control)).arg(isSolo ? "solo" : "team")));
  }
}

void DrGuestWii::applyControlProfile(dr_wii_control control)
{
  QRetro *retro = core();
  if (!retro || !retro->input())
    return;

  for (unsigned i = 0; i < 4; i++)
  {
    dr_apply_wii_control(retro, i, control);
    log(DR_LOG_INFO, qPrintable(QString("[control] port %1: %2 (all)")
      .arg(i).arg(dr_wii_control_name(control))));
  }
}
