#include "DrSettings.h"

#include "DrCommon.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

/* Builds a checkbox bound to one bool in dr_settings: reads the current value,
 * and on toggle writes `member` back through and persists. */
static QCheckBox *settings_checkbox(QWidget *parent, const QString &text,
  const QString &tooltip, bool dr_settings::*member)
{
  QCheckBox *box = new QCheckBox(text, parent);

  box->setChecked(dr_settings_get().*member);
  box->setToolTip(tooltip);
  QObject::connect(box, &QCheckBox::toggled, box, [member](bool on) {
    dr_settings_get().*member = on;
    dr_settings_save();
  });

  return box;
}

/* Builds a resolution dropdown bound to one unsigned in dr_settings: entry N is
 * labelled for N times `native_height`, and writes through on change. */
static QComboBox *settings_scale_combo(QWidget *parent, unsigned native_height,
  unsigned max_scale, unsigned recommended, unsigned dr_settings::*member)
{
  QComboBox *combo = new QComboBox(parent);
  const unsigned current = dr_settings_get().*member;

  for (unsigned n = 1; n <= max_scale; n++)
  {
    QString label = QObject::tr("%1x for %2p").arg(n).arg(n * native_height);

    if (n == recommended)
      label += QObject::tr(" (recommended)");
    combo->addItem(label, n);
  }

  /* A value from a newer build, or a hand-edited ini, would not be in the list. */
  combo->setCurrentIndex(
    (current >= 1 && current <= max_scale) ? static_cast<int>(current) - 1
                                           : static_cast<int>(recommended) - 1);
  QObject::connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), combo,
    [member, combo](int index) {
      dr_settings_get().*member = combo->itemData(index).toUInt();
      dr_settings_save();
    });

  return combo;
}

/* A muted, wrapped caption under the control it explains. */
static QLabel *settings_note(QWidget *parent, const QString &text)
{
  QLabel *note = new QLabel(text, parent);

  note->setWordWrap(true);
  note->setEnabled(false); /* muted */

  return note;
}

DrSettings::DrSettings(QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);

  QGroupBox *system = new QGroupBox(tr("System"), this);
  QVBoxLayout *systemLayout = new QVBoxLayout(system);

  systemLayout->addWidget(settings_checkbox(system,
    tr("Shared Dolphin core for GameCube games"),
    tr("Share one Dolphin core across the GameCube games, swapping discs between them,\n"
       "instead of giving each its own. Uses less memory but relies on the disc-swap\n"
       "load path."),
    &dr_settings::shared_gamecube_core));
  systemLayout->addWidget(settings_note(system,
    tr("Requires restart of the program. Only enable this if your system is running out "
       "of memory.")));
  layout->addWidget(system);

  QGroupBox *graphics = new QGroupBox(tr("Graphics"), this);
  QVBoxLayout *graphicsLayout = new QVBoxLayout(graphics);

  graphicsLayout->addWidget(settings_checkbox(graphics, tr("Widescreen hack"),
    tr("Render every game at 16:9 instead of the 4:3 they were made for.\n"
       "Stretches some HUDs and lets objects pop in at the edges of the screen."),
    &dr_settings::widescreen_hack));
  graphicsLayout->addWidget(settings_note(graphics,
    tr("Requires restart of the program. Play Nintendo 64 and GameCube games in simulated widescreen.")));

  /* Native heights: N64 240p, GameCube/Wii 480p, DS 192p per screen. */
  QGroupBox *resolution = new QGroupBox(tr("Internal resolution"), graphics);
  QVBoxLayout *resolutionLayout = new QVBoxLayout(resolution);
  QFormLayout *resolutions = new QFormLayout();

  resolutions->addRow(tr("Nintendo 64"),
    settings_scale_combo(resolution, 240, 8, 2, &dr_settings::res_scale_n64));
  resolutions->addRow(tr("GameCube / Wii"),
    settings_scale_combo(resolution, 480, 6, 1, &dr_settings::res_scale_gcn));
  resolutions->addRow(tr("Nintendo DS"),
    settings_scale_combo(resolution, 192, 8, 2, &dr_settings::res_scale_ds));
  resolutionLayout->addLayout(resolutions);
  resolutionLayout->addWidget(settings_note(resolution,
    tr("Requires restart of the program. Higher costs GPU time, and a few mini-games "
       "force their own resolution to render correctly.")));
  graphicsLayout->addWidget(resolution);
  layout->addWidget(graphics);

  QGroupBox *game = new QGroupBox(tr("Game"), this);
  QVBoxLayout *gameLayout = new QVBoxLayout(game);

  gameLayout->addWidget(settings_checkbox(game, tr("BETA: Waluigi and Daisy in Mario Party 1 and 2"),
    tr("Play as injected characters in Mario Party and Mario Party 2 Mini-Games. Expect issues!"),
    &dr_settings::character_injection));
  gameLayout->addWidget(settings_note(
    game, tr("Play as injected characters in Mario Party and Mario Party 2 Mini-Games. Expect issues!")));
  layout->addWidget(game);

  QGroupBox *debug = new QGroupBox(tr("Debug"), this);
  QVBoxLayout *debugLayout = new QVBoxLayout(debug);

  debugLayout->addWidget(settings_checkbox(debug, tr("Show the loading overlay"),
    tr("Cover the switch between games with the last frame and a loading icon.\n"
       "Turn this off to see the cores swapping underneath."),
    &dr_settings::loading_overlay));
  debugLayout->addWidget(settings_checkbox(debug, tr("Mute game while loading"),
    tr("Silence a mini-game's core until the mini-game itself starts.\n"
       "Turn this off to hear it boot underneath the loading overlay."),
    &dr_settings::mute_while_loading));
  layout->addWidget(debug);

  QPushButton *redownloadSaves = new QPushButton(tr("Re-download saves"), this);
  redownloadSaves->setToolTip(
    tr("Fetch the latest save files from the server, overwriting your local ones.\n"
       "Saves are otherwise downloaded only on the first launch."));
  connect(redownloadSaves, &QPushButton::clicked, this,
    [this]() { emit redownloadSavesRequested(); });
  layout->addWidget(redownloadSaves);

  layout->addStretch();
}
