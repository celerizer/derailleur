#include "DrDebugCore.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <QApplication>
#include <QFileInfo>
#include <QPalette>
#include <QRetro.h>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>

#include "DrLogger.h"
#include "DrPaths.h"
#include "classicslive-integration/editor/cle_memory_inspector.h"

extern "C"
{
#include "classicslive-integration/cl_abi.h"
#include "classicslive-integration/cl_memory.h"
}

/* Matches the CL editor's own refresh rate */
#define DR_INSPECTOR_TICK_MS 100

/* The debug core, which a scan pauses, and whether it currently holds it paused */
static QPointer<QRetro> dr_cl_core;
static bool dr_cl_paused = false;

static cl_error dr_cl_set_pause(unsigned mode)
{
  QRetro *core = dr_cl_core.data();

  if (!core)
    return CL_OK;
  if (mode && !dr_cl_paused)
  {
    core->pause();
    dr_cl_paused = true;
  }
  else if (!mode && dr_cl_paused)
  {
    core->unpause();
    dr_cl_paused = false;
  }

  return CL_OK;
}

/* Required by the ABI, but regions are installed directly by installRegions() */
static cl_error dr_cl_install_memory_regions(cl_memory_region_t **regions, unsigned *region_count)
{
  CL_UNUSED(regions);
  CL_UNUSED(region_count);
  return CL_ERR_CLIENT_RUNTIME;
}

static void dr_cl_register(void)
{
  static cl_abi_t abi;
  static bool registered = false;

  if (registered)
    return;
  memset(&abi, 0, sizeof(abi));
  abi.version = CL_ABI_VERSION;
  abi.functions.core.install_memory_regions = dr_cl_install_memory_regions;
  abi.functions.core.set_pause = dr_cl_set_pause;
  registered = cl_abi_register(&abi) == CL_OK;
}

/* Whether `corePath` is an N64 core, which keeps big-endian RDRAM as
 * host-order words without saying so in its memory map */
static bool dr_debug_core_is_n64(const QString &corePath)
{
  const QString name = QFileInfo(corePath).completeBaseName();
  const QString mupen =
    QFileInfo(DrPaths::instance().corePath(DR_CORE_MUPEN64PLUSNEXT)).completeBaseName();

  return name.compare(mupen, Qt::CaseInsensitive) == 0 ||
         name.compare("parallel_n64_libretro", Qt::CaseInsensitive) == 0;
}

/* "core: content", for window titles. QRetro::corePath() is the core's folder,
 * so this works from the paths launch() was given. */
static QString dr_debug_core_label(const QString &corePath, const QString &contentPath)
{
  const QString core_name = QFileInfo(corePath).completeBaseName();
  const QString content_name = QFileInfo(contentPath).fileName();

  return content_name.isEmpty() ? core_name : core_name + ": " + content_name;
}

/* The CL editor's dark Fusion look, which its hex view's colors assume */
static void dr_cl_apply_theme(QWidget *widget)
{
  static QStyle *fusion_style = nullptr;
  QPalette dark_palette;

  if (!fusion_style)
  {
    fusion_style = QStyleFactory::create("Fusion");
    fusion_style->setParent(qApp);
  }

  dark_palette.setColor(QPalette::Window, QColor(53, 53, 53));
  dark_palette.setColor(QPalette::WindowText, Qt::white);
  dark_palette.setColor(QPalette::Base, QColor(25, 25, 25));
  dark_palette.setColor(QPalette::AlternateBase, QColor(30, 30, 30));
  dark_palette.setColor(QPalette::ToolTipBase, Qt::white);
  dark_palette.setColor(QPalette::ToolTipText, Qt::white);
  dark_palette.setColor(QPalette::Text, Qt::white);
  dark_palette.setColor(QPalette::Button, QColor(53, 53, 53));
  dark_palette.setColor(QPalette::ButtonText, Qt::white);
  dark_palette.setColor(QPalette::BrightText, Qt::red);
  dark_palette.setColor(QPalette::Link, QColor(26, 183, 234));
  dark_palette.setColor(QPalette::Highlight, QColor(26, 183, 234));
  dark_palette.setColor(QPalette::HighlightedText, Qt::black);

  widget->setStyle(fusion_style);
  widget->setPalette(dark_palette);
  widget->setStyleSheet(
    "QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
  for (QWidget *child : widget->findChildren<QWidget *>())
  {
    child->setStyle(fusion_style);
    child->setPalette(dark_palette);
  }
}

DrDebugCore::DrDebugCore(DrLogger *logger, QObject *parent)
  : QObject(parent)
  , m_Logger(logger)
{
  m_Timer = new QTimer(this);
  connect(m_Timer, &QTimer::timeout, this, &DrDebugCore::tick);
}

DrDebugCore::~DrDebugCore()
{
  close();
}

bool DrDebugCore::launch(const QString &corePath, const QString &contentPath)
{
  const QByteArray core_path = corePath.toUtf8();
  const QByteArray content_path = contentPath.toUtf8();
  QRetro *core;

  close();
  dr_cl_register();

  core = new QRetro();
  m_Logger->attachCore(core);

  if (!core->loadCore(core_path.constData()))
  {
    m_Logger->message(DR_LOG_ERROR, QString("[debug] failed to load core: %1").arg(corePath));
    delete core;
    return false;
  }
  if (!contentPath.isEmpty() && !core->loadContent(content_path.constData()))
  {
    m_Logger->message(DR_LOG_ERROR, QString("[debug] failed to load content: %1").arg(contentPath));
    delete core;
    return false;
  }

  m_Core = core;
  dr_cl_core = core;
  m_Label = dr_debug_core_label(corePath, contentPath);
  m_WordFlip = dr_debug_core_is_n64(corePath);

  /* Memory is set up on the timing thread just before this; read it from here on */
  connect(core, &QRetro::onCoreStart, this, [this]() {
    if (!m_Core)
      return;
    /* The inspector's searches are built over the regions, so they go first */
    installRegions();
    createInspector();
  }, Qt::QueuedConnection);
  /* Closing the core's window unloads and deletes it (QRetro handles QEvent::Close) */
  connect(core, &QObject::destroyed, this, [this]() { close(); });

  if (!core->startCore())
  {
    m_Logger->message(DR_LOG_ERROR, QString("[debug] failed to start core: %1").arg(corePath));
    close();
    return false;
  }

  core->setTitle(m_Label);
  core->resize(640, 480);
  core->show();
  m_Timer->start(DR_INSPECTOR_TICK_MS);
  m_Logger->message(DR_LOG_INFO, QString("[debug] launched %1").arg(m_Label));

  return true;
}

void DrDebugCore::close(void)
{
  QRetro *core = m_Core.data();

  m_Timer->stop();
  destroyInspector();

  if (dr_cl_paused && core)
    core->unpause();
  dr_cl_paused = false;
  dr_cl_core = nullptr;
  m_Core = nullptr;
  cl_memory_free();

  if (core)
  {
    disconnect(core, nullptr, this, nullptr);
    /* No-op if its own close already unloaded it */
    core->unloadCore();
    core->deleteLater();
  }
}

void DrDebugCore::createInspector(void)
{
  m_Inspector = new CleMemoryInspector();
  m_Inspector->setAttribute(Qt::WA_DeleteOnClose);
  m_Inspector->setWindowTitle(tr("Live Editor - %1").arg(m_Label));
  dr_cl_apply_theme(m_Inspector);

  /* Its own timer would refresh without checking the core is still loaded, so
   * tick() drives it instead */
  for (QTimer *timer : m_Inspector->findChildren<QTimer *>(QString(), Qt::FindDirectChildrenOnly))
    timer->stop();

  /* Closing the inspector closes the core too */
  connect(m_Inspector, &QObject::destroyed, this, [this]() { close(); });
  m_Inspector->show();
}

void DrDebugCore::destroyInspector(void)
{
  CleMemoryInspector *inspector = m_Inspector.data();

  if (!inspector)
    return;
  disconnect(inspector, nullptr, this, nullptr);
  m_Inspector = nullptr;
  inspector->hide();
  inspector->deleteLater();
}

quintptr DrDebugCore::layoutKey(void)
{
  retro_memory_map *maps = m_Core->memory().memoryMaps();
  const retro_memory_descriptor *ram =
    maps->num_descriptors ? nullptr : m_Core->memory().containing(0);

  return reinterpret_cast<quintptr>(maps->descriptors) ^ maps->num_descriptors
    ^ (ram ? reinterpret_cast<quintptr>(ram->ptr) : 0);
}

void DrDebugCore::installRegions(void)
{
  retro_memory_map *maps;
  const retro_memory_descriptor *descs;
  unsigned desc_count;
  cl_memory_region_t *regions;
  unsigned region_count = 0;
  unsigned i, j;

  cl_memory_free();
  if (!m_Core)
    return;
  maps = m_Core->memory().memoryMaps();
  descs = maps->descriptors;
  desc_count = maps->num_descriptors;
  m_LayoutKey = layoutKey();

  /* Cores without memory maps (mupen) still expose their system RAM */
  if (!desc_count)
  {
    descs = m_Core->memory().containing(0);
    desc_count = descs ? 1 : 0;
  }
  if (!desc_count)
    return;

  regions = static_cast<cl_memory_region_t *>(calloc(desc_count, sizeof(cl_memory_region_t)));
  for (i = 0; i < desc_count; i++)
  {
    const retro_memory_descriptor *desc = &descs[i];
    cl_memory_region_t *region;
    uint8_t *host;
    bool mirror = false;

    if (!desc->ptr || !desc->len)
      continue;
    host = static_cast<uint8_t *>(desc->ptr) + desc->offset;

    /* Skip mirrors and subregions of a region already added */
    for (j = 0; j < region_count && !mirror; j++)
    {
      uint8_t *base = static_cast<uint8_t *>(regions[j].base_host);

      mirror = host >= base && host < base + regions[j].size;
    }
    if (mirror)
      continue;

    region = &regions[region_count++];
    region->base_host = host;
    region->base_guest = desc->start;
    region->size = desc->len;
    region->pointer_length = 4;
    region->flags.bits.read = 1;
    region->flags.bits.write = (desc->flags & RETRO_MEMDESC_CONST) ? 0 : 1;
    /* Also turns on the hex view's word-flip swizzle for these regions */
    if (m_WordFlip)
      region->endianness = CL_ENDIAN_WORD_FLIP_BL;
    else
      region->endianness =
        (desc->flags & RETRO_MEMDESC_BIGENDIAN) ? CL_ENDIAN_BIG : CL_ENDIAN_LITTLE;
    if (desc->addrspace && desc->addrspace[0])
      snprintf(region->title, sizeof(region->title), "%s", desc->addrspace);
    else
      snprintf(region->title, sizeof(region->title), "Region %u", i + 1);
  }

  if (!region_count)
  {
    free(regions);
    return;
  }
  memory.regions = regions;
  memory.region_count = region_count;

  m_Logger->message(DR_LOG_INFO, QString("[debug] %1 memory region(s), first at 0x%2 (%3)")
    .arg(region_count)
    .arg(static_cast<qulonglong>(regions[0].base_guest), 0, 16)
    .arg(cl_string_endianness(regions[0].endianness)));
}

void DrDebugCore::tick(void)
{
  if (!m_Inspector)
    return;

  /* Unloaded under us: its memory is gone */
  if (!m_Core || !m_Core->isActive())
  {
    close();
    return;
  }

  if (layoutKey() != m_LayoutKey)
  {
    /* The inspector's searches hold pointers into the old region table, so
     * start it over */
    destroyInspector();
    installRegions();
    createInspector();
  }

  m_Inspector->run();
}
