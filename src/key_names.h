#pragma once
#include <windows.h>
#include <string>

inline std::wstring GetKeyDisplayName(uint16_t vkCode, uint16_t scanCode, uint32_t flags) {
    // Special-case some common keys for friendly presentation
    switch (vkCode) {
    case VK_BACK:       return L"Backspace";
    case VK_TAB:        return L"Tab";
    case VK_RETURN:     return L"Enter";
    case VK_ESCAPE:     return L"Escape";
    case VK_SPACE:      return L"Space";
    case VK_PRIOR:      return L"Page Up";
    case VK_NEXT:       return L"Page Down";
    case VK_END:        return L"End";
    case VK_HOME:       return L"Home";
    case VK_LEFT:       return L"Left Arrow";
    case VK_UP:         return L"Up Arrow";
    case VK_RIGHT:      return L"Right Arrow";
    case VK_DOWN:       return L"Down Arrow";
    case VK_SNAPSHOT:   return L"Print Screen";
    case VK_INSERT:     return L"Insert";
    case VK_DELETE:     return L"Delete";
    case VK_LWIN:       return L"Left Win";
    case VK_RWIN:       return L"Right Win";
    case VK_APPS:       return L"Context Menu";
    case VK_NUMPAD0:    return L"Num 0";
    case VK_NUMPAD1:    return L"Num 1";
    case VK_NUMPAD2:    return L"Num 2";
    case VK_NUMPAD3:    return L"Num 3";
    case VK_NUMPAD4:    return L"Num 4";
    case VK_NUMPAD5:    return L"Num 5";
    case VK_NUMPAD6:    return L"Num 6";
    case VK_NUMPAD7:    return L"Num 7";
    case VK_NUMPAD8:    return L"Num 8";
    case VK_NUMPAD9:    return L"Num 9";
    case VK_MULTIPLY:   return L"Num *";
    case VK_ADD:        return L"Num +";
    case VK_SUBTRACT:   return L"Num -";
    case VK_DECIMAL:    return L"Num .";
    case VK_DIVIDE:     return L"Num /";
    case VK_CAPITAL:    return L"Caps Lock";
    case VK_NUMLOCK:    return L"Num Lock";
    case VK_SCROLL:     return L"Scroll Lock";
    case VK_LSHIFT:     return L"Left Shift";
    case VK_RSHIFT:     return L"Right Shift";
    case VK_LCONTROL:   return L"Left Ctrl";
    case VK_RCONTROL:   return L"Right Ctrl";
    case VK_LMENU:      return L"Left Alt";
    case VK_RMENU:      return L"Right Alt";
    case VK_SHIFT:      return L"Shift";
    case VK_CONTROL:    return L"Ctrl";
    case VK_MENU:       return L"Alt";
    }

    if (vkCode >= VK_F1 && vkCode <= VK_F24) {
        return L"F" + std::to_wstring(vkCode - VK_F1 + 1);
    }

    // Windows GetKeyNameText fallback
    LONG lParamKey = (scanCode << 16);
    if (flags & KEYEVENTF_EXTENDEDKEY) {
        lParamKey |= (1 << 24);
    }

    wchar_t nameBuf[64] = {};
    if (GetKeyNameTextW(lParamKey, nameBuf, 64) > 0) {
        return std::wstring(nameBuf);
    }

    // Direct ASCII char fallback
    if (vkCode >= 0x30 && vkCode <= 0x5A) {
        return std::wstring(1, static_cast<wchar_t>(vkCode));
    }

    return L"0x" + std::to_wstring(vkCode);
}
