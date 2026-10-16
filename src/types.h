#pragma once
#include <windows.h>
#include <cstdint>

#pragma pack(push, 1)
struct KeyEvent {
    uint32_t delayMs;      // Delay in milliseconds before this event fires
    uint16_t vkCode;       // Virtual-Key code (e.g. VK_SPACE, 'A', VK_SHIFT)
    uint16_t scanCode;     // Hardware scan code for low-level compatibility
    uint32_t flags;        // Flags (e.g. KEYEVENTF_KEYUP, KEYEVENTF_EXTENDEDKEY)
};
#pragma pack(pop)
