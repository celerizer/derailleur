#include "DrSettings.h"

#include "DrCommon.h"

#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DrSettings::DrSettings(QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);

  QCheckBox *sharedGcn =
    new QCheckBox(tr("Shared Dolphin core for GameCube games"), this);
  sharedGcn->setChecked(dr_settings_get().shared_gamecube_core);
  sharedGcn->setToolTip(
    tr("Share one Dolphin core across the GameCube games, swapping discs between them,\n"
       "instead of giving each its own. Uses less memory but relies on the disc-swap\n"
       "load path."));
  connect(sharedGcn, &QCheckBox::toggled, this, [](bool on) {
    dr_settings_get().shared_gamecube_core = on;
    dr_settings_save();
  });
  layout->addWidget(sharedGcn);

  QLabel *note = new QLabel(
    tr("Requires restart of the program. Only enable this if your system is running out "
       "of memory."),
    this);
  note->setWordWrap(true);
  note->setEnabled(false); /* muted */
  layout->addWidget(note);

  QPushButton *redownloadSaves = new QPushButton(tr("Re-download saves"), this);
  redownloadSaves->setToolTip(
    tr("Fetch the latest save files from the server, overwriting your local ones.\n"
       "Saves are otherwise downloaded only on the first launch."));
  connect(redownloadSaves, &QPushButton::clicked, this,
    [this]() { emit redownloadSavesRequested(); });
  layout->addWidget(redownloadSaves);

  layout->addStretch();
}
