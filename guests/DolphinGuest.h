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
};

#endif
