#include "SonicShuffleHand.h"

#include <QLinearGradient>
#include <QPainter>
#include <QPixmap>

/* Layout metrics (pixels). */
#define SSH_MARGIN    8
#define SSH_CHAR_SIZE 72
#define SSH_CARD_W    48
#define SSH_CARD_H    72
#define SSH_CARD_GAP  4
#define SSH_MAX_CARDS 7

/* Per-character background gradient (top -> bottom); see the guest's ss_player_t
 * character enum (1 Sonic .. 8 Super Sonic). Anything else is "no character". */
struct CharTheme
{
  QColor top;
  QColor bottom;
};

static CharTheme characterTheme(int character)
{
  switch (character)
  {
  case 1: return { QColor(0x00, 0x60, 0xff), QColor(0x00, 0x30, 0x7f) }; /* Sonic       */
  case 2: return { QColor(0xff, 0xcd, 0x00), QColor(0x7f, 0x66, 0x00) }; /* Tails       */
  case 3: return { QColor(0xff, 0x5e, 0x34), QColor(0x7f, 0x2f, 0x1a) }; /* Knuckles    */
  case 4: return { QColor(0xff, 0x66, 0x66), QColor(0x7f, 0x33, 0x33) }; /* Amy         */
  case 5: return { QColor(0x78, 0x62, 0xf1), QColor(0x3c, 0x31, 0x78) }; /* Big         */
  case 6: return { QColor(0x83, 0x5c, 0x5c), QColor(0x41, 0x2e, 0x2e) }; /* Gamma       */
  case 7: return { QColor(0x98, 0xd9, 0xee), QColor(0x4c, 0x6c, 0x77) }; /* Chao        */
  case 8: return { QColor(0xef, 0xef, 0x00), QColor(0x77, 0x77, 0x00) }; /* Super Sonic */
  default: return { QColor(0x50, 0x55, 0x5c), QColor(0x28, 0x2a, 0x2e) }; /* invalid    */
  }
}

SonicShuffleHand::SonicShuffleHand(QWidget *parent)
  : QWidget(parent)
{
  setWindowTitle(tr("Your Hand"));
  setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint);
  /* Showing the panel shouldn't steal focus from the game window. */
  setAttribute(Qt::WA_ShowWithoutActivating);
}

QSize SonicShuffleHand::sizeHint(void) const
{
  const int w = SSH_MARGIN * 2 + SSH_MAX_CARDS * SSH_CARD_W + (SSH_MAX_CARDS - 1) * SSH_CARD_GAP;
  const int h = SSH_MARGIN * 3 + SSH_CHAR_SIZE + SSH_CARD_H;
  return QSize(w, h);
}

void SonicShuffleHand::setHand(int character, const QByteArray &cards, int highlight)
{
  m_character = character;
  m_cards = cards;
  m_highlight = highlight;
  update();
}

void SonicShuffleHand::paintEvent(QPaintEvent *)
{
  QPainter p(this);
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);

  /* Character-themed vertical gradient (top -> bottom). */
  const CharTheme theme = characterTheme(m_character);
  QLinearGradient grad(0, 0, 0, height());
  grad.setColorAt(0.0, theme.top);
  grad.setColorAt(1.0, theme.bottom);
  p.fillRect(rect(), grad);

  /* Glow colour is the opposite of the background so it always stands out. */
  const QColor glow(255 - theme.top.red(), 255 - theme.top.green(), 255 - theme.top.blue());

  /* Character portrait, top-left. */
  {
    const QPixmap portrait(QString(":/assets/sonicshuffle/char/%1.png").arg(m_character));
    if (!portrait.isNull())
      p.drawPixmap(SSH_MARGIN, SSH_MARGIN, SSH_CHAR_SIZE, SSH_CHAR_SIZE, portrait);
  }

  /* Cards row beneath the portrait, in hand order. */
  const int y = SSH_MARGIN * 2 + SSH_CHAR_SIZE;
  const int stride = SSH_CARD_W + SSH_CARD_GAP;

  /* Glow behind the selected card (drawn first so every card sits on top of it):
   * a few expanding rounded outlines, thinner and stronger near the card, so it
   * reads as an aura once the card covers the inner half. */
  if (m_highlight >= 0 && m_highlight < m_cards.size())
  {
    const QRect slot(SSH_MARGIN + m_highlight * stride, y, SSH_CARD_W, SSH_CARD_H);
    p.setBrush(Qt::NoBrush);
    for (int g = 3; g >= 1; g--)
    {
      QColor c = glow;
      c.setAlpha(255 - (g - 1) * 55); /* inner ring near-solid, outer fainter */
      p.setPen(QPen(c, g));           /* thin: 1px inner .. 3px outer */
      p.drawRoundedRect(slot.adjusted(-g, -g, g, g), 5, 5);
    }
  }

  for (int i = 0; i < m_cards.size(); i++)
  {
    const int value = static_cast<uint8_t>(m_cards.at(i));
    const QPixmap card(QString(":/assets/sonicshuffle/card/%1.png").arg(value));
    if (!card.isNull())
      p.drawPixmap(SSH_MARGIN + i * stride, y, SSH_CARD_W, SSH_CARD_H, card);
  }
}
