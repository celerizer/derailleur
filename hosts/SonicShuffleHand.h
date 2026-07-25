#ifndef DR_SONIC_SHUFFLE_HAND_H
#define DR_SONIC_SHUFFLE_HAND_H

#include <QByteArray>
#include <QWidget>

/// Floating panel showing the local player's Sonic Shuffle hand: their 72x72
/// character portrait in the top-left over a character-themed vertical gradient,
/// with their cards laid out in order below as 48x72 images.
///
/// Fed from SonicShuffleHost::localHandChanged (emitted on the core's timing
/// thread, delivered here queued), so setHand is safe to call cross-thread.
class SonicShuffleHand : public QWidget
{
  Q_OBJECT

public:
  explicit SonicShuffleHand(QWidget *parent = nullptr);

  QSize sizeHint(void) const override;

public slots:
  /// character: Sonic Shuffle character id (0 invalid, 1 Sonic, 2 Tails, ...).
  /// cards: one byte per card value, already in hand order.
  /// highlight: index of the card to glow (the one being selected), or -1.
  void setHand(int character, const QByteArray &cards, int highlight);

protected:
  void paintEvent(QPaintEvent *) override;

private:
  int m_character = 0;
  QByteArray m_cards;
  int m_highlight = -1;
};

#endif
