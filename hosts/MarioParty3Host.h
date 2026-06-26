#ifndef DR_HOST_MARIO_PARTY_3_H
#define DR_HOST_MARIO_PARTY_3_H

#include "../DrHost.h"

class MarioParty3Host : public DrHost
{
  Q_OBJECT

public:
  explicit MarioParty3Host(QObject *parent = nullptr);
  dr_game game(void) const override { return DR_GAME_MARIOPARTY3; }
};

#endif
