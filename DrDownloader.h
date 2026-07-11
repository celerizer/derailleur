#ifndef DR_DOWNLOADER_H
#define DR_DOWNLOADER_H

#include <QByteArray>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QSettings;

class DrDownloader : public QObject
{
  Q_OBJECT

public:
  explicit DrDownloader(QObject *parent = nullptr);

  /// Runs the save (always) and state (conditional) downloads, blocking on a
  /// local event loop. Progress is reported via progressStarted/progressFinished
  /// so a host (e.g. the log window) can show a bar. Config and the persisted
  /// state validator live in `settings` under the [downloads] section.
  void runBlocking(QSettings &settings, const QString &saveDir, const QString &stateDir);

signals:
  /// Diagnostic log; level matches DrLogger::message (DR_LOG_*).
  void logMessage(unsigned level, const QString &msg);

  /// A download phase began (with a human label); progressFinished() ends it.
  void progressStarted(const QString &label);
  void progressFinished();

  void progressUpdated(qint64 received, qint64 total);

private:
  /// One HTTP validator pair; either field may be empty if the server omits it.
  struct Validator
  {
    QByteArray etag;
    QByteArray lastModified;
  };

  /// GETs `url` (conditional if `have` is non-empty), and on 200 extracts the
  /// zip, routing its entries to `saveDir`/`stateDir`/`<cwd>/system`. Returns
  /// true on success or 304 (nothing to do); sets `notModified` on 304 and fills
  /// `got` with the response validator on 200.
  bool fetchAndExtract(QNetworkAccessManager &nam, const QString &url, const QString &saveDir,
    const QString &stateDir, const Validator &have, Validator &got, bool &notModified,
    int timeoutMs);

  /// Extracts a zip held in `data`, sanitizing entry paths. A leading
  /// "derailleur-" wrapper folder is stripped, then the first path component
  /// routes the destination: save -> saveDir, state -> stateDir, everything else
  /// -> the current working directory (preserving its path).
  bool extractZip(const QByteArray &data, const QString &saveDir, const QString &stateDir);
};

#endif
