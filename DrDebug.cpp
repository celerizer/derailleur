#include "DrDebug.h"

#include <functional>

#include <QAction>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHash>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSpinBox>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
/* Names for the dr_player_t enums that DrCommon.h doesn't already provide a
 * helper for. dr_character_name comes from the header. */
const char *controlTypeName(dr_control_type t)
{
  switch (t)
  {
  case DR_CONTROL_TYPE_HUMAN: return "Human";
  case DR_CONTROL_TYPE_CPU: return "CPU";
  default: return "Invalid";
  }
}

/* Fill a combo with [first, size) enum values, storing each raw value as item
 * data, and preselect def. */
void fillCombo(QComboBox *combo, int first, int size,
  const std::function<QString(int)> &namer, int def)
{
  for (int v = first; v < size; v++)
    combo->addItem(namer(v), v);
  const int idx = combo->findData(def);
  if (idx >= 0)
    combo->setCurrentIndex(idx);
}

/* Per-player field defaults, matching the values this dialog used to hardcode. */
const dr_player_t k_Defaults[4] = {
  { DR_CHARACTER_DAISY, DR_CONTROL_PORT_P1, DR_CONTROL_TYPE_HUMAN, DR_DIFFICULTY_HARD,
    DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 0 },
  { DR_CHARACTER_WARIO, DR_CONTROL_PORT_P2, DR_CONTROL_TYPE_CPU, DR_DIFFICULTY_VERY_HARD,
    DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 1 },
  { DR_CHARACTER_YOSHI, DR_CONTROL_PORT_P3, DR_CONTROL_TYPE_CPU, DR_DIFFICULTY_EASY,
    DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 2 },
  { DR_CHARACTER_LUIGI, DR_CONTROL_PORT_P4, DR_CONTROL_TYPE_CPU, DR_DIFFICULTY_NORMAL,
    DR_TEAM_COLOR_BLUE, DR_TEAM_TYPE_4P, 3 },
};
}

DrDebug::DrDebug(QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);

  m_guestCombo = new QComboBox(this);
  m_guestCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  layout->addWidget(m_guestCombo);

  m_miniMenu = new QMenu(this);
  m_miniButton = new QToolButton(this);
  m_miniButton->setPopupMode(QToolButton::InstantPopup);
  m_miniButton->setToolButtonStyle(Qt::ToolButtonTextOnly);
  m_miniButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
  m_miniButton->setMenu(m_miniMenu);
  m_miniButton->setText(tr("(no mini-game)"));
  layout->addWidget(m_miniButton);

  /* QMenu::triggered fires for actions in submenus too; each action stores its
   * index into m_entries. */
  connect(m_miniMenu, &QMenu::triggered, this, [this](QAction *act) {
    bool ok = false;
    const int idx = act->data().toInt(&ok);
    if (ok)
      selectEntry(idx);
  });

  connect(m_guestCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
    [this](int idx) { refreshMinis(idx); });

  /* One group box of dropdowns per player, arranged P1/P2 on top, P3/P4 below. */
  QGridLayout *playerGrid = new QGridLayout;
  for (int i = 0; i < 4; i++)
  {
    PlayerControls &pc = m_players[i];
    const dr_player_t &def = k_Defaults[i];

    QGroupBox *box = new QGroupBox(tr("Player %1").arg(i + 1), this);
    QFormLayout *form = new QFormLayout(box);
    form->setLabelAlignment(Qt::AlignRight);
    form->setContentsMargins(6, 4, 6, 4);
    form->setVerticalSpacing(3);

    pc.character = new QComboBox(box);
    fillCombo(pc.character, DR_CHARACTER_MARIO, DR_CHARACTER_SIZE,
      [](int v) { return QString::fromUtf8(dr_character_name(dr_character(v))); }, def.character);

    pc.controlType = new QComboBox(box);
    fillCombo(pc.controlType, DR_CONTROL_TYPE_HUMAN, DR_CONTROL_TYPE_SIZE,
      [](int v) { return QString::fromUtf8(controlTypeName(dr_control_type(v))); }, def.control_type);

    pc.difficulty = new QComboBox(box);
    fillCombo(pc.difficulty, DR_DIFFICULTY_VERY_EASY, DR_DIFFICULTY_SIZE,
      [](int v) { return QString::fromUtf8(dr_difficulty_name(dr_difficulty(v))); }, def.difficulty);

    form->addRow(tr("Char:"), pc.character);
    form->addRow(tr("Type:"), pc.controlType);
    form->addRow(tr("Diff:"), pc.difficulty);

    playerGrid->addWidget(box, i / 2, i % 2);
  }
  playerGrid->setColumnStretch(0, 1);
  playerGrid->setColumnStretch(1, 1);
  layout->addLayout(playerGrid);

  QPushButton *btn = new QPushButton("Request Minigame", this);
  connect(btn, &QPushButton::clicked, this, [this]() {
    int idx = m_selectedEntry;
    if (idx < 0 || idx >= m_entries.size())
      return;

    DrGuest *guest = m_entries[idx].first;
    const dr_mp_minigame_t *minigame = m_entries[idx].second;

    std::array<dr_player_t, 4> players{};
    for (int i = 0; i < 4; i++)
    {
      const PlayerControls &pc = m_players[i];
      players[i].character = dr_character(pc.character->currentData().toInt());
      players[i].control_port = dr_control_port(DR_CONTROL_PORT_P1 + i);
      players[i].control_type = dr_control_type(pc.controlType->currentData().toInt());
      players[i].difficulty = dr_difficulty(pc.difficulty->currentData().toInt());
      players[i].team_color = DR_TEAM_COLOR_BLUE;
      players[i].team_type = DR_TEAM_TYPE_4P;
      players[i].team_id = i;
    }

    /* Team layout is inferred from the minigame type, with a random assignment
     * of players to teams. */
    unsigned order[4] = { 0, 1, 2, 3 };
    auto *rng = QRandomGenerator::global();
    for (int i = 3; i > 0; i--)
    {
      unsigned j = rng->bounded(i + 1);
      unsigned t = order[i];
      order[i] = order[j];
      order[j] = t;
    }

    switch (minigame->type)
    {
    case DR_MINIGAME_2V2:
      for (unsigned i = 0; i < 4; i++)
      {
        bool blue = (order[i] < 2);
        players[i].team_color = blue ? DR_TEAM_COLOR_BLUE : DR_TEAM_COLOR_RED;
        players[i].team_type = DR_TEAM_TYPE_2V2;
        players[i].team_id = blue ? 0 : 1;
      }
      break;
    case DR_MINIGAME_1V3:
      for (unsigned i = 0; i < 4; i++)
      {
        bool solo = (order[i] == 0);
        players[i].team_color = solo ? DR_TEAM_COLOR_BLUE : DR_TEAM_COLOR_RED;
        players[i].team_type = solo ? DR_TEAM_TYPE_1V3_SOLO : DR_TEAM_TYPE_1V3_GROUP;
        players[i].team_id = solo ? 0 : 1;
      }
      break;
    case DR_MINIGAME_DUEL:
      players[0].team_type = DR_TEAM_TYPE_SOLO;
      players[1].team_type = ((order[0] == 1) || (order[1] == 1)) ? DR_TEAM_TYPE_SOLO : DR_TEAM_TYPE_INVALID;
      players[2].team_type = order[2] == 1 ? DR_TEAM_TYPE_SOLO : DR_TEAM_TYPE_INVALID;
      players[3].team_type = order[3] == 1 ? DR_TEAM_TYPE_SOLO : DR_TEAM_TYPE_INVALID;
      break;
    default:
      for (unsigned i = 0; i < 4; i++)
      {
        players[i].team_color = DR_TEAM_COLOR_BLUE;
        players[i].team_type = DR_TEAM_TYPE_4P;
        players[i].team_id = i;
      }
      break;
    }

    emit minigameRequested(guest, minigame, players);
  });
  layout->addWidget(btn);

  QPushButton *cancelBtn = new QPushButton("Cancel Minigame", this);
  connect(cancelBtn, &QPushButton::clicked, this, [this]() { emit cancelRequested(); });
  layout->addWidget(cancelBtn);

  /* Turn selector: jump the board's current-turn counter (useful for testing the
   * last-5-turns event and end-of-game handling). */
  QHBoxLayout *turnRow = new QHBoxLayout;
  turnRow->addWidget(new QLabel(tr("Turn:"), this));
  QSpinBox *turnSpin = new QSpinBox(this);
  turnSpin->setRange(1, 99);
  turnRow->addWidget(turnSpin);
  QPushButton *turnBtn = new QPushButton("Set Turn", this);
  connect(turnBtn, &QPushButton::clicked, this,
    [this, turnSpin]() { emit setTurnRequested(turnSpin->value()); });
  turnRow->addWidget(turnBtn);
  turnRow->addStretch();
  layout->addLayout(turnRow);

  layout->addStretch();
  setLayout(layout);
}

void DrDebug::populate(const QList<DrGuest *> &guests)
{
  m_groups.clear();
  m_guestCombo->clear();
  for (DrGuest *guest : guests)
    for (const DrMinigameGroup &group : guest->minigameGroups())
    {
      m_guestCombo->addItem(QString::fromUtf8(group.name));
      m_groups.append({ guest, group });
    }
  refreshMinis(0);
}

void DrDebug::refreshMinis(int groupIdx)
{
  m_miniMenu->clear();
  m_entries.clear();
  m_selectedEntry = -1;
  m_miniButton->setText(tr("(no mini-game)"));
  if (groupIdx < 0 || groupIdx >= m_groups.size())
    return;
  const auto &[guest, group] = m_groups[groupIdx];

  /* One submenu per mini-game type, created on first use so only the types
   * present in this group appear. */
  QHash<int, QMenu *> typeMenus;
  for (const dr_mp_minigame_t *mg : group.minigames)
  {
    const int entryIdx = m_entries.size();
    m_entries.append({ guest, mg });

    QMenu *sub = typeMenus.value(mg->type, nullptr);
    if (!sub)
    {
      sub = m_miniMenu->addMenu(QString::fromUtf8(dr_minigame_type_name(mg->type)));
      typeMenus.insert(mg->type, sub);
    }
    QAction *act = sub->addAction(QString::fromUtf8(mg->name));
    act->setData(entryIdx);
  }

  /* Default to the first entry so the picker isn't empty. */
  if (!m_entries.isEmpty())
    selectEntry(0);
}

void DrDebug::selectEntry(int idx)
{
  if (idx < 0 || idx >= m_entries.size())
    return;
  m_selectedEntry = idx;
  m_miniButton->setText(QString::fromUtf8(m_entries[idx].second->name));
}
