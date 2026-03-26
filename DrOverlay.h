#ifndef DR_OVERLAY_H
#define DR_OVERLAY_H

#include <QImage>
#include <QPixmap>
#include <QVariantAnimation>
#include <QWidget>

class DrOverlay : public QWidget
{
  Q_OBJECT

public:
  DrOverlay(QWidget *parent = nullptr);

  void setImage(const QImage &image);
  void flash(const QPixmap &pixmap, int durationMs = 500);
  void trackGeometry(const QRect &rect);

protected:
  void paintEvent(QPaintEvent *) override;

private:
  QImage             m_image;
  QVariantAnimation *m_anim = nullptr;
};

#endif
