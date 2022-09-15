#ifndef __GLOBALMUTEX_H__
#define __GLOBALMUTEX_H__

#include <windows.h>
#include "Exception.h"
#include "Lockable.h"

/**
 * Global mutex (allows to use mutex between separate processes).
 *
 * @author yuri, enikey.
 */
class GlobalMutex : public Lockable
{
public:
  /**
   * Creates new global mutex.
   * @param [optional] name name of mutex.
   * @param throwIfExsist if flag is set then thows exception if mutex exsists.
   * @param interSession if set, then mutex can be accessed from separate sessions, if not,
   * then every session will create it's own mutex.
   * @remark if name is 0, then mutex will be unnamed.
   * @throws Exception when cannot create mutex or when throwIfExist flag is set
   * and mutex already exist.
   */
  GlobalMutex(const TCHAR *name = 0, bool interSession = false, bool throwIfExist = false) throw(Exception);

  /**
   * Deletes global mutex.
   */
  virtual ~GlobalMutex();

  /**
   * Inherited from Lockable.
   */
  virtual void lock();

  /**
   * Inherited from Lockable.
   */
  virtual void unlock();

private:
  void setAccessToAll(HANDLE objHandle);

  HANDLE m_mutex;
};

#endif // __GLOBALMUTEX_H__
