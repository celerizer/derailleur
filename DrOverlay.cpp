#include "DrOverlay.h"

#include <cmath>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QRandomGenerator>
#include <QTimer>

#include "DrCommon.h"

/* Delay before the loading icon appears, so loads that finish quickly don't flash
 * it on screen. The frozen-frame background still shows immediately. */
#define DR_OVERLAY_ICON_DELAY_MS 400

DrOverlay::DrOverlay(QWidget *parent)
  : QWidget(parent)
{
  setMask(QRegion());
}

void DrOverlay::setActive(bool active)
{
  if (active)
  {
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    show();
  }
  else
  {
    m_image = QImage();
    m_cardMode = false;
    setWindowFlags(Qt::Widget);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
  }
}

void DrOverlay::setImage(const QImage &image)
{
  m_image = image;
  update();
}

void DrOverlay::flash(const QPixmap &pixmap, int durationMs)
{
  if (m_spriteDelay)
    m_spriteDelay->stop();
  if (m_bounceTimer)
    m_bounceTimer->stop();

  m_cardMode = false;
  m_image = pixmap.toImage();
  setActive(true);
  setWindowOpacity(1.0);
  repaint();

  if (m_anim)
  {
    m_anim->stop();
    m_anim->deleteLater();
  }

  m_anim = new QVariantAnimation(this);
  m_anim->setStartValue(1.0);
  m_anim->setEndValue(0.0);
  m_anim->setDuration(durationMs);
  connect(m_anim, &QVariantAnimation::valueChanged, this,
    [this](const QVariant &v) { setWindowOpacity(v.toReal()); });
  connect(m_anim, &QVariantAnimation::finished, this, [this]() { setActive(false); });
  m_anim->start(QAbstractAnimation::DeleteWhenStopped);
  m_anim = nullptr;

  if (m_bounceTimer)
    m_bounceTimer->start();
}

void DrOverlay::hold(const QPixmap &pixmap)
{
  m_cardMode = false;
  m_image = pixmap.toImage();
  if (m_anim)
  {
    m_anim->stop();
    m_anim->deleteLater();
    m_anim = nullptr;
  }

  /* Show the frozen frame right away to cover the core swap, but hold the loading
   * icon back for a moment: a load that finishes before the delay fires never
   * flashes an icon (fadeOut() cancels the pending reveal). */
  m_sprite = QPixmap();
  if (m_bounceTimer)
    m_bounceTimer->stop();
  setActive(true);
  setWindowOpacity(1.0);
  repaint();

  if (!m_spriteDelay)
  {
    m_spriteDelay = new QTimer(this);
    m_spriteDelay->setSingleShot(true);
    connect(m_spriteDelay, &QTimer::timeout, this, [this]() {
      pickRandomSprite();
      if (m_bounceTimer)
        m_bounceTimer->start();
      repaint();
    });
  }
  m_spriteDelay->start(DR_OVERLAY_ICON_DELAY_MS);
}

void DrOverlay::showLoadingCard(
  const QString &result, const QString &game, const QString &minigame)
{
  if (m_anim)
  {
    m_anim->stop();
    m_anim->deleteLater();
    m_anim = nullptr;
  }

  m_image = QImage(":/assets/loading.png");
  m_cardMode = true;
  m_cardResult = result;
  m_cardGame = game;
  m_cardMinigame = minigame;

  /* Same bouncing loading sprite as hold(): cleared now, revealed after the delay
   * and drawn on top of the card (see paintEvent). */
  m_sprite = QPixmap();
  if (m_bounceTimer)
    m_bounceTimer->stop();
  setActive(true);
  setWindowOpacity(1.0);
  repaint();

  if (!m_spriteDelay)
  {
    m_spriteDelay = new QTimer(this);
    m_spriteDelay->setSingleShot(true);
    connect(m_spriteDelay, &QTimer::timeout, this, [this]() {
      pickRandomSprite();
      if (m_bounceTimer)
        m_bounceTimer->start();
      repaint();
    });
  }
  m_spriteDelay->start(DR_OVERLAY_ICON_DELAY_MS);
}

void DrOverlay::fadeOut(int durationMs)
{
  /* Cancel a not-yet-revealed icon so a quick load never flashes one. */
  if (m_spriteDelay)
    m_spriteDelay->stop();

  if (m_bounceTimer)
    m_bounceTimer->stop();

  if (m_anim)
  {
    m_anim->stop();
    m_anim->deleteLater();
  }

  m_anim = new QVariantAnimation(this);
  m_anim->setStartValue(1.0);
  m_anim->setEndValue(0.0);
  m_anim->setDuration(durationMs);
  connect(m_anim, &QVariantAnimation::valueChanged, this,
    [this](const QVariant &v) { setWindowOpacity(v.toReal()); });
  connect(m_anim, &QVariantAnimation::finished, this, [this]() { setActive(false); });
  m_anim->start(QAbstractAnimation::DeleteWhenStopped);
  m_anim = nullptr;
}

#define DR_OVERLAY_SPRITE_COUNT 36

void DrOverlay::pickRandomSprite()
{
  int index = QRandomGenerator::global()->bounded(DR_OVERLAY_SPRITE_COUNT);
  QPixmap sprite(QString(":/assets/char/%1.png").arg(index));
  if (!sprite.isNull())
    setSprite(sprite.scaled(
      sprite.width() * 4, sprite.height() * 4, Qt::IgnoreAspectRatio, Qt::FastTransformation));
}

void DrOverlay::setSprite(const QPixmap &sprite)
{
  m_sprite = sprite;

  if (m_bounceTimer)
  {
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

  if (m_cardMode)
  {
    p.setRenderHint(QPainter::TextAntialiasing, true);

    QFont big = p.font();
    big.setPixelSize(qMax(28, height() / 9));
    big.setBold(true);
    QFont mid = p.font();
    mid.setPixelSize(qMax(18, height() / 18));
    QFont small = p.font();
    small.setPixelSize(qMax(15, height() / 24));

    /* result / "Up next..." / game / mini-game, stacked and centered. Each line
     * is drawn with a soft shadow so white stays legible over any background. */
    struct Line
    {
      QString text;
      QFont font;
      int gapAbove;
    };
    const Line lines[] = {
      { m_cardResult, big, 0 },
      { QStringLiteral("Up next..."), mid, height() / 12 },
      { m_cardGame, mid, height() / 40 },
      { m_cardMinigame, small, height() / 80 },
    };
    const int count = static_cast<int>(sizeof(lines) / sizeof(lines[0]));

    int total = 0;
    for (int i = 0; i < count; i++)
      total += lines[i].gapAbove + QFontMetrics(lines[i].font).height();

    int y = (height() - total) / 2;
    for (int i = 0; i < count; i++)
    {
      y += lines[i].gapAbove;
      const int lh = QFontMetrics(lines[i].font).height();
      const QRect r(0, y, width(), lh);
      p.setFont(lines[i].font);
      p.setPen(QColor(0, 0, 0, 160));
      p.drawText(r.translated(2, 2), Qt::AlignHCenter | Qt::AlignVCenter, lines[i].text);
      p.setPen(Qt::white);
      p.drawText(r, Qt::AlignHCenter | Qt::AlignVCenter, lines[i].text);
      y += lh;
    }
  }

  /* Drawn last so the bouncing loading sprite sits on top of the card. */
  if (!m_sprite.isNull())
  {
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);
    p.drawPixmap(
      width() - m_sprite.width() - 32, height() - m_sprite.height() - 32 + m_spriteY, m_sprite);
  }
}
