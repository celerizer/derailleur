#ifndef DR_GUEST_DOLPHIN_GUEST_H
#define DR_GUEST_DOLPHIN_GUEST_H

#include "../DrGuest.h"
#include <string>

/// Abstract base for any game hosted by the shared CoreDolphin instance.
/// Exposes the core/disc/savestate paths CoreDolphin needs in order to load
/// and swap between games, without coupling it to any single game's config.
class DolphinGuest : public DrGuest
{
  Q_OBJECT

public:
  DolphinGuest(QObject *parent = nullptr) : DrGuest(parent) {}

  virtual std::string corePath() const = 0;
  virtual std::string discPath() const = 0;
  virtual std::string statePath() const = 0;

protected:
  /// Applies a mini-game's Wii control layout (from its quirks: dolphin.control /
  /// control_team) to all four players' joypads. `players` supplies the 1v3
  /// solo/trio split and the controller-port -> slot mapping. Call from a guest's
  /// doApplyGameData.
  void applyControlRemap(dr_emulation_quirk_t quirks, const dr_player_t players[4]);

  /// Applies a single Wii control layout to all four joypads, ignoring the 1v3
  /// split. Used e.g. to force the pointer layout on a mini-game's explanation
  /// screen before switching to the actual layout for play.
  void applyControlProfile(dr_wii_control control);
};

#endif
