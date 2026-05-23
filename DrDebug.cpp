#include "DrDebug.h"

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
    if (idx >= 0 && idx < m_entries.size())
      emit minigameRequested(m_entries[idx].first, m_entries[idx].second);
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
