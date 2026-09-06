#ifndef DR_HOST_MARIO_PARTY_6_H
#define DR_HOST_MARIO_PARTY_6_H

#include "MarioPartyGcnHost.h"
#include <string>

class MarioParty6Host : public MarioPartyGcnHost
{
  Q_OBJECT

public:
  explicit MarioParty6Host(QObject *parent = nullptr, const std::string &game = {});
  dr_game game(void) const override { return DR_GAME_MARIOPARTY6; }
};

#endif
