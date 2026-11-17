#ifndef DR_GUEST_DONKEY_KONG_64_H
#define DR_GUEST_DONKEY_KONG_64_H

#include "../DrGuestN64.h"

class DonkeyKong64 : public DrGuestN64
{
  Q_OBJECT

public:
  DonkeyKong64(QObject *parent = nullptr);

  const char *name() const override { return "Donkey Kong 64"; }
  dr_guest id() const override { return DR_GUEST_DONKEYKONG64; }
  dr_core coreId() const override { return DR_CORE_MUPEN64PLUSNEXT; }
  const char *rom() const override { return "Donkey Kong 64 (USA).z64"; }
  const char *state() const override { return "donkeykong64"; }

  const dr_mp_minigame_t *minigames() const override;
  dr_minigame_result_t minigameResult(unsigned index) override;
  //dr_sync_validator_t syncValidator(void) override;

protected:
  void onBeforeBoot(const DrGameData &data) override;
  void doApplyGameData(const DrGameData &data) override;
  void run() override;

private:
  int m_playercount = 0;
  int m_arcadelvl = -1;
  int m_State = -1;
  size_t adrchain = -1;
  int64_t m_minigameState = -1;
};

#endif
