#include "DrSettings.h"

#include "DrCommon.h"

#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>

DrSettings::DrSettings(QWidget *parent)
  : QWidget(parent)
{
  QVBoxLayout *layout = new QVBoxLayout(this);

  QCheckBox *separateGcn =
    new QCheckBox(tr("Use separate Dolphin instances for each GameCube game"), this);
  separateGcn->setChecked(dr_settings_get().separate_gamecube_instances);
  separateGcn->setToolTip(
    tr("Give each GameCube game its own Dolphin core instead of sharing one and\n"
       "swapping discs. Uses more memory, but avoids the disc-swap load path."));
  connect(separateGcn, &QCheckBox::toggled, this, [](bool on) {
    dr_settings_get().separate_gamecube_instances = on;
    dr_settings_save();
  });
  layout->addWidget(separateGcn);

  QLabel *note = new QLabel(
    tr("Requires restart of the program. Will noticeably raise memory usage, so please use "
       "only if GameCube mini-games do not work at all on your machine."),
    this);
  note->setWordWrap(true);
  note->setEnabled(false); /* muted */
  layout->addWidget(note);

  layout->addStretch();
}
