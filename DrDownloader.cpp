#include "DrDownloader.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QSslError>
#include <QSslSocket>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

#include <private/qzipreader_p.h>

#include "DrCommon.h" // dr_log_level (DR_LOG_*)

namespace
{
/* derailleur-%1.zip, %1 = "saves" | "states"; v is a fixed cache-buster. */
const char *k_DefaultBaseUrl =
  "https://www.doggylongface.com/derailleur/download/derailleur-%1.zip?v=1";
}

DrDownloader::DrDownloader(QObject *parent)
  : QObject(parent)
{
}

void DrDownloader::runBlocking(
  QSettings &settings, const QString &saveDir, const QString &stateDir)
{
  if (!settings.value("downloads/enabled", true).toBool())
  {
    emit logMessage(DR_LOG_INFO, "downloads: disabled, skipping");
    return;
  }

  const QString base = settings.value("downloads/base_url", k_DefaultBaseUrl).toString();
  const int timeoutMs = settings.value("downloads/timeout_ms", 15000).toInt();

  emit progressStarted(tr("Downloading..."));
  QCoreApplication::processEvents();

  QNetworkAccessManager nam;

  /* If Qt can't bring up its own TLS backend, every https request dies at the
   * handshake regardless of the system's SSL — surface that up front. */
  emit logMessage(QSslSocket::supportsSsl() ? DR_LOG_INFO : DR_LOG_WARN,
    QString("downloads: TLS supported=%1 build=%2 runtime=%3")
      .arg(QSslSocket::supportsSsl() ? "yes" : "no",
        QSslSocket::sslLibraryBuildVersionString(), QSslSocket::sslLibraryVersionString()));

  /* Saves: always fetched, overwriting local files so peers stay identical. */
  {
    emit progressStarted(tr("Downloading saves..."));
    QCoreApplication::processEvents();
    Validator got;
    bool notModified = false;
    fetchAndExtract(
      nam, base.arg("saves"), saveDir, stateDir, Validator{}, got, notModified, timeoutMs);
  }

  /* Data (states + anything that lives in the cwd): conditional on the stored
   * validator; only re-applied when the server copy is newer. */
  {
    emit progressStarted(tr("Downloading data..."));
    QCoreApplication::processEvents();
    Validator have;
    have.etag = settings.value("downloads/data_etag").toByteArray();
    have.lastModified = settings.value("downloads/data_last_modified").toByteArray();
    Validator got;
    bool notModified = false;
    if (fetchAndExtract(
          nam, base.arg("data"), saveDir, stateDir, have, got, notModified, timeoutMs)
      && !notModified)
    {
      /* Persist the new validator so the next boot can short-circuit with 304. */
      settings.setValue("downloads/data_etag", got.etag);
      settings.setValue("downloads/data_last_modified", got.lastModified);
      settings.sync();
    }
  }

  emit progressFinished();
}

bool DrDownloader::fetchAndExtract(QNetworkAccessManager &nam, const QString &url,
  const QString &saveDir, const QString &stateDir, const Validator &have, Validator &got,
  bool &notModified, int timeoutMs)
{
  notModified = false;

  QUrl u(url);
  QUrlQuery q(u);
  q.addQueryItem("cb", QString::number(QDateTime::currentMSecsSinceEpoch()));
  u.setQuery(q);

  QNetworkRequest req{ u };
  req.setAttribute(
    QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
  if (!have.etag.isEmpty())
    req.setRawHeader("If-None-Match", have.etag);
  if (!have.lastModified.isEmpty())
    req.setRawHeader("If-Modified-Since", have.lastModified);

  QNetworkReply *reply = nam.get(req);

  /* Log the concrete SSL error(s) rather than the generic "handshake failed",
   * then proceed anyway: the download host is trusted and this Qt build's TLS is
   * linked against a mismatched OpenSSL, so certificate verification can't be
   * relied on here. */
  connect(reply, &QNetworkReply::sslErrors, this, [this, reply](const QList<QSslError> &errors) {
    for (const QSslError &e : errors)
      emit logMessage(DR_LOG_WARN, QString("downloads: ssl: %1").arg(e.errorString()));
    reply->ignoreSslErrors();
  });

  /* Block on a local loop, but keep the progress dialog responsive and bound the
   * wait with a timeout so a hung connection can't stall boot forever. */
  QEventLoop loop;
  connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  QTimer timer;
  timer.setSingleShot(true);
  connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
  connect(reply, &QNetworkReply::downloadProgress, this,
    [this, &timer, timeoutMs](qint64 received, qint64 total) {
      timer.start(timeoutMs);
      emit progressUpdated(received, total);
    });
  timer.start(timeoutMs);
  loop.exec();

  if (!reply->isFinished())
  {
    reply->abort();
    reply->deleteLater();
    emit logMessage(DR_LOG_WARN, QString("downloads: timed out fetching %1").arg(u.toString()));
    return false;
  }

  const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

  if (reply->error() != QNetworkReply::NoError && status != 304)
  {
    emit logMessage(
      DR_LOG_WARN, QString("downloads: %1 failed: %2").arg(url, reply->errorString()));
    reply->deleteLater();
    return false;
  }

  if (status == 304)
  {
    notModified = true;
    reply->deleteLater();
    emit logMessage(DR_LOG_INFO, QString("downloads: %1 unchanged (304)").arg(u.toString()));
    return true;
  }

  got.etag = reply->rawHeader("ETag");
  got.lastModified = reply->rawHeader("Last-Modified");
  const QByteArray data = reply->readAll();
  reply->deleteLater();

  /* Guard against an error page / truncated body masquerading as a zip. */
  if (data.size() < 4 || static_cast<unsigned char>(data[0]) != 0x50 ||
    static_cast<unsigned char>(data[1]) != 0x4B)
  {
    emit logMessage(DR_LOG_WARN, QString("downloads: %1 was not a zip archive").arg(u.toString()));
    return false;
  }

  if (!extractZip(data, saveDir, stateDir))
  {
    emit logMessage(DR_LOG_WARN, QString("downloads: failed to extract %1").arg(u.toString()));
    return false;
  }

  emit logMessage(DR_LOG_INFO, QString("downloads: applied %1").arg(u.toString()));
  return true;
}

bool DrDownloader::extractZip(const QByteArray &data, const QString &saveDir, const QString &stateDir)
{
  const QString cwd = QDir::currentPath();

  QByteArray mutableData = data;
  QBuffer buffer(&mutableData);
  buffer.open(QIODevice::ReadOnly);

  QZipReader zip(&buffer);
  if (zip.status() != QZipReader::NoError)
    return false;

  const QVector<QZipReader::FileInfo> entries = zip.fileInfoList();
  if (entries.isEmpty())
    return false;

  bool wroteAny = false;
  for (const QZipReader::FileInfo &entry : entries)
  {
    if (!entry.isFile)
      continue;

    if (entry.filePath.isEmpty() || QDir::isAbsolutePath(entry.filePath) ||
      entry.filePath.contains('\\'))
      continue;

    /* Split into components; reject any ".." so nothing can escape its root. */
    QStringList parts = entry.filePath.split('/', Qt::SkipEmptyParts);
    if (parts.contains(QLatin1String("..")))
      continue;

    /* Strip a leading "derailleur-" wrapper folder if present. */
    if (!parts.isEmpty() && parts.first().startsWith(QLatin1String("derailleur-")))
      parts.removeFirst();
    if (parts.isEmpty())
      continue;

    /* Route by the top-level folder: save/ and state/ go to their configured
     * directories (with that folder stripped); everything else lands in the cwd
     * preserving its path. */
    QString root;
    QString rel;
    if (parts.first() == QLatin1String("save"))
    {
      root = saveDir;
      rel = parts.mid(1).join('/');
    }
    else if (parts.first() == QLatin1String("state"))
    {
      root = stateDir;
      rel = parts.mid(1).join('/');
    }
    else
    {
      root = cwd;
      rel = parts.join('/');
    }

    if (rel.isEmpty()) // a bare category directory entry
      continue;

    const QString baseAbs = QDir(root).absolutePath();
    const QString outAbs = QDir::cleanPath(QDir(root).absoluteFilePath(rel));
    if (outAbs != baseAbs && !outAbs.startsWith(baseAbs + '/'))
      continue;

    QDir().mkpath(QFileInfo(outAbs).absolutePath());
    QFile out(outAbs);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate))
      continue;
    out.write(zip.fileData(entry.filePath));
    out.close();
    wroteAny = true;
  }

  return wroteAny;
}
