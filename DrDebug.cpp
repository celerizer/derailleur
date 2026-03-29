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

  layout->addStretch();
  setLayout(layout);
}

void DrDebug::populate(const QList<DrGuest*> &guests)
{
  m_combo->clear();
  m_entries.clear();

  for (DrGuest *guest : guests) {
    for (const dr_mp_minigame_t *mg = guest->minigames(); mg->name; mg++) {
      m_combo->addItem(QString("%1 \xe2\x80\x94 %2").arg(guest->name()).arg(mg->name));
      m_entries.append({guest, mg});
    }
  }
}
