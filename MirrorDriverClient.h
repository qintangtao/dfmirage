#ifndef MIRRORDRIVERCLIENT_H
#define MIRRORDRIVERCLIENT_H

#include <tchar.h>
#include <windows.h>

#include "DisplayEsc.h"
#include "RegistryKey.h"
#include "PixelFormat.h"
#include "Dimension.h"
#include "Point.h"

class MirrorDriverClient
{
public:
    explicit MirrorDriverClient();
    ~MirrorDriverClient();

    inline PixelFormat getPixelFormat() const
    { return m_pixelFormat; }
    inline Dimension getDimension() const
    { return m_dimension; }

    inline void *getBuffer() const
    { return m_screenBuffer; }
    inline CHANGES_BUF *getChangesBuf() const
    { return m_changesBuffer; }

    void open();
    void close();

    void load();
    void unload();

    void connect();
    void disconnect();

private:
  static const TCHAR MINIPORT_REGISTRY_PATH[];

  static const int EXT_DEVMODE_SIZE_MAX = 3072;
  struct DFEXT_DEVMODE : DEVMODE
  {
    char extension[EXT_DEVMODE_SIZE_MAX];
  };

private:
    void dispose();

    void extractDeviceInfo(TCHAR *driverName);
    void openDeviceRegKey(TCHAR *miniportName);

    void initScreenPropertiesByCurrent();
    // value - true to attach, false to detach.
    void setAttachToDesktop(bool value);
    void commitDisplayChanges(DEVMODE *pdm);

private:
    // Driver states.
    bool m_isDriverOpened;
    bool m_isDriverLoaded;
    bool m_isDriverAttached;
    bool m_isDriverConnected;

    DWORD m_deviceNumber;
    DISPLAY_DEVICE m_deviceInfo;
    RegistryKey m_regkeyDevice;
    DFEXT_DEVMODE m_deviceMode;
    HDC m_driverDC;

    CHANGES_BUF *m_changesBuffer;
    void *m_screenBuffer;

    //WindowsEvent m_initListener;
    bool m_isDisplayChanged;
    //MessageWindow m_propertyChangeListenerWindow;

    PixelFormat m_pixelFormat;
    Dimension m_dimension;
    Point m_leftTopCorner;
    //Screen m_screen;
};

#endif // MIRRORDRIVERCLIENT_H
