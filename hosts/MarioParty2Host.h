#ifndef DR_HOST_MARIO_PARTY_2_HOST_H
#define DR_HOST_MARIO_PARTY_2_HOST_H

#include "../DrHost.h"

class MarioParty2Host : public DrHost
{
  Q_OBJECT

public:
  explicit MarioParty2Host(QObject *parent = nullptr);
  void writeMinigameNames();
};

#endif
