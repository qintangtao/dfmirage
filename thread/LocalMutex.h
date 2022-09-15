
#ifndef __LOCALMUTEX_H__
#define __LOCALMUTEX_H__

#include <windows.h>
#include "Lockable.h"

/**
 * Local mutex (cannot be used within separate processes).
 *
 * @remark local mutex uses Windows critical sections to implement
 * lockable interface..
 */
class LocalMutex : public Lockable
{
public:
  /**
   * Creates new local mutex.
   */
  LocalMutex();

  /**
   * Deletes local mutex.
   */
  virtual ~LocalMutex();

  /**
   * Inherited from Lockable.
   */
  virtual void lock();

  /**
   * Inherited from Lockable.
   */
  virtual void unlock();

private:
  /**
   * Windows critical section.
   */
  CRITICAL_SECTION m_criticalSection;
};

#endif // __LOCALMUTEX_H__
