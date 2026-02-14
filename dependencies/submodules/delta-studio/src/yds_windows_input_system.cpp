#include "../include/yds_windows_input_system.h"
#include "platform.h"

// ────────────────────────────────────────────────────────────────
// Windows build — keep the original Raw Input implementation
// ────────────────────────────────────────────────────────────────
#if PLATFORM_WINDOWS

#include "../include/yds_key_maps.h"
#include "../include/yds_windows_input_device.h"
#include "../include/yds_windows_window.h"
#include "../include/yds_windows_window_system.h"

#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>

ysWindowsInputSystem::ysWindowsInputSystem()
    : ysInputSystem(Platform::Windows) {}

ysWindowsInputSystem::~ysWindowsInputSystem() {}

ysInputDevice::InputDeviceType ysWindowsInputSystem::TranslateType(int type) {
    switch (type) {
        case RIM_TYPEHID:      return ysInputDevice::InputDeviceType::CUSTOM;
        case RIM_TYPEKEYBOARD: return ysInputDevice::InputDeviceType::KEYBOARD;
        case RIM_TYPEMOUSE:    return ysInputDevice::InputDeviceType::MOUSE;
    }
    return ysInputDevice::InputDeviceType::UNKNOWN;
}

ysWindowsInputDevice *ysWindowsInputSystem::DeviceLookup(HANDLE hDevice) {
    const int n = m_inputDeviceArray.GetNumObjects();
    for (int i = 0; i < n; i++) {
        ysWindowsInputDevice *d =
            static_cast<ysWindowsInputDevice *>(m_inputDeviceArray.Get(i));
        if (d->m_deviceHandle == hDevice) return d;
    }
    return nullptr;
}

ysWindowsInputDevice *ysWindowsInputSystem::SystemNameDeviceLookup(wchar_t *systemName) {
    const int n = m_inputDeviceArray.GetNumObjects();
    for (int i = 0; i < n; i++) {
        ysWindowsInputDevice *d =
            static_cast<ysWindowsInputDevice *>(m_inputDeviceArray.Get(i));
        if (wcscmp(systemName, d->m_systemName) == 0) return d;
    }
    return nullptr;
}

ysError ysWindowsInputSystem::CheckDeviceStatus(ysInputDevice *device) {
    YDS_ERROR_DECLARE("CheckDeviceStatus");
    if (device == nullptr) return YDS_ERROR_RETURN(ysError::InvalidParameter);

    ysWindowsInputDevice *windowsDevice =
        static_cast<ysWindowsInputDevice *>(device);

    UINT nDevices;
    if (GetRawInputDeviceList(nullptr, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
        return YDS_ERROR_RETURN(ysError::NoDeviceList);

    PRAWINPUTDEVICELIST pList = new RAWINPUTDEVICELIST[nDevices];
    if (!pList) return YDS_ERROR_RETURN(ysError::OutOfMemory);
    if (GetRawInputDeviceList(pList, &nDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
        return YDS_ERROR_RETURN(ysError::NoDeviceList);

    for (unsigned int i = 0; i < nDevices; i++) {
        RID_DEVICE_INFO info; UINT size = sizeof(info);
        GetRawInputDeviceInfo(pList[i].hDevice, RIDI_DEVICEINFO, &info, &size);
        ysInputDevice::InputDeviceType type = TranslateType(info.dwType);
        if (pList[i].hDevice != windowsDevice->m_deviceHandle) continue;
        if (type != device->GetType()) continue;
        delete[] pList;
        return YDS_ERROR_RETURN(ysError::None);
    }
    delete[] pList;
    DisconnectDevice(device);
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysWindowsInputSystem::CheckAllDevices() {
    YDS_ERROR_DECLARE("CheckAllDevices");
    UINT nDevices;
    if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
        return YDS_ERROR_RETURN(ysError::NoDeviceList);

    PRAWINPUTDEVICELIST pList = new RAWINPUTDEVICELIST[nDevices];
    if (!pList) return YDS_ERROR_RETURN(ysError::OutOfMemory);
    if (GetRawInputDeviceList(pList, &nDevices, sizeof(RAWINPUTDEVICELIST)) == (UINT)-1)
        return YDS_ERROR_RETURN(ysError::NoDeviceList);

    const int deviceCount = GetDeviceCount();
    for (int j = deviceCount - 1; j >= 0; j--) {
        ysWindowsInputDevice *d =
            static_cast<ysWindowsInputDevice *>(m_inputDeviceArray.Get(j));
        for (unsigned int i = 0; i < nDevices; i++) {
            RID_DEVICE_INFO info; UINT size = sizeof(info);
            GetRawInputDeviceInfo(pList[i].hDevice, RIDI_DEVICEINFO, &info, &size);
            ysInputDevice::InputDeviceType type = TranslateType(info.dwType);
            if (pList[i].hDevice != d->m_deviceHandle) continue;
            if (type != d->GetType()) continue;
            break;
        }
        DisconnectDevice(d);
    }
    delete[] pList;
    return YDS_ERROR_RETURN(ysError::None);
}

ysError ysWindowsInputSystem::CreateDevices() {
    YDS_ERROR_DECLARE("CreateDevices");
    RAWINPUTDEVICE deviceList[2];
    ysWindowsWindow *primaryWindow =
        m_windowSystem->GetWindowCount() > 0
            ? static_cast<ysWindowsWindow *>(m_windowSystem->GetWindow(0))
            : nullptr;

    deviceList[0] = { 0x01, 0x02, RIDEV_INPUTSINK,
        primaryWindow ? primaryWindow->GetWindowHandle() : NULL };
    deviceList[1] = { 0x01, 0x06, RIDEV_INPUTSINK,
        primaryWindow ? primaryWindow->GetWindowHandle() : NULL };

    if (!RegisterRawInputDevices(deviceList, 2, sizeof(RAWINPUTDEVICE)))
        return YDS_ERROR_RETURN(ysError::CouldNotRegisterForInput);

    return YDS_ERROR_RETURN(ysError::None);
}

ysInputDevice *ysWindowsInputSystem::CreateDevice(
    ysInputDevice::InputDeviceType type, int id)
{
    const char *baseName;
    char deviceName[ysInputDevice::MAX_NAME_LENGTH];
    switch (type) {
        case ysInputDevice::InputDeviceType::CUSTOM:   baseName = "HID";      break;
        case ysInputDevice::InputDeviceType::KEYBOARD: baseName = "KEYBOARD"; break;
        case ysInputDevice::InputDeviceType::MOUSE:    baseName = "MOUSE";    break;
        default: baseName = "";
    }

    int deviceID = (id >= 0) ? id : GetNextDeviceID(type);
    ysWindowsInputDevice *newDevice =
        m_inputDeviceArray.NewGeneric<ysWindowsInputDevice>();
    sprintf_s(deviceName, 256, "%s%0.3d", baseName, deviceID);

    newDevice->SetName(deviceName);
    newDevice->SetType(type);
    newDevice->SetDeviceID(deviceID);
    newDevice->m_deviceHandle = NULL;
    newDevice->SetGeneric(true);
    newDevice->SetVirtual(false);
    memset(&newDevice->m_info, 0, sizeof(newDevice->m_info));

    if (type == ysInputDevice::InputDeviceType::KEYBOARD) {
        ysKeyboard *keyboard = newDevice->GetAsKeyboard();
        keyboard->RegisterKeyMap(ysKeyMaps::GetWindowsKeyMap());
    }
    RegisterDevice(newDevice);
    return newDevice;
}

ysInputDevice *ysWindowsInputSystem::CreateVirtualDevice(
    ysInputDevice::InputDeviceType type)
{
    const char *baseName;
    char deviceName[ysInputDevice::MAX_NAME_LENGTH];
    switch (type) {
        case ysInputDevice::InputDeviceType::CUSTOM:   baseName = "VIRTUAL_HID";      break;
        case ysInputDevice::InputDeviceType::KEYBOARD: baseName = "VIRTUAL_KEYBOARD"; break;
        case ysInputDevice::InputDeviceType::MOUSE:    baseName = "VIRTUAL_MOUSE";    break;
        default: baseName = "";
    }

    ysWindowsInputDevice *newDevice =
        m_inputDeviceArray.NewGeneric<ysWindowsInputDevice>();
    sprintf_s(deviceName, 256, "%s", baseName);
    newDevice->SetName(deviceName);
    newDevice->SetType(type);
    newDevice->SetDeviceID(-1);
    newDevice->m_deviceHandle = NULL;
    newDevice->SetGeneric(true);
    newDevice->SetConnected(true);
    newDevice->SetVirtual(true);
    memset(&newDevice->m_info, 0, sizeof(newDevice->m_info));

    if (type == ysInputDevice::InputDeviceType::KEYBOARD) {
        ysKeyboard *keyboard = newDevice->GetAsKeyboard();
        keyboard->RegisterKeyMap(ysKeyMaps::GetWindowsKeyMap());
    }
    RegisterDevice(newDevice);
    return newDevice;
}

ysWindowsInputDevice *ysWindowsInputSystem::AddDevice(RAWINPUT *rawInput) {
    RID_DEVICE_INFO info; UINT size = sizeof(info);
    UINT nameSize = 256;
    wchar_t systemName[256];

    ysInputDevice::InputDeviceType type = TranslateType(rawInput->header.dwType);
    GetRawInputDeviceInfo(rawInput->header.hDevice, RIDI_DEVICEINFO, &info, &size);
    GetRawInputDeviceInfo(rawInput->header.hDevice, RIDI_DEVICENAME, systemName, &nameSize);

    ysWindowsInputDevice *nameLookup = SystemNameDeviceLookup(systemName);
    ysWindowsInputDevice *newDevice  = nullptr;

    if (nameLookup && !nameLookup->IsConnected()) newDevice = nameLookup;
    else newDevice = static_cast<ysWindowsInputDevice *>(FindGenericSlot(type));

    if (!newDevice)
        newDevice = static_cast<ysWindowsInputDevice *>(CreateDevice(type, -1));

    newDevice->m_deviceHandle = rawInput->header.hDevice;
    newDevice->m_info = info;
    newDevice->SetGeneric(false);
    newDevice->SetConnected(true);
    wcscpy_s(newDevice->m_systemName, 256, systemName);
    return newDevice;
}

int ysWindowsInputSystem::ProcessInputMessage(HRAWINPUT lparam) {
    UINT dwSize = 0;
    GetRawInputData(lparam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
    if (dwSize > m_rawInputBuffer.size()) m_rawInputBuffer.resize(dwSize);
    if (GetRawInputData(lparam, RID_INPUT, m_rawInputBuffer.data(), &dwSize,
                        sizeof(RAWINPUTHEADER)) != dwSize) return 0;

    RAWINPUT *raw = (RAWINPUT *)m_rawInputBuffer.data();
    ysInputDevice *device = nullptr;
    if (raw->header.hDevice) {
        device = DeviceLookup(raw->header.hDevice);
        if (!device) device = AddDevice(raw);
    }

    if (device) {
        if (raw->header.dwType == RIM_TYPEKEYBOARD) {
            ysKeyboard *keyboard = device->GetAsKeyboard();
            ysKey::Code index    = keyboard->GetKeyMap(raw->data.keyboard.VKey);
            const ysKey *key     = keyboard->GetKey(index);

            ysKey::Variation newConf  = key->m_configuration;
            ysKey::State     newState = ysKey::State::DownTransition;

            if (raw->data.keyboard.Flags & RI_KEY_E0) newConf = ysKey::Variation::Left;
            else if (raw->data.keyboard.Flags & RI_KEY_E1) newConf = ysKey::Variation::Right;
            if (raw->data.keyboard.Flags & RI_KEY_BREAK) newState = ysKey::State::UpTransition;

            keyboard->SetKeyState(index, newState, newConf);
        } else if (raw->header.dwType == RIM_TYPEMOUSE) {
            ysMouse *mouse = device->GetAsMouse();
            bool delta = !(raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE);

            POINT p;
            if (GetCursorPos(&p)) mouse->SetOsPosition(p.x, p.y);
            mouse->UpdatePosition(raw->data.mouse.lLastX, raw->data.mouse.lLastY, delta);

            if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
                mouse->UpdateWheel((int)((short)raw->data.mouse.usButtonData));

            #define BTN(down_flag, up_flag, btn) \
                if (raw->data.mouse.usButtonFlags & (down_flag)) \
                    mouse->UpdateButton(ysMouse::Button::btn, ysMouse::ButtonState::DownTransition); \
                if (raw->data.mouse.usButtonFlags & (up_flag)) \
                    mouse->UpdateButton(ysMouse::Button::btn, ysMouse::ButtonState::UpTransition);

            BTN(RI_MOUSE_LEFT_BUTTON_DOWN,   RI_MOUSE_LEFT_BUTTON_UP,   Left)
            BTN(RI_MOUSE_RIGHT_BUTTON_DOWN,  RI_MOUSE_RIGHT_BUTTON_UP,  Right)
            BTN(RI_MOUSE_MIDDLE_BUTTON_DOWN, RI_MOUSE_MIDDLE_BUTTON_UP, Middle)
            BTN(RI_MOUSE_BUTTON_1_DOWN,      RI_MOUSE_BUTTON_1_UP,      Aux_1)
            BTN(RI_MOUSE_BUTTON_2_DOWN,      RI_MOUSE_BUTTON_2_UP,      Aux_2)
            BTN(RI_MOUSE_BUTTON_3_DOWN,      RI_MOUSE_BUTTON_3_UP,      Aux_3)
            BTN(RI_MOUSE_BUTTON_4_DOWN,      RI_MOUSE_BUTTON_4_UP,      Aux_4)
            BTN(RI_MOUSE_BUTTON_5_DOWN,      RI_MOUSE_BUTTON_5_UP,      Aux_5)
            #undef BTN
        }
    }
    return 0;
}

int ysWindowsInputSystem::OnOsKey(LPARAM lParam, WPARAM wParam) {
    const WORD vkCode      = LOWORD(wParam);
    const WORD keyFlags    = HIWORD(lParam);
    const bool wasKeyDown  = (keyFlags & KF_REPEAT) == KF_REPEAT;
    const bool isKeyReleased = (keyFlags & KF_UP)   == KF_UP;

    ysKeyboard *keyboard = GetDefaultKeyboard();
    ysKey::Code index    = keyboard->GetKeyMap(vkCode);
    const ysKey *key     = keyboard->GetKey(index);

    ysKey::Variation newConf  = key->m_configuration;
    ysKey::State     newState = key->m_state;

    if (isKeyReleased) newState = ysKey::State::UpTransition;
    else if (!wasKeyDown) newState = ysKey::State::DownTransition;

    keyboard->SetKeyState(index, newState, newConf);
    return 0;
}

int ysWindowsInputSystem::OnOsMouseButtonDown(ysMouse::Button button) {
    GetDefaultMouse()->UpdateButton(button, ysMouse::ButtonState::DownTransition);
    return 0;
}

int ysWindowsInputSystem::OnOsMouseButtonUp(ysMouse::Button button) {
    GetDefaultMouse()->UpdateButton(button, ysMouse::ButtonState::UpTransition);
    return 0;
}

int ysWindowsInputSystem::OnOsMouseWheel(LPARAM lParam, WPARAM wParam) {
    (void)lParam;
    GetDefaultMouse()->UpdateWheel(GET_WHEEL_DELTA_WPARAM(wParam));
    return 0;
}

int ysWindowsInputSystem::OnOsMouseMove(LPARAM lParam, WPARAM wParam) {
    (void)wParam;
    const int x_pos = GET_X_LPARAM(lParam);
    const int y_pos = GET_Y_LPARAM(lParam);
    ysMouse *mouse = GetDefaultMouse();
    mouse->UpdatePosition(x_pos, y_pos, false);
    POINT p;
    if (GetCursorPos(&p)) mouse->SetOsPosition(p.x, p.y);
    return 0;
}

// ────────────────────────────────────────────────────────────────
// Non-Windows stub — input handled via SDL2 events in the
// SDL window system; these methods are never called on Linux.
// ────────────────────────────────────────────────────────────────
#else  // !PLATFORM_WINDOWS

ysWindowsInputSystem::ysWindowsInputSystem()
    : ysInputSystem(Platform::Windows) {}

ysWindowsInputSystem::~ysWindowsInputSystem() {}

ysInputDevice::InputDeviceType ysWindowsInputSystem::TranslateType(int) {
    return ysInputDevice::InputDeviceType::UNKNOWN;
}

ysWindowsInputDevice *ysWindowsInputSystem::DeviceLookup(void *) { return nullptr; }
ysWindowsInputDevice *ysWindowsInputSystem::SystemNameDeviceLookup(wchar_t *) { return nullptr; }

ysError ysWindowsInputSystem::CheckDeviceStatus(ysInputDevice *) {
    YDS_ERROR_DECLARE("CheckDeviceStatus");
    return YDS_ERROR_RETURN(ysError::None);
}
ysError ysWindowsInputSystem::CheckAllDevices() {
    YDS_ERROR_DECLARE("CheckAllDevices");
    return YDS_ERROR_RETURN(ysError::None);
}
ysError ysWindowsInputSystem::CreateDevices() {
    YDS_ERROR_DECLARE("CreateDevices");
    return YDS_ERROR_RETURN(ysError::None);
}
ysInputDevice *ysWindowsInputSystem::CreateDevice(ysInputDevice::InputDeviceType, int) { return nullptr; }
ysInputDevice *ysWindowsInputSystem::CreateVirtualDevice(ysInputDevice::InputDeviceType) { return nullptr; }
ysWindowsInputDevice *ysWindowsInputSystem::AddDevice(void *) { return nullptr; }
int ysWindowsInputSystem::ProcessInputMessage(void *) { return 0; }
int ysWindowsInputSystem::OnOsKey(long, unsigned long long) { return 0; }
int ysWindowsInputSystem::OnOsMouseButtonDown(ysMouse::Button) { return 0; }
int ysWindowsInputSystem::OnOsMouseButtonUp(ysMouse::Button) { return 0; }
int ysWindowsInputSystem::OnOsMouseWheel(long, unsigned long long) { return 0; }
int ysWindowsInputSystem::OnOsMouseMove(long, unsigned long long) { return 0; }

#endif  // PLATFORM_WINDOWS