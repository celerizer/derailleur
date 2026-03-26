#include "DrOverlay.h"

#include <QPainter>

DrOverlay::DrOverlay(QWidget *parent) : QWidget(parent)
{
  setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_TransparentForMouseEvents);
  setMask(QRegion());
}

void DrOverlay::setImage(const QImage &image)
{
  m_image = image;
  update();
}

void DrOverlay::flash(const QPixmap &pixmap, int durationMs)
{
  m_image = pixmap.toImage();
  setWindowOpacity(1.0);
  repaint();

  if (m_anim) {
    m_anim->stop();
    m_anim->deleteLater();
  }

  m_anim = new QVariantAnimation(this);
  m_anim->setStartValue(1.0);
  m_anim->setEndValue(0.0);
  m_anim->setDuration(durationMs);
  connect(m_anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
    setWindowOpacity(v.toReal());
  });
  m_anim->start(QAbstractAnimation::DeleteWhenStopped);
  m_anim = nullptr;
}

void DrOverlay::trackGeometry(const QRect &rect)
{
  setGeometry(rect);
}

void DrOverlay::paintEvent(QPaintEvent *)
{
  if (m_image.isNull())
    return;
  QPainter p(this);
  p.drawImage(rect(), m_image);
}
