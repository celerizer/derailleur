#ifndef DR_SETTINGS_H
#define DR_SETTINGS_H

#include <QWidget>

/**
 * The "Settings" tool page: user-facing options backed by the global dr_settings
 * (see DrCommon). Toggling a control updates the struct and persists it to
 * derailleur.ini immediately. Options that only apply at startup are labelled as
 * needing a restart.
 */
class DrSettings : public QWidget
{
  Q_OBJECT

public:
  explicit DrSettings(QWidget *parent = nullptr);
};

#endif
