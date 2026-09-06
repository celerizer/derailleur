#ifndef DR_HOST_MARIO_PARTY_3_H
#define DR_HOST_MARIO_PARTY_3_H

#include "MarioPartyN64Host.h"
#include <string>

class MarioParty3Host : public MarioPartyN64Host
{
  Q_OBJECT

public:
  explicit MarioParty3Host(QObject *parent = nullptr, const std::string &game = {});
  dr_game game(void) const override { return DR_GAME_MARIOPARTY3; }
};

#endif
