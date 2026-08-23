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

    /* Shrink everything in this box a notch so the extra rows still fit. */
    QFont compact = box->font();
    if (compact.pointSizeF() > 0)
      compact.setPointSizeF(compact.pointSizeF() * 0.85);
    else
      compact.setPixelSize(qMax(1, compact.pixelSize() - 2));
    box->setFont(compact);

    QFormLayout *form = new QFormLayout(box);
    form->setLabelAlignment(Qt::AlignRight);
    form->setContentsMargins(4, 2, 4, 2);
    form->setVerticalSpacing(1);
    form->setHorizontalSpacing(4);

    pc.character = new QComboBox(box);
    fillCombo(pc.character, DR_CHARACTER_MARIO, DR_CHARACTER_SIZE,
      [](int v) { return QString::fromUtf8(dr_character_name(dr_character(v))); }, def.character);

    pc.controlPort = new QComboBox(box);
    fillCombo(pc.controlPort, DR_CONTROL_PORT_P1, DR_CONTROL_PORT_SIZE,
      [](int v) { return QString("P%1").arg(v - DR_CONTROL_PORT_P1 + 1); }, def.control_port);

    pc.controlType = new QComboBox(box);
    fillCombo(pc.controlType, DR_CONTROL_TYPE_HUMAN, DR_CONTROL_TYPE_SIZE,
      [](int v) { return QString::fromUtf8(controlTypeName(dr_control_type(v))); }, def.control_type);

    pc.teamId = new QComboBox(box);
    fillCombo(pc.teamId, 0, 4,
      [](int v) { return QString::number(v); }, def.team_id);

    pc.difficulty = new QComboBox(box);
    fillCombo(pc.difficulty, DR_DIFFICULTY_VERY_EASY, DR_DIFFICULTY_SIZE,
      [](int v) { return QString::fromUtf8(dr_difficulty_name(dr_difficulty(v))); }, def.difficulty);

    /* Keep the dropdowns narrow so two players sit side by side comfortably. */
    for (QComboBox *combo : { pc.character, pc.controlPort, pc.controlType, pc.teamId, pc.difficulty })
    {
      combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
      combo->setMinimumContentsLength(6);
    }

    /* Re-check the team split whenever a team assignment changes. */
    connect(pc.teamId, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
      [this](int) { validateTeams(); });

    form->addRow(tr("Char:"), pc.character);
    form->addRow(tr("Port:"), pc.controlPort);
    form->addRow(tr("Type:"), pc.controlType);
    form->addRow(tr("Team:"), pc.teamId);
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
      players[i].control_port = dr_control_port(pc.controlPort->currentData().toInt());
      players[i].control_type = dr_control_type(pc.controlType->currentData().toInt());
      players[i].difficulty = dr_difficulty(pc.difficulty->currentData().toInt());

      /* No board to read totals from, so hand out something a mini-game HUD can
       * plausibly show. */
      players[i].coins = 100;
      players[i].stars = 3;

      /* team_id is chosen directly now (any 0-3); team_color follows it and team_type
       * follows the minigame's layout for the chosen team. */
      players[i].team_id = pc.teamId->currentData().toInt();
      static const dr_team_color k_teamColors[4] = {
        DR_TEAM_COLOR_BLUE, DR_TEAM_COLOR_RED, DR_TEAM_COLOR_YELLOW, DR_TEAM_COLOR_GREEN
      };
      players[i].team_color = (players[i].team_id >= 0 && players[i].team_id < 4)
        ? k_teamColors[players[i].team_id] : DR_TEAM_COLOR_INVALID;

      switch (minigame->type)
      {
      case DR_MINIGAME_2V2:
        players[i].team_type = DR_TEAM_TYPE_2V2;
        break;
      case DR_MINIGAME_1V3:
        players[i].team_type =
          (players[i].team_id == 0) ? DR_TEAM_TYPE_1V3_SOLO : DR_TEAM_TYPE_1V3_GROUP;
        break;
      case DR_MINIGAME_DUEL:
      case DR_MINIGAME_1P:
      case DR_MINIGAME_ITEM:
        /* team_id 0 = player (participates), 1 = non-player (sits out). */
        players[i].team_type =
          (players[i].team_id == 0) ? DR_TEAM_TYPE_SOLO : DR_TEAM_TYPE_INVALID;
        break;
      default:
        players[i].team_type = DR_TEAM_TYPE_4P;
        break;
      }
    }

    emit minigameRequested(guest, minigame, players);
  });

  /* Request button with a warning icon that lights up when the team split doesn't
   * match the mini-game type. */
  QHBoxLayout *requestRow = new QHBoxLayout;
  requestRow->addWidget(btn);
  m_teamWarning = new QLabel(QString::fromUtf8("⚠"), this); /* ⚠ */
  m_teamWarning->setStyleSheet("color: #d9822b; font-weight: bold;");
  m_teamWarning->setVisible(false);
  requestRow->addWidget(m_teamWarning);
  requestRow->addStretch();
  layout->addLayout(requestRow);

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
  updateTeamOptions();
  validateTeams();
}

dr_minigame_type DrDebug::selectedType() const
{
  if (m_selectedEntry < 0 || m_selectedEntry >= m_entries.size())
    return DR_MINIGAME_INVALID;
  return m_entries[m_selectedEntry].second->type;
}

void DrDebug::updateTeamOptions()
{
  const dr_minigame_type type = selectedType();

  /* Every player can be set to any team id 0-3 for any type, so arbitrary/invalid
   * splits (e.g. everyone on one team) can be tested. The switch only picks a valid
   * default assignment per player so a freshly chosen mini-game starts out legal. */
  int def[4] = { 0, 1, 2, 3 };
  switch (type)
  {
  case DR_MINIGAME_2V2:
  case DR_MINIGAME_DUEL:
    def[0] = 0; def[1] = 0; def[2] = 1; def[3] = 1;
    break;
  case DR_MINIGAME_1V3:
  case DR_MINIGAME_1P:
  case DR_MINIGAME_ITEM:
    def[0] = 0; def[1] = 1; def[2] = 1; def[3] = 1;
    break;
  default: /* 4P, battle, etc: free-for-all, each player their own team */
    break;
  }

  for (int i = 0; i < 4; i++)
  {
    QComboBox *c = m_players[i].teamId;
    const int prev = c->count() ? c->currentData().toInt() : def[i];
    c->blockSignals(true);
    c->clear();
    for (int v = 0; v < 4; v++)
      c->addItem(QString::number(v), v);
    /* Keep the prior choice, else the type's default. */
    int sel = c->findData(prev);
    if (sel < 0)
      sel = c->findData(def[i]);
    c->setCurrentIndex(sel >= 0 ? sel : 0);
    c->blockSignals(false);
  }
}

void DrDebug::validateTeams()
{
  if (!m_teamWarning)
    return;

  /* Count how many players hold the "special" role (team_id 0): the solo, the duelists,
   * the coin/item player, or one 2v2 side. */
  int special = 0;
  for (int i = 0; i < 4; i++)
    if (m_players[i].teamId->currentData().toInt() == 0)
      special++;

  bool valid = true;
  QString why;
  switch (selectedType())
  {
  case DR_MINIGAME_2V2:
    valid = (special == 2);
    why = tr("2v2 needs exactly 2 on Blue and 2 on Red.");
    break;
  case DR_MINIGAME_1V3:
    valid = (special == 1);
    why = tr("1v3 needs exactly one Solo player.");
    break;
  case DR_MINIGAME_DUEL:
    valid = (special == 2);
    why = tr("A duel needs exactly 2 players.");
    break;
  case DR_MINIGAME_1P:
  case DR_MINIGAME_ITEM:
    valid = (special == 1);
    why = tr("This mini-game needs exactly one player.");
    break;
  default:
    valid = true; /* free-for-all types have no team constraint */
    break;
  }

  m_teamWarning->setVisible(!valid);
  m_teamWarning->setToolTip(valid ? QString() : why);
}
