#include "QRetroInputBackendShared.h"

#include "DrInputStore.h"

QRetroInputBackendShared::QRetroInputBackendShared(DrInputStore *store, QObject *parent)
  : QRetroInputBackend(parent)
  , m_Store(store)
{
}

void QRetroInputBackendShared::init(QRetroInputJoypad *joypads, unsigned maxUsers)
{
  m_Joypads = joypads;
  m_MaxUsers = maxUsers;
}

void QRetroInputBackendShared::poll()
{
  if (m_Store && m_Joypads)
    m_Store->copyTo(m_Joypads, m_MaxUsers);
}
