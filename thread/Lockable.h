#ifndef __LOCKABLE_H__
#define __LOCKABLE_H__

/**
 * Synchronized (thread-safe) object that can be locked and unlocked.
 */
class Lockable
{
public:
  virtual ~Lockable() {}

  /**
   * Locks object.
   */
  virtual void lock() = 0;

  /**
   * Unlocks object.
   */
  virtual void unlock() = 0;
};

#endif // __LOCKABLE_H__
