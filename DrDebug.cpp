#include "DrDebug.h"

#include <QRandomGenerator>
#include <QVBoxLayout>
#include <QComboBox>
#include <QPushButton>

DrDebug::DrDebug(QWidget *parent) : QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);

  m_combo = new QComboBox(this);
  m_combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  layout->addWidget(m_combo);

  QPushButton *btn = new QPushButton("Request Minigame", this);
  connect(btn, &QPushButton::clicked, this, [this]() {
    int idx = m_combo->currentIndex();
    if (idx < 0 || idx >= m_entries.size())
      return;

    DrGuest               *guest    = m_entries[idx].first;
    const dr_mp_minigame_t *minigame = m_entries[idx].second;

    std::array<dr_player_t, 4> players = {{
      { DR_CHARACTER_DAISY, DR_CONTROL_PORT_P1, DR_CONTROL_TYPE_HUMAN, DR_DIFFICULTY_HARD,      DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 0 },
      { DR_CHARACTER_WARIO, DR_CONTROL_PORT_P2, DR_CONTROL_TYPE_CPU,   DR_DIFFICULTY_VERY_HARD, DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 1 },
      { DR_CHARACTER_YOSHI, DR_CONTROL_PORT_P3, DR_CONTROL_TYPE_CPU,   DR_DIFFICULTY_EASY,      DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 2 },
      { DR_CHARACTER_LUIGI, DR_CONTROL_PORT_P4, DR_CONTROL_TYPE_CPU,   DR_DIFFICULTY_NORMAL,    DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 3 },
    }};
    std::array<bool, 4> playerValid = { true, true, true, true };

    unsigned order[4] = { 0, 1, 2, 3 };
    auto *rng = QRandomGenerator::global();
    for (int i = 3; i > 0; i--) { unsigned j = rng->bounded(i + 1); unsigned t = order[i]; order[i] = order[j]; order[j] = t; }

    switch (minigame->type) {
    case DR_MINIGAME_2V2:
      for (unsigned i = 0; i < 4; i++) {
        bool blue = (order[i] < 2);
        players[i].team_color = blue ? DR_TEAM_COLOR_BLUE : DR_TEAM_COLOR_RED;
        players[i].team_type  = DR_TEAM_TYPE_2V2;
        players[i].team_id    = blue ? 0 : 1;
      }
      break;
    case DR_MINIGAME_1V3:
      for (unsigned i = 0; i < 4; i++) {
        bool solo = (order[i] == 0);
        players[i].team_color = solo ? DR_TEAM_COLOR_BLUE : DR_TEAM_COLOR_RED;
        players[i].team_type  = solo ? DR_TEAM_TYPE_1V3_SOLO : DR_TEAM_TYPE_1V3_GROUP;
        players[i].team_id    = solo ? 0 : 1;
      }
      break;
    default:
      for (unsigned i = 0; i < 4; i++) {
        players[i].team_color = DR_TEAM_COLOR_BLUE;
        players[i].team_type  = DR_TEAM_TYPE_4P;
        players[i].team_id    = i;
      }
      break;
    }

    emit minigameRequested(guest, minigame, players, playerValid);
  });
  layout->addWidget(btn);

  QPushButton *cancelBtn = new QPushButton("Cancel Minigame", this);
  connect(cancelBtn, &QPushButton::clicked, this, [this]() {
    emit cancelRequested();
  });
  layout->addWidget(cancelBtn);

  layout->addStretch();
  setLayout(layout);
}

void DrDebug::populate(const QList<DrGuest*> &guests)
{
  m_combo->clear();
  m_entries.clear();

  for (DrGuest *guest : guests) {
    for (const DrMinigameGroup &group : guest->minigameGroups()) {
      for (const dr_mp_minigame_t *mg : group.minigames) {
        m_combo->addItem(QString("%1 \xe2\x80\x94 %2").arg(group.name).arg(mg->name));
        m_entries.append({guest, mg});
      }
    }
  }
}
