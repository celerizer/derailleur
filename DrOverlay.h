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
  void fadeOut(int durationMs = 500);
  void trackGeometry(const QRect &rect);
  void setSprite(const QPixmap &sprite);

protected:
  void paintEvent(QPaintEvent *) override;

private:
  void pickRandomSprite();

  QImage m_image;
  QVariantAnimation *m_anim = nullptr;
  QPixmap m_sprite;
  int m_spriteY = 0;
  QTimer *m_bounceTimer = nullptr;
};

#endif
