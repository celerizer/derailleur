#include "DrOverlay.h"

#include <cmath>
#include <QPainter>
#include <QRandomGenerator>
#include <QTimer>

DrOverlay::DrOverlay(QWidget *parent) : QWidget(parent)
{
  setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
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
  if (m_bounceTimer)
    m_bounceTimer->stop();

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
  m_anim = nullptr; // DeleteWhenStopped handles lifetime

  if (m_bounceTimer)
    m_bounceTimer->start();
}

void DrOverlay::hold(const QPixmap &pixmap)
{
  m_image = pixmap.toImage();
  if (m_anim) {
    m_anim->stop();
    m_anim->deleteLater();
    m_anim = nullptr;
  }
  pickRandomSprite();
  if (m_bounceTimer)
    m_bounceTimer->start();
  setWindowOpacity(1.0);
  repaint();
}

void DrOverlay::fadeOut(int durationMs)
{
  if (m_bounceTimer)
    m_bounceTimer->stop();

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

#define DR_OVERLAY_SPRITE_COUNT 2

void DrOverlay::pickRandomSprite()
{
  int index = QRandomGenerator::global()->bounded(DR_OVERLAY_SPRITE_COUNT);
  QPixmap sprite(QString(":/assets/char/%1.png").arg(index));
  if (!sprite.isNull())
    setSprite(sprite.scaled(sprite.width() * 4, sprite.height() * 4,
                            Qt::IgnoreAspectRatio, Qt::FastTransformation));
}

void DrOverlay::setSprite(const QPixmap &sprite)
{
  m_sprite = sprite;

  if (m_bounceTimer) {
    m_bounceTimer->stop();
    m_bounceTimer->deleteLater();
  }
  m_bounceTimer = new QTimer(this);
  m_bounceTimer->setInterval(500);
  connect(m_bounceTimer, &QTimer::timeout, this, [this]() {
    QVariantAnimation *anim = new QVariantAnimation(this);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->setDuration(300);
    connect(anim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
      m_spriteY = static_cast<int>(-16.0 * std::sin(v.toDouble() * M_PI));
      update();
    });
    anim->start(QAbstractAnimation::DeleteWhenStopped);
  });
  m_bounceTimer->start();
  update();
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
  if (!m_sprite.isNull()) {
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawPixmap(width()  - m_sprite.width()  - 32,
                 height() - m_sprite.height() - 32 + m_spriteY,
                 m_sprite);
  }
}
