#include "DrChallenge.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPushButton>
#include <QRandomGenerator>
#include <QSettings>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace
{
/* The challenge roster is the first eight characters, Mario through Daisy. */
const int k_FirstCharacter = DR_CHARACTER_MARIO;
const int k_CharacterCount = 8;

/* Tiers 1-5 mirror Very Easy..Very Hard; 6 is the team-only "Unfair" tier. */
const int k_TierUnfair = 6;
const int k_TierNormal = 3;

/* The last-played character lives alongside the rest of the app config. */
const char *const k_CharacterKey = "challenge/character";
QString k_SettingsPath()
{
  return QDir::current().filePath("derailleur.ini");
}

bool isTeamType(dr_minigame_type t)
{
  return t == DR_MINIGAME_2V2 || t == DR_MINIGAME_1V3;
}

dr_difficulty tierDifficulty(int tier)
{
  return static_cast<dr_difficulty>(DR_DIFFICULTY_VERY_EASY + tier - 1);
}

/* The tier a mini-game is actually played (and scored) at for a given selection.
 * A 1P mini has no difficulty, so it is always the full five stars. An Unfair
 * selection only applies to team minis; anything else drops to Very Hard. */
int effectiveTier(const dr_mp_minigame_t *minigame, int selectedTier)
{
  if (minigame->type == DR_MINIGAME_1P)
    return 5;
  if (selectedTier == k_TierUnfair && !isTeamType(minigame->type))
    return 5;
  return selectedTier;
}

QString tierName(int tier)
{
  if (tier == k_TierUnfair)
    return QStringLiteral("Unfair");
  return QString::fromUtf8(dr_difficulty_name(tierDifficulty(tier)));
}

/* The five base tiers as filled/hollow stars. An Unfair clear is appended as an
 * extra star by the caller, and never shown as an empty slot. */
QString starText(int cleared)
{
  QString stars;
  for (int i = 1; i <= 5; i++)
    stars += (i <= cleared) ? QString::fromUtf8("\xe2\x98\x85") : QString::fromUtf8("\xe2\x98\x86");
  return stars;
}
}

DrChallenge::DrChallenge(QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);

  QHBoxLayout *row = new QHBoxLayout;
  row->addWidget(new QLabel(tr("Character:"), this));
  m_character = new QComboBox(this);
  m_character->setIconSize(QSize(32, 32));
  for (int c = k_FirstCharacter; c < k_FirstCharacter + k_CharacterCount; c++)
  {
    /* The player art is indexed by the raw dr_character value. */
    m_character->addItem(QIcon(QString(":/assets/player-32px/%1.png").arg(c)),
      QString::fromUtf8(dr_character_name(static_cast<dr_character>(c))), c);
  }
  row->addWidget(m_character);

  row->addWidget(new QLabel(tr("Difficulty:"), this));
  m_difficulty = new QComboBox(this);
  row->addWidget(m_difficulty);
  row->addStretch();

  /* When set, Start walks through every still-uncleared mini-game one after
   * another instead of playing just the highlighted one. */
  m_continuous = new QCheckBox(tr("Continuous play"), this);
  m_continuous->setToolTip(
    tr("Play random uncleared mini-games back to back at the chosen difficulty."));
  connect(m_continuous, &QCheckBox::toggled, this, [this]() { updateDifficultyChoices(); });
  row->addWidget(m_continuous);
  layout->addLayout(row);

  m_tree = new QTreeWidget(this);
  m_tree->setColumnCount(2);
  m_tree->setHeaderLabels({ tr("Mini-game"), tr("Cleared") });
  m_tree->header()->setStretchLastSection(false);
  m_tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  m_tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
  layout->addWidget(m_tree);

  /* The Unfair tier only exists for team mini-games, so the choices depend on
   * what is selected. */
  connect(m_tree, &QTreeWidget::currentItemChanged, this,
    [this]() { updateDifficultyChoices(); });

  m_totals = new QLabel(this);
  layout->addWidget(m_totals);

  QPushButton *startBtn = new QPushButton(tr("Start Challenge"), this);
  connect(startBtn, &QPushButton::clicked, this, [this]() { start(); });
  layout->addWidget(startBtn);

  /* Restore whoever was played last, then track changes. */
  {
    QSettings settings(k_SettingsPath(), QSettings::IniFormat);
    const int last = settings.value(k_CharacterKey, DR_CHARACTER_MARIO).toInt();
    const int idx = m_character->findData(last);
    if (idx >= 0)
      m_character->setCurrentIndex(idx);
  }
  connect(m_character, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
    QSettings settings(k_SettingsPath(), QSettings::IniFormat);
    settings.setValue(k_CharacterKey, static_cast<int>(currentCharacter()));
    reload();
  });

  m_cleared = loadFor(currentCharacter());
  updateDifficultyChoices();
}

void DrChallenge::populate(const QList<DrGuest *> &guests)
{
  m_entries.clear();
  m_tree->clear();

  for (DrGuest *guest : guests)
  {
    for (const DrMinigameGroup &group : guest->minigameGroups())
    {
      QTreeWidgetItem *head = new QTreeWidgetItem(m_tree);
      head->setText(0, QString::fromUtf8(group.name));
      head->setFirstColumnSpanned(true);

      for (const dr_mp_minigame_t *mg : group.minigames)
      {
        /* Unclassified entries are not offered anywhere else either. */
        if (!mg->name || mg->type == DR_MINIGAME_INVALID)
          continue;
        QTreeWidgetItem *item = new QTreeWidgetItem(head);
        item->setText(0, QString::fromUtf8(mg->name));
        item->setData(0, Qt::UserRole, static_cast<int>(m_entries.size()));
        m_entries.append({ guest, mg });
      }

      if (head->childCount() == 0)
        delete head;
      else
        head->setExpanded(true);
    }
  }

  refreshStars();
  updateDifficultyChoices();
}

const dr_mp_minigame_t *DrChallenge::selectedMinigame(DrGuest **guestOut) const
{
  QTreeWidgetItem *item = m_tree->currentItem();
  if (!item)
    return nullptr;

  const QVariant data = item->data(0, Qt::UserRole);
  if (!data.isValid()) /* a group header */
    return nullptr;

  const int idx = data.toInt();
  if (idx < 0 || idx >= m_entries.size())
    return nullptr;

  if (guestOut)
    *guestOut = m_entries[idx].first;
  return m_entries[idx].second;
}

void DrChallenge::updateDifficultyChoices()
{
  /* In continuous mode the highlighted item is only a starting point, so offer
   * the full set of tiers (including Unfair) regardless of what is selected. */
  const bool continuous = m_continuous && m_continuous->isChecked();
  const dr_mp_minigame_t *minigame = selectedMinigame();
  const bool team = continuous || (minigame && isTeamType(minigame->type));
  const bool solo = !continuous && minigame && minigame->type == DR_MINIGAME_1P;

  /* Coming back from a 1P entry there is no meaningful previous tier to keep. */
  const QVariant current = m_difficulty->currentData();
  const int previous =
    (m_difficulty->isEnabled() && current.isValid()) ? current.toInt() : k_TierNormal;

  m_difficulty->clear();

  /* A 1P mini-game has no opponents to scale, so there is nothing to pick:
   * clearing it at all counts as the full five stars. */
  if (solo)
  {
    m_difficulty->addItem(tr("N/A"), 5);
    m_difficulty->setEnabled(false);
    return;
  }
  m_difficulty->setEnabled(true);

  for (int tier = 1; tier <= 5; tier++)
    m_difficulty->addItem(tierName(tier), tier);
  if (team)
    m_difficulty->addItem(tierName(k_TierUnfair), k_TierUnfair);

  const int idx = m_difficulty->findData(previous);
  m_difficulty->setCurrentIndex(idx >= 0 ? idx : m_difficulty->findData(k_TierNormal));
}

void DrChallenge::refreshStars()
{
  for (int i = 0; i < m_tree->topLevelItemCount(); i++)
  {
    QTreeWidgetItem *head = m_tree->topLevelItem(i);
    for (int j = 0; j < head->childCount(); j++)
    {
      QTreeWidgetItem *item = head->child(j);
      const int idx = item->data(0, Qt::UserRole).toInt();
      if (idx < 0 || idx >= m_entries.size())
        continue;

      const DrGuest *guest = m_entries[idx].first;
      const dr_mp_minigame_t *minigame = m_entries[idx].second;
      const int cleared = m_cleared.value(recordKey(guest, minigame), 0);

      QString text = starText(qMin(cleared, 5));
      if (cleared >= k_TierUnfair)
        text += QString::fromUtf8("\xe2\x98\x85"); /* only appears once earned */
      item->setText(1, text);

      /* A 1P clear has no tier to name. */
      if (cleared <= 0)
        item->setToolTip(1, tr("Not cleared"));
      else if (minigame->type == DR_MINIGAME_1P)
        item->setToolTip(1, tr("Cleared"));
      else
        item->setToolTip(1, tr("Cleared on %1").arg(tierName(cleared)));
    }
  }

  updateTotals();
}

void DrChallenge::updateTotals()
{
  int earned[DR_MINIGAME_SIZE] = {};
  int total[DR_MINIGAME_SIZE] = {};

  for (const auto &entry : m_entries)
  {
    const int type = entry.second->type;
    if (type <= DR_MINIGAME_INVALID || type >= DR_MINIGAME_SIZE)
      continue;
    /* Cap at the five base tiers so an Unfair clear does not inflate the count. */
    earned[type] += qMin(m_cleared.value(recordKey(entry.first, entry.second), 0), 5);
    total[type] += 5;
  }

  QStringList parts;
  int allEarned = 0;
  int allTotal = 0;
  for (int type = DR_MINIGAME_INVALID + 1; type < DR_MINIGAME_SIZE; type++)
  {
    if (total[type] == 0)
      continue;
    parts << QString("%1 %2/%3")
               .arg(QString::fromUtf8(dr_minigame_type_name(static_cast<dr_minigame_type>(type))))
               .arg(earned[type])
               .arg(total[type]);
    allEarned += earned[type];
    allTotal += total[type];
  }
  parts << tr("Total %1/%2").arg(allEarned).arg(allTotal);

  m_totals->setText(parts.join(QStringLiteral("   ")));
}

std::array<dr_player_t, 4> DrChallenge::buildPlayers(
  const dr_mp_minigame_t *minigame, int tier) const
{
  std::array<dr_player_t, 4> players{};
  const bool unfair = (tier == k_TierUnfair);
  const dr_difficulty flat = unfair ? DR_DIFFICULTY_NORMAL : tierDifficulty(tier);
  const dr_character chosen = static_cast<dr_character>(m_character->currentData().toInt());
  auto *rng = QRandomGenerator::global();

  /* The CPUs draw random characters from the roster, minus the player's pick. */
  QList<int> pool;
  for (int c = k_FirstCharacter; c < k_FirstCharacter + k_CharacterCount; c++)
    if (c != static_cast<int>(chosen))
      pool.append(c);
  for (int i = pool.size() - 1; i > 0; i--)
    pool.swapItemsAt(i, static_cast<int>(rng->bounded(i + 1)));

  for (unsigned i = 0; i < 4; i++)
  {
    players[i].character =
      (i == 0) ? chosen : static_cast<dr_character>(pool.value(static_cast<int>(i) - 1));
    players[i].control_port = static_cast<dr_control_port>(DR_CONTROL_PORT_P1 + i);
    players[i].control_type = (i == 0) ? DR_CONTROL_TYPE_HUMAN : DR_CONTROL_TYPE_CPU;
    players[i].difficulty = flat;
    players[i].team_color = DR_TEAM_COLOR_BLUE;
    players[i].team_type = DR_TEAM_TYPE_4P;
    players[i].team_id = i;
  }

  /* The challenger is always player 0. Under Unfair their team-mates are Very
   * Easy and their opponents Very Hard. */
  switch (minigame->type)
  {
  case DR_MINIGAME_2V2:
  {
    const unsigned mate = 1 + rng->bounded(3);
    for (unsigned i = 0; i < 4; i++)
    {
      const bool ours = (i == 0 || i == mate);
      players[i].team_type = DR_TEAM_TYPE_2V2;
      players[i].team_id = ours ? 0 : 1;
      players[i].team_color = ours ? DR_TEAM_COLOR_BLUE : DR_TEAM_COLOR_RED;
      if (unfair && i != 0)
        players[i].difficulty = ours ? DR_DIFFICULTY_VERY_EASY : DR_DIFFICULTY_VERY_HARD;
    }
    break;
  }
  case DR_MINIGAME_1V3:
  {
    /* The challenger is the "1", except under Unfair where they join the trio so
     * they get Very Easy team-mates against a Very Hard solo. */
    const unsigned solo = unfair ? (1 + rng->bounded(3)) : 0;
    for (unsigned i = 0; i < 4; i++)
    {
      const bool isSolo = (i == solo);
      players[i].team_type = isSolo ? DR_TEAM_TYPE_1V3_SOLO : DR_TEAM_TYPE_1V3_GROUP;
      players[i].team_id = isSolo ? 0 : 1;
      players[i].team_color = isSolo ? DR_TEAM_COLOR_RED : DR_TEAM_COLOR_BLUE;
      if (unfair && i != 0)
        players[i].difficulty = isSolo ? DR_DIFFICULTY_VERY_HARD : DR_DIFFICULTY_VERY_EASY;
    }
    break;
  }
  case DR_MINIGAME_DUEL:
  {
    const unsigned rival = 1 + rng->bounded(3);
    for (unsigned i = 0; i < 4; i++)
      players[i].team_type =
        (i == 0 || i == rival) ? DR_TEAM_TYPE_SOLO : DR_TEAM_TYPE_INVALID;
    break;
  }
  default:
    break;
  }

  return players;
}

void DrChallenge::start()
{
  const int tier = m_difficulty->currentData().toInt();
  if (tier < 1)
    return;

  if (m_continuous->isChecked())
  {
    /* The selected tier is fixed for the whole run; each mini is played at its
     * own effective tier (Unfair only sticks for team minis). */
    m_continuousTier = tier;
    m_continuousResult.clear(); /* the first launch shows no result card */
    launchNext(-1);
    return;
  }

  DrGuest *guest = nullptr;
  const dr_mp_minigame_t *minigame = selectedMinigame(&guest);
  if (!minigame || !guest)
    return;

  const int idx = indexOfEntry(guest, minigame);
  if (idx >= 0)
    launchEntry(idx, tier);
}

int DrChallenge::indexOfEntry(const DrGuest *guest, const dr_mp_minigame_t *minigame) const
{
  for (int i = 0; i < m_entries.size(); i++)
    if (m_entries[i].first == guest && m_entries[i].second == minigame)
      return i;
  return -1;
}

void DrChallenge::launchEntry(int idx, int playTier)
{
  if (idx < 0 || idx >= m_entries.size())
    return;

  DrGuest *guest = m_entries[idx].first;
  const dr_mp_minigame_t *minigame = m_entries[idx].second;

  /* Keep the tree in sync with what is being played. */
  for (int i = 0; i < m_tree->topLevelItemCount(); i++)
  {
    QTreeWidgetItem *head = m_tree->topLevelItem(i);
    for (int j = 0; j < head->childCount(); j++)
    {
      QTreeWidgetItem *item = head->child(j);
      if (item->data(0, Qt::UserRole).toInt() == idx)
        m_tree->setCurrentItem(item);
    }
  }

  m_pending.guest = guest;
  m_pending.minigame = minigame;
  m_pending.tier = playTier;
  m_pending.character = currentCharacter();

  emit minigameRequested(guest, minigame, buildPlayers(minigame, playTier));
}

void DrChallenge::launchNext(int excludeIdx)
{
  /* Every entry not yet cleared at its effective tier is a candidate. */
  QList<int> pool;
  for (int i = 0; i < m_entries.size(); i++)
  {
    if (i == excludeIdx)
      continue;
    const DrGuest *guest = m_entries[i].first;
    const dr_mp_minigame_t *minigame = m_entries[i].second;
    if (m_cleared.value(recordKey(guest, minigame), 0) <
        effectiveTier(minigame, m_continuousTier))
      pool.append(i);
  }

  /* If the one just played is the only thing left uncleared, replay it rather
   * than ending early. */
  if (pool.isEmpty() && excludeIdx >= 0 && excludeIdx < m_entries.size())
  {
    const DrGuest *guest = m_entries[excludeIdx].first;
    const dr_mp_minigame_t *minigame = m_entries[excludeIdx].second;
    if (m_cleared.value(recordKey(guest, minigame), 0) <
        effectiveTier(minigame, m_continuousTier))
      pool.append(excludeIdx);
  }

  if (pool.isEmpty()) /* everything cleared -- the run is finished */
  {
    m_continuousResult.clear();
    return;
  }

  const int idx = pool.at(static_cast<int>(QRandomGenerator::global()->bounded(pool.size())));
  launchEntry(idx, effectiveTier(m_entries[idx].second, m_continuousTier));
}

void DrChallenge::recordResult(DrGuest *guest)
{
  if (!isPending())
    return;

  const Pending pending = m_pending;
  m_pending = Pending();

  if (guest != pending.guest)
    return;

  /* The challenger is always player 0. Battle mini-games report a placement
   * (0 = first) instead of a payout, so they are scored the other way round. */
  const dr_minigame_result_t result = guest->minigameResult(0);
  bool won;

  if (pending.minigame->type == DR_MINIGAME_BATTLE)
    won = result.coins == 0;
  else if (pending.minigame->type == DR_MINIGAME_4P)
  {
    /* A free-for-all is a win if the challenger is at least tied for the lead.
     * A tie counts -- unless the whole field scored nothing (nobody won). */
    won = true;
    bool anyScored = result.coins != 0;
    for (unsigned i = 1; i < 4; i++)
    {
      const signed opponent = guest->minigameResult(i).coins;
      if (result.coins < opponent)
        won = false;
      if (opponent != 0)
        anyScored = true;
    }
    won = won && anyScored;
  }
  else
    won = result.coins > 0;

  if (won)
  {
    /* Credit the character that played, which may no longer be the selected one. */
    QHash<QString, int> records = loadFor(pending.character);
    const QString key = recordKey(pending.guest, pending.minigame);
    if (pending.tier > records.value(key, 0))
    {
      records.insert(key, pending.tier);
      saveFor(pending.character, records);
    }
  }

  reload();

  /* Continuous mode chains straight into the next uncleared mini-game (win or
   * lose), avoiding an immediate repeat of the one just played. This leaves a
   * fresh pending challenge, so the caller keeps showing the guest. The result
   * banner is shown on the loading card before the next mini-game. */
  if (m_continuous->isChecked())
  {
    m_continuousResult = won ? tr("SUCCESS") : tr("FAILURE");
    launchNext(indexOfEntry(pending.guest, pending.minigame));
  }
}

dr_character DrChallenge::currentCharacter() const
{
  return static_cast<dr_character>(m_character->currentData().toInt());
}

void DrChallenge::reload()
{
  m_cleared = loadFor(currentCharacter());
  refreshStars();
}

QString DrChallenge::filePathFor(dr_character character)
{
  /* e.g. "Donkey Kong" -> save/challenge-donkeykong.json */
  QString slug;
  for (const QChar &ch : QString::fromUtf8(dr_character_name(character)).toLower())
    if (ch.isLetterOrNumber())
      slug += ch;
  return dr_save_directory() + "/challenge-" + slug + ".json";
}

QString DrChallenge::recordKey(const DrGuest *guest, const dr_mp_minigame_t *minigame)
{
  return QString("%1::%2")
    .arg(QString::fromUtf8(guest->name()), QString::fromUtf8(minigame->name));
}

QHash<QString, int> DrChallenge::loadFor(dr_character character)
{
  QHash<QString, int> records;

  QFile file(filePathFor(character));
  if (!file.open(QIODevice::ReadOnly))
    return records;

  const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
  for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
    records.insert(it.key(), it.value().toInt());

  return records;
}

void DrChallenge::saveFor(dr_character character, const QHash<QString, int> &records)
{
  QJsonObject obj;
  for (auto it = records.constBegin(); it != records.constEnd(); ++it)
    obj.insert(it.key(), it.value());

  QDir().mkpath(dr_save_directory());
  QFile file(filePathFor(character));
  if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}
