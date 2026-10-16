#pragma once
#include <windows.h>
#include <string>
#include <algorithm>

inline std::wstring GetKeyDisplayName(uint16_t vkCode, uint16_t scanCode, uint32_t flags) {
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

    LONG lParamKey = (scanCode << 16);
    if (flags & KEYEVENTF_EXTENDEDKEY) {
        lParamKey |= (1 << 24);
    }

    wchar_t nameBuf[64] = {};
    if (GetKeyNameTextW(lParamKey, nameBuf, 64) > 0) {
        return std::wstring(nameBuf);
    }

    if (vkCode >= 0x30 && vkCode <= 0x5A) {
        return std::wstring(1, static_cast<wchar_t>(vkCode));
    }

    return L"0x" + std::to_wstring(vkCode);
}

// Convert user-entered string back to (vkCode, scanCode, flags)
inline bool ParseKeyString(const std::wstring& input, uint16_t& outVk, uint16_t& outScan, uint32_t& outFlags) {
    if (input.empty()) return false;

    std::wstring lower = input;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    outFlags = 0;

    // Single character input like 'f', 's', '1', 'a'
    if (input.size() == 1) {
        wchar_t ch = input[0];
        SHORT res = VkKeyScanW(ch);
        if (res != -1) {
            outVk = static_cast<uint16_t>(LOBYTE(res));
            outScan = static_cast<uint16_t>(MapVirtualKeyW(outVk, MAPVK_VK_TO_VSC));
            return true;
        }
    }

    // Common named keys
    if (lower == L"space" || lower == L"spc") {
        outVk = VK_SPACE;
    } else if (lower == L"enter" || lower == L"return") {
        outVk = VK_RETURN;
    } else if (lower == L"tab") {
        outVk = VK_TAB;
    } else if (lower == L"esc" || lower == L"escape") {
        outVk = VK_ESCAPE;
    } else if (lower == L"backspace" || lower == L"back") {
        outVk = VK_BACK;
    } else if (lower == L"shift" || lower == L"lshift" || lower == L"left shift") {
        outVk = VK_LSHIFT;
    } else if (lower == L"rshift" || lower == L"right shift") {
        outVk = VK_RSHIFT;
    } else if (lower == L"ctrl" || lower == L"control" || lower == L"lctrl" || lower == L"left ctrl") {
        outVk = VK_LCONTROL;
    } else if (lower == L"rctrl" || lower == L"right ctrl") {
        outVk = VK_RCONTROL;
    } else if (lower == L"alt" || lower == L"lalt" || lower == L"left alt") {
        outVk = VK_LMENU;
    } else if (lower == L"ralt" || lower == L"right alt") {
        outVk = VK_RMENU;
    } else if (lower == L"up" || lower == L"up arrow") {
        outVk = VK_UP; outFlags |= KEYEVENTF_EXTENDEDKEY;
    } else if (lower == L"down" || lower == L"down arrow") {
        outVk = VK_DOWN; outFlags |= KEYEVENTF_EXTENDEDKEY;
    } else if (lower == L"left" || lower == L"left arrow") {
        outVk = VK_LEFT; outFlags |= KEYEVENTF_EXTENDEDKEY;
    } else if (lower == L"right" || lower == L"right arrow") {
        outVk = VK_RIGHT; outFlags |= KEYEVENTF_EXTENDEDKEY;
    } else if (lower == L"delete" || lower == L"del") {
        outVk = VK_DELETE; outFlags |= KEYEVENTF_EXTENDEDKEY;
    } else if (lower == L"insert" || lower == L"ins") {
        outVk = VK_INSERT; outFlags |= KEYEVENTF_EXTENDEDKEY;
    } else if (lower == L"home") {
        outVk = VK_HOME; outFlags |= KEYEVENTF_EXTENDEDKEY;
    } else if (lower == L"end") {
        outVk = VK_END; outFlags |= KEYEVENTF_EXTENDEDKEY;
    } else if (lower == L"page up" || lower == L"pgup") {
        outVk = VK_PRIOR; outFlags |= KEYEVENTF_EXTENDEDKEY;
    } else if (lower == L"page down" || lower == L"pgdn") {
        outVk = VK_NEXT; outFlags |= KEYEVENTF_EXTENDEDKEY;
    } else if (lower.size() >= 2 && lower[0] == L'f' && iswdigit(lower[1])) {
        int fNum = _wtoi(lower.c_str() + 1);
        if (fNum >= 1 && fNum <= 24) {
            outVk = static_cast<uint16_t>(VK_F1 + (fNum - 1));
        } else {
            return false;
        }
    } else {
        return false;
    }

    outScan = static_cast<uint16_t>(MapVirtualKeyW(outVk, MAPVK_VK_TO_VSC));
    return true;
}
