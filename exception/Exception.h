#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#pragma warning(disable:4290)

#include "StringStorage.h"

/**
 * Common Exception class.
 */
class Exception
{
public:
  /**
   * Creates exception with empty description.
   */
  Exception();
  /**
   * Creates exception with specified description.
   * @param format description string in printf-like notation.
   */
  Exception(const TCHAR *format, ...);
  /**
   * Destructor.
   */
  virtual ~Exception();

  /**
   * Returns description of exception.
   */
  const TCHAR *getMessage() const;

protected:
  StringStorage m_message;
};

#endif
