#ifndef DR_DEBUG_CORE_H
#define DR_DEBUG_CORE_H

#include <QObject>
#include <QPointer>
#include <QString>

class CleMemoryInspector;
class DrLogger;
class QRetro;
class QTimer;

/**
 * The debug page's developer core: any core + content booted in its own window,
 * always paired with a Classics Live memory inspector over its memory. Only one
 * exists at a time, and closing either window closes both. It runs outside
 * netplay, and saves SRAM like any QRetro (to <cwd>/save by default).
 */
class DrDebugCore : public QObject
{
  Q_OBJECT

public:
  explicit DrDebugCore(DrLogger *logger, QObject *parent = nullptr);
  ~DrDebugCore() override;

  /// Boots `corePath` with `contentPath` (may be empty), replacing any debug
  /// core already running, and opens the inspector on it.
  bool launch(const QString &corePath, const QString &contentPath);

  /// Closes the core and its inspector.
  void close(void);

private:
  /// Opens the inspector window on the current region table.
  void createInspector(void);
  void destroyInspector(void);

  /// Rebuilds CL's region table from the core's memory maps (or its system RAM).
  void installRegions(void);

  /// A cheap identity for the core's current memory layout, to spot it changing.
  quintptr layoutKey(void);

  /// Validates the core, then refreshes the inspector. Drives the inspector in
  /// place of its own timer so it never reads a core that has just unloaded.
  /// The inspector only exists once the core has loaded and its regions are in.
  void tick(void);

  DrLogger *m_Logger = nullptr;
  QPointer<QRetro> m_Core;
  QPointer<CleMemoryInspector> m_Inspector;
  QTimer *m_Timer = nullptr;
  quintptr m_LayoutKey = 0;
  /* "core: content", for window titles */
  QString m_Label;
  /* An N64 core: its memory is tagged word-flipped (CL_ENDIAN_WORD_FLIP_BL) */
  bool m_WordFlip = false;
};

#endif
