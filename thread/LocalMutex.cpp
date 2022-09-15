#include "LocalMutex.h"
#include <stdio.h>
LocalMutex::LocalMutex(void)
{
  InitializeCriticalSection(&m_criticalSection);
}

LocalMutex::~LocalMutex(void)
{
  DeleteCriticalSection(&m_criticalSection);
}

void LocalMutex::lock()
{
  EnterCriticalSection(&m_criticalSection);
}

void LocalMutex::unlock()
{
  LeaveCriticalSection(&m_criticalSection);
}
