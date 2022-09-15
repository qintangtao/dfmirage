#include "AutoLock.h"

AutoLock::AutoLock(Lockable *locker)
: m_locker(locker)
{
  m_locker->lock();
}

AutoLock::~AutoLock()
{
  m_locker->unlock();
}
