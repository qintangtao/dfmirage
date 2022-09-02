#include "MirrorDriverClient.h"
#include "Exception.h"
#include <stdio.h>
const TCHAR MirrorDriverClient::MINIPORT_REGISTRY_PATH[] =
  _T("SYSTEM\\CurrentControlSet\\Hardware Profiles\\")
  _T("Current\\System\\CurrentControlSet\\Services");

MirrorDriverClient::MirrorDriverClient() :
    m_isDriverOpened(false),
    m_isDriverLoaded(false),
    m_isDriverAttached(false),
    m_isDriverConnected(false),
    m_isDisplayChanged(false),
    m_deviceNumber(0),
    m_driverDC(0),
    m_changesBuffer(0),
    m_screenBuffer(0)
{
    memset(&m_deviceMode, 0, sizeof(m_deviceMode));
    m_deviceMode.dmSize = sizeof(DEVMODE);

    try {
        open();
        load();
        connect();
      } catch (Exception &e) {
        _ftprintf(stderr, _T("An error occured during the mirror driver initialization: %ls\n"), e.getMessage());
      }
}

MirrorDriverClient::~MirrorDriverClient()
{
    try {
        dispose();
      } catch (Exception &e) {
        _ftprintf(stderr, _T("An error occured during the mirror driver deinitialization: %ls\n"), e.getMessage());
      }
}

void MirrorDriverClient::open()
{
  _ASSERT(!m_isDriverOpened);

  extractDeviceInfo(_T("Mirage Driver"));
  openDeviceRegKey(_T("dfmirage"));

  m_isDriverOpened = true;
}
void MirrorDriverClient::close()
{
  m_regkeyDevice.close();
  m_isDriverOpened = false;
}

void MirrorDriverClient::load()
{
  _ASSERT(m_isDriverOpened);
  if (!m_isDriverLoaded) {
     _ftprintf(stdout, _T("Loading mirror driver...\n"));

     initScreenPropertiesByCurrent();

    WORD drvExtraSaved = m_deviceMode.dmDriverExtra;
    // IMPORTANT: we dont touch extension data and size
    memset(&m_deviceMode, 0, sizeof(DEVMODE));
    // m_deviceMode.dmSize = sizeof(m_deviceMode);
    m_deviceMode.dmSize = sizeof(DEVMODE);
    // 2005.10.07
    m_deviceMode.dmDriverExtra = drvExtraSaved;

    m_deviceMode.dmPelsWidth = m_dimension.width;
    m_deviceMode.dmPelsHeight = m_dimension.height;
    m_deviceMode.dmBitsPerPel = m_pixelFormat.bitsPerPixel;
    m_deviceMode.dmPosition.x = m_leftTopCorner.x;
    m_deviceMode.dmPosition.y = m_leftTopCorner.y;

    m_deviceMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH |
                            DM_PELSHEIGHT | DM_POSITION;
    m_deviceMode.dmDeviceName[0] = '\0';

    setAttachToDesktop(true);
    commitDisplayChanges(&m_deviceMode);

    // Win 2000 version:
    // m_driverDC = CreateDC(_T("DISPLAY"), m_deviceInfo.DeviceName, NULL, NULL);
    m_driverDC = CreateDC(m_deviceInfo.DeviceName, 0, 0, 0);
    if (!m_driverDC) {
      throw Exception(_T("Can't create device context on mirror driver"));
    }
    _ftprintf(stdout, _T("Device context is created\n"));

    m_isDriverLoaded = true;
    _ftprintf(stdout, _T("Mirror driver is now loaded\n"));
  }
}

void MirrorDriverClient::unload()
{
  if (m_driverDC != 0) {
    DeleteDC(m_driverDC);
    m_driverDC = 0;
    _ftprintf(stdout, _T("The mirror driver device context released\n"));
  }

  if (m_isDriverAttached) {
      _ftprintf(stdout, _T("Unloading mirror driver...\n"));

    setAttachToDesktop(false);

    m_deviceMode.dmPelsWidth = 0;
    m_deviceMode.dmPelsHeight = 0;

    // IMPORTANT: Windows 2000 fails to unload the driver
    // if the mode passed to ChangeDisplaySettingsEx() contains DM_POSITION set.
    DEVMODE *pdm = 0;
    //if (!Environment::isWin2000()) {
      pdm = &m_deviceMode;
    //}

    try {
      commitDisplayChanges(pdm);
      _ftprintf(stdout, _T("Mirror driver is unloaded\n"));
    } catch (Exception &e) {
          _ftprintf(stderr, _T("Failed to unload the mirror driver:  %ls\n"), e.getMessage());
    }
  }

  // NOTE: extension data and size is also reset
  memset(&m_deviceMode, 0, sizeof(m_deviceMode));
  m_deviceMode.dmSize = sizeof(DEVMODE);

  m_isDriverLoaded = false;
}

void MirrorDriverClient::connect()
{
    _ftprintf(stdout, _T("Try to connect to the mirror driver.\n"));
  if (!m_isDriverConnected) {
    GETCHANGESBUF buf = {0};
    int res = ExtEscape(m_driverDC, dmf_esc_usm_pipe_map, 0, 0, sizeof(buf), (LPSTR)&buf);
    if (res <= 0) {
      StringStorage errMess;
      errMess.format(_T("Can't set a connection for the mirror driver: ")
                     _T("ExtEscape() failed with %d"),
                     res);
      throw Exception(errMess.getString());
    }

    m_changesBuffer = buf.buffer;
    m_screenBuffer = buf.Userbuffer;

    m_isDriverConnected = true;
  }
}

void MirrorDriverClient::disconnect()
{
     _ftprintf(stdout, _T("Try to disconnect the mirror driver.\n"));
  if (m_isDriverConnected) {
    GETCHANGESBUF buf;
    buf.buffer = m_changesBuffer;
    buf.Userbuffer = m_screenBuffer;

    int res = ExtEscape(m_driverDC, dmf_esc_usm_pipe_unmap, sizeof(buf), (LPSTR)&buf, 0, 0);
    if (res <= 0) {
        _ftprintf(stderr, _T("Can't unmap buffer: error code = %d\n"), res);
    }
    m_isDriverConnected = false;
  }
}

void MirrorDriverClient::dispose()
{
  if (m_isDriverConnected) {
    disconnect();
  }
  if (m_isDriverLoaded) {
    unload();
  }
  if (m_isDriverOpened) {
    close();
  }
}

void MirrorDriverClient::extractDeviceInfo(TCHAR *driverName)
{
  memset(&m_deviceInfo, 0, sizeof(m_deviceInfo));
  m_deviceInfo.cb = sizeof(m_deviceInfo);

  _ftprintf(stdout, _T("Searching for: %ls\n"), driverName);

  m_deviceNumber = 0;
  BOOL result;
  while (result = EnumDisplayDevices(0, m_deviceNumber, &m_deviceInfo, 0)) {
      _ftprintf(stdout, _T("DeviceString:%30ls, DeviceName:%15ls, DeviceKey:%ls\n"),
                m_deviceInfo.DeviceString, m_deviceInfo.DeviceName, m_deviceInfo.DeviceKey);
    StringStorage deviceString(m_deviceInfo.DeviceString);
    if (deviceString.isEqualTo(driverName)) {
        _ftprintf(stdout, _T("%ls is found\n"), driverName);
        break;
    }
    m_deviceNumber++;
  }
  if (!result) {
    StringStorage errMess;
    errMess.format(_T("Can't find %ls!"), driverName);
    throw Exception(errMess.getString());
  }
}

void MirrorDriverClient::openDeviceRegKey(TCHAR *miniportName)
{
  StringStorage deviceKey(m_deviceInfo.DeviceKey);
  deviceKey.toUpperCase();
  TCHAR *substrPos = deviceKey.find(_T("\\DEVICE"));
  StringStorage subKey(_T("DEVICE0"));
  if (substrPos != 0) {
    StringStorage str(substrPos);
    if (str.getLength() >= 8) {
      str.getSubstring(&subKey, 1, 7);
    }
  }

  _ftprintf(stdout, _T("Opening registry key %ls\\%ls\\%ls\n"),
               MINIPORT_REGISTRY_PATH,
               miniportName,
               subKey.getString());

  RegistryKey regKeyServices(HKEY_LOCAL_MACHINE, MINIPORT_REGISTRY_PATH, true);
  RegistryKey regKeyDriver(&regKeyServices, miniportName, true);
  m_regkeyDevice.open(&regKeyDriver, subKey.getString(), true);
  if (!regKeyServices.isOpened() || !regKeyDriver.isOpened() ||
      !m_regkeyDevice.isOpened()) {
        throw Exception(_T("Can't open registry for the mirror driver"));
  }
}

void MirrorDriverClient::initScreenPropertiesByCurrent()
{
  m_pixelFormat.initBigEndianByNative();
  m_pixelFormat.bitsPerPixel = 32;
  m_pixelFormat.redMax = 255;
  m_pixelFormat.redShift = 16;
  m_pixelFormat.greenMax = 255;
  m_pixelFormat.greenShift = 8;
  m_pixelFormat.blueMax = 255;
  m_pixelFormat.blueShift = 0;
  m_pixelFormat.colorDepth = 24;

  Rect virtDeskRect;
  virtDeskRect.left = GetSystemMetrics(SM_XVIRTUALSCREEN);
  virtDeskRect.top = GetSystemMetrics(SM_YVIRTUALSCREEN);
  virtDeskRect.setWidth(GetSystemMetrics(SM_CXVIRTUALSCREEN));
  virtDeskRect.setHeight(GetSystemMetrics(SM_CYVIRTUALSCREEN));

   m_dimension.setDim(&virtDeskRect);
  m_leftTopCorner.setPoint(virtDeskRect.left, virtDeskRect.top);
}

void MirrorDriverClient::setAttachToDesktop(bool value)
{
  if (!m_regkeyDevice.setValueAsInt32(_T("Attach.ToDesktop"),
      (int)value)) {
    throw Exception(_T("Can't set the Attach.ToDesktop."));
  }
  m_isDriverAttached = value;
}

void MirrorDriverClient::commitDisplayChanges(DEVMODE *pdm)
{
  // MSDN: Passing NULL for the lpDevMode parameter and 0 for the
  // dwFlags parameter is the easiest way to return to the default
  // mode after a dynamic mode change.
  // But the "default mode" does not mean that the driver is
  // turned off. Especially, when a OS was turned off with turned on driver.
  // (The driver is deactivated but a default mode is saved as was in
  // previous session)

  // 2005.05.21
  // PRB: XP does not work with the parameters:
  // ChangeDisplaySettingsEx(m_deviceInfo.DeviceName, pdm, NULL,
  //                         CDS_UPDATEREGISTRY, NULL)
  // And the 2000 does not work with DEVMODE that has the set DM_POSITION bit.
  _ftprintf(stdout, _T("commitDisplayChanges(1): %ls\n"), m_deviceInfo.DeviceName);

  if (pdm) {
    LONG code = ChangeDisplaySettingsEx(m_deviceInfo.DeviceName, pdm, 0, CDS_UPDATEREGISTRY, 0);
    if (code < 0) {
      StringStorage errMess;
      errMess.format(_T("1st ChangeDisplaySettingsEx() failed with code %d"),
                     (int)code);
      throw Exception(errMess.getString());
    }
    _ftprintf(stdout, _T("commitDisplayChanges(2): %ls\n"), m_deviceInfo.DeviceName);

    code = ChangeDisplaySettingsEx(m_deviceInfo.DeviceName, pdm, 0, 0, 0);
    if (code < 0) {
      StringStorage errMess;
      errMess.format(_T("2nd ChangeDisplaySettingsEx() failed with code %d"),
                     (int)code);
      throw Exception(errMess.getString());
    }
  } else {
    LONG code = ChangeDisplaySettingsEx(m_deviceInfo.DeviceName, 0, 0, 0, 0);
    if (code < 0) {
      StringStorage errMess;
      errMess.format(_T("ChangeDisplaySettingsEx() failed with code %d"),
                     (int)code);
      throw Exception(errMess.getString());
    }
  }
  _ftprintf(stdout, _T("ChangeDisplaySettingsEx() was successfull\n"));
}
