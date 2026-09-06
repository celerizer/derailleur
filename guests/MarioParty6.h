#ifndef MARIO_PARTY_6_H
#define MARIO_PARTY_6_H

#include "MarioPartyGcn.h"

class MarioParty6 : public MarioPartyGcn
{
  Q_OBJECT

public:
  MarioParty6(QRetro *sharedCore, QObject *parent = nullptr);
  const char *name() const override { return "Mario Party 6"; }
  dr_guest id() const override { return DR_GUEST_MARIOPARTY6; }

protected:
  void run() override;

private:
  int m_dayNightFrames = 0; // frames left holding the board's day/night bit
  bool m_night = false;     // the side of it rolled for this mini-game
};

#endif
