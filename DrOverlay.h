#ifndef DR_OVERLAY_H
#define DR_OVERLAY_H

#include <QImage>
#include <QPixmap>
#include <QTimer>
#include <QVariantAnimation>
#include <QWidget>

class DrOverlay : public QWidget
{
  Q_OBJECT

public:
  DrOverlay(QWidget *parent = nullptr);

  void setImage(const QImage &image);
  void flash(const QPixmap &pixmap, int durationMs = 500);
  void hold(const QPixmap &pixmap);
  /// Holds the loading.png background with centered white text: the just-finished
  /// result over an "Up next..." block naming the coming game and mini-game. Used
  /// between continuous-play challenge mini-games.
  void showLoadingCard(const QString &result, const QString &game, const QString &minigame);
  void fadeOut(int durationMs = 500);
  void trackGeometry(const QRect &rect);
  void setSprite(const QPixmap &sprite);

protected:
  void paintEvent(QPaintEvent *) override;

private:
  void setActive(bool active);
  void pickRandomSprite();

  QImage m_image;
  QVariantAnimation *m_anim = nullptr;
  QPixmap m_sprite;
  int m_spriteY = 0;
  QTimer *m_bounceTimer = nullptr;
  QTimer *m_spriteDelay = nullptr;

  /* When set, paintEvent draws the "Up next..." text block over m_image. */
  bool m_cardMode = false;
  QString m_cardResult;
  QString m_cardGame;
  QString m_cardMinigame;
};

#endif
