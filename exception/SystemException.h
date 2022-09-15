#ifndef _SYSTEM_EXCEPTION_H_
#define _SYSTEM_EXCEPTION_H_

#include "Exception.h"

/**
 * Windows exception.
 *
 * Solves problem with generating formatted message strings width describes
 * user code-space where error occured and windows specific information about WinAPI error.
 */
class SystemException : public Exception
{
public:
  /**
   * Creates exception with formatted message from system
   * and error code which equals to GetLastError() value.
   */
  SystemException();
  /**
   * Creates exception with formatted message from system
   * and error code.
   * @param errcode windows error code.
   */
  SystemException(int errcode);
  /**
   * Creates exception with user message + formatted message from system
   * and error code set to GetLastError() value.
   * @param userMessage user message.
   */
  SystemException(const TCHAR *userMessage);
  /**
   * Creates exception with user message + formatted message from system
   * and specified error code.
   * @param userMessage user message.
   * @param errcode windows error code.
   */
  SystemException(const TCHAR *userMessage, int errcode);
  /**
   * Destructor, does nothing.
   */
  virtual ~SystemException();
  /**
   * Returns error code.
   * @return windows error code associated with this exception.
   */
  int getErrorCode() const;
  /**
   * Returns system error description.
   * @return system error description.
   */
  const TCHAR *getSystemErrorDescription() const;
private:
  /**
   * Creates formatted message for exception.
   * @param userMessage user description about exception reason.
   * @param errcode windows error code.
   * @fixme document all special cases.
   */
  void createMessage(const TCHAR *userMessage, int errcode);
private:
  /**
   * Description of error from OS.
   */
  StringStorage m_systemMessage;
  /**
   * Windows error code.
   */
  int m_errcode;
};

#endif
