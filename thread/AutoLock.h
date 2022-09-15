#ifndef __AUTOLOCK_H__
#define __AUTOLOCK_H__

#include "Lockable.h"

class AutoLock
{
public:
  AutoLock(Lockable *locker);
  virtual ~AutoLock();

protected:
  Lockable *m_locker;
};

#endif // __AUTOLOCK_H__
