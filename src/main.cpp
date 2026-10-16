#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <map>
#include "types.h"
#include "key_names.h"
#include "macro_recorder.h"
#include "macro_player.h"
#include "macro_file.h"

// Enable visual styles
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Control IDs
#define IDC_BTN_RECORD      101
#define IDC_BTN_PLAY        102
#define IDC_BTN_STOP        103
#define IDC_BTN_SAVE        104
#define IDC_BTN_OPEN        105
#define IDC_LBL_STATUS      106
#define IDC_EDIT_SPEED      107
#define IDC_EDIT_LOOPS      108
#define IDC_CHK_INFINITE    109
#define IDC_EDIT_TIME       110
#define IDC_CHK_TIMED       111
#define IDC_EDIT_INTERVAL   112
#define IDC_LBL_EVENTS      113
#define IDC_PROGRESS_BAR    114
#define IDC_BTN_TOGGLE_VIEW 115
#define IDC_LIST_TIMELINE   116
#define IDC_BTN_EDIT_DELAY  117
#define IDC_BTN_DEL_EVENT   118
#define IDC_BTN_CLEAR_ALL   119

// Hotkey IDs
#define HOTKEY_RECORD_STOP  1
#define HOTKEY_PLAY         2
#define HOTKEY_EMERGENCY    3

// Custom window messages
#define WM_USER_PHASE_UPDATE (WM_USER + 1)

static HWND g_hWnd = NULL;
static HWND g_hBtnRecord = NULL;
static HWND g_hBtnPlay = NULL;
static HWND g_hBtnStop = NULL;
static HWND g_hLblStatus = NULL;
static HWND g_hLblEvents = NULL;
static HWND g_hEditSpeed = NULL;
static HWND g_hEditLoops = NULL;
static HWND g_hChkInfinite = NULL;
static HWND g_hEditTime = NULL;
static HWND g_hChkTimed = NULL;
static HWND g_hEditInterval = NULL;
static HWND g_hProgressBar = NULL;
static HWND g_hBtnToggleView = NULL;
static HWND g_hListTimeline = NULL;
static HWND g_hBtnEditDelay = NULL;
static HWND g_hBtnDelEvent = NULL;
static HWND g_hBtnClearAll = NULL;

static bool g_isTimelineOpen = false;
static bool g_isUnsaved = false;
static PlaybackPhase g_currentPhase = PlaybackPhase::Stopped;
static int g_currentLoop = 0;
static int g_totalLoops = 0;
static uint32_t g_intervalRemainingMs = 0;
static uint32_t g_lastConfiguredIntervalMs = 0;

// Custom colors for status banner
static HBRUSH g_hBrushGreen = NULL;
static HBRUSH g_hBrushAmber = NULL;
static HBRUSH g_hBrushRed   = NULL;
static HBRUSH g_hBrushGray  = NULL;

// Forward declarations
bool SaveMacroDialog();
void RefreshTimelineList();
void EditTimelineCell(int itemIndex, int subItem);

void SetTimelineOpen(bool open) {
    g_isTimelineOpen = open;
    int width = 450;
    int height = g_isTimelineOpen ? 520 : 255;
    SetWindowPos(g_hWnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);

    SetWindowTextW(g_hBtnToggleView, g_isTimelineOpen ? L"▲ Hide Inspector" : L"▼ Timeline Inspector");

    int showCmd = g_isTimelineOpen ? SW_SHOW : SW_HIDE;
    ShowWindow(g_hListTimeline, showCmd);
    ShowWindow(g_hBtnEditDelay, showCmd);
    ShowWindow(g_hBtnDelEvent, showCmd);
    ShowWindow(g_hBtnClearAll, showCmd);

    if (g_isTimelineOpen) {
        RefreshTimelineList();
    }
}

void RefreshTimelineList() {
    if (!g_isTimelineOpen || !g_hListTimeline) return;

    SendMessageW(g_hListTimeline, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(g_hListTimeline);

    const auto& events = MacroRecorder::Instance().GetEvents();

    // Calculate hold duration by matching key-down events with their subsequent key-up
    std::vector<std::wstring> holdDurations(events.size(), L"-");
    std::map<uint16_t, std::pair<uint64_t, size_t>> activePresses;

    uint64_t runningTimeMs = 0;
    for (size_t i = 0; i < events.size(); i++) {
        runningTimeMs += events[i].delayMs;
        bool isUp = (events[i].flags & KEYEVENTF_KEYUP);

        if (!isUp) {
            activePresses[events[i].vkCode] = { runningTimeMs, i };
        } else {
            auto it = activePresses.find(events[i].vkCode);
            if (it != activePresses.end()) {
                uint64_t holdMs = runningTimeMs - it->second.first;
                holdDurations[i] = std::to_wstring(holdMs) + L" ms";
                activePresses.erase(it);
            }
        }
    }

    // Populate ListView
    for (size_t i = 0; i < events.size(); i++) {
        const auto& ev = events[i];
        bool isUp = (ev.flags & KEYEVENTF_KEYUP);

        LVITEMW lvi = {};
        lvi.mask = LVIF_TEXT;
        lvi.iItem = static_cast<int>(i);

        // Column 0: Index (#)
        std::wstring idxStr = std::to_wstring(i + 1);
        lvi.pszText = const_cast<wchar_t*>(idxStr.c_str());
        ListView_InsertItem(g_hListTimeline, &lvi);

        // Column 1: Key Name
        std::wstring keyName = GetKeyDisplayName(ev.vkCode, ev.scanCode, ev.flags);
        ListView_SetItemText(g_hListTimeline, static_cast<int>(i), 1, const_cast<wchar_t*>(keyName.c_str()));

        // Column 2: Action
        std::wstring actionStr = isUp ? L"Up" : L"Down";
        ListView_SetItemText(g_hListTimeline, static_cast<int>(i), 2, const_cast<wchar_t*>(actionStr.c_str()));

        // Column 3: Hold Duration
        ListView_SetItemText(g_hListTimeline, static_cast<int>(i), 3, const_cast<wchar_t*>(holdDurations[i].c_str()));

        // Column 4: Delay After (Interval before this event)
        std::wstring delayStr = std::to_wstring(ev.delayMs) + L" ms";
        ListView_SetItemText(g_hListTimeline, static_cast<int>(i), 4, const_cast<wchar_t*>(delayStr.c_str()));
    }

    SendMessageW(g_hListTimeline, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(g_hListTimeline, NULL, TRUE);
}

void UpdateUIStatus() {
    bool isRec = MacroRecorder::Instance().IsRecording();
    bool isPlay = MacroPlayer::Instance().IsPlaying();
    size_t count = MacroRecorder::Instance().GetEventCount();

    if (isRec) {
        SetWindowTextW(g_hLblStatus, L"● RECORDING... [F8 to Stop]");
        SetWindowTextW(g_hBtnRecord, L"Stop (F8)");
        EnableWindow(g_hBtnPlay, FALSE);
        EnableWindow(g_hBtnStop, TRUE);
        SendMessageW(g_hProgressBar, PBM_SETPOS, 0, 0);
    } else if (isPlay) {
        EnableWindow(g_hBtnRecord, FALSE);
        EnableWindow(g_hBtnPlay, FALSE);
        EnableWindow(g_hBtnStop, TRUE);

        if (g_currentPhase == PlaybackPhase::Playing) {
            std::wstring statusText = L"▶ PLAYING - Loop " + std::to_wstring(g_currentLoop);
            if (g_totalLoops > 0) {
                statusText += L" / " + std::to_wstring(g_totalLoops);
            } else {
                statusText += L" (Continuous)";
            }
            SetWindowTextW(g_hLblStatus, statusText.c_str());
            SendMessageW(g_hProgressBar, PBM_SETPOS, 100, 0);
        } else if (g_currentPhase == PlaybackPhase::Interval) {
            double remSec = static_cast<double>(g_intervalRemainingMs) / 1000.0;
            wchar_t buf[128];
            swprintf_s(buf, L"⏳ INTERVAL: Waiting %.1fs...", remSec);
            SetWindowTextW(g_hLblStatus, buf);

            if (g_lastConfiguredIntervalMs > 0) {
                int progress = static_cast<int>(100.0 - (static_cast<double>(g_intervalRemainingMs) / static_cast<double>(g_lastConfiguredIntervalMs)) * 100.0);
                if (progress < 0) progress = 0;
                if (progress > 100) progress = 100;
                SendMessageW(g_hProgressBar, PBM_SETPOS, progress, 0);
            }
        }
    } else {
        SetWindowTextW(g_hLblStatus, L"⏹ Stopped / Ready");
        SetWindowTextW(g_hBtnRecord, L"Record (F8)");
        EnableWindow(g_hBtnRecord, TRUE);
        EnableWindow(g_hBtnPlay, count > 0 ? TRUE : FALSE);
        EnableWindow(g_hBtnStop, FALSE);
        SendMessageW(g_hProgressBar, PBM_SETPOS, 0, 0);
    }

    std::wstring countStr = L"Events: " + std::to_wstring(count);
    if (g_isUnsaved && count > 0) {
        countStr += L" *";
    }
    SetWindowTextW(g_hLblEvents, countStr.c_str());

    InvalidateRect(g_hLblStatus, NULL, TRUE);
}

// Prompts user if there are unsaved changes
bool PromptUnsavedChangesIfNeeded() {
    if (!g_isUnsaved || MacroRecorder::Instance().GetEventCount() == 0) {
        return true;
    }

    int result = MessageBoxW(
        g_hWnd,
        L"You have unsaved recorded keystrokes.\n\n"
        L"- Click 'Yes' to Save now\n"
        L"- Click 'No' to Discard and proceed\n"
        L"- Click 'Cancel' to Go Back",
        L"Unsaved Macro",
        MB_YESNOCANCEL | MB_ICONWARNING
    );

    if (result == IDYES) {
        bool saved = SaveMacroDialog();
        return saved;
    } else if (result == IDNO) {
        g_isUnsaved = false;
        return true;
    } else {
        return false;
    }
}

void StartRecording() {
    if (MacroPlayer::Instance().IsPlaying()) return;

    if (!PromptUnsavedChangesIfNeeded()) {
        return;
    }

    if (MacroRecorder::Instance().Start()) {
        g_isUnsaved = false;
        UpdateUIStatus();
        RefreshTimelineList();
    }
}

void StopRecording() {
    MacroRecorder::Instance().Stop();
    if (MacroRecorder::Instance().GetEventCount() > 0) {
        g_isUnsaved = true;
    }
    UpdateUIStatus();
    RefreshTimelineList();
}

void ToggleRecording() {
    if (MacroRecorder::Instance().IsRecording()) {
        StopRecording();
    } else {
        StartRecording();
    }
}

void StartPlayback() {
    if (MacroRecorder::Instance().IsRecording()) return;
    if (MacroPlayer::Instance().IsPlaying()) return;

    const auto& events = MacroRecorder::Instance().GetEvents();
    if (events.empty()) return;

    wchar_t speedBuf[32];
    GetWindowTextW(g_hEditSpeed, speedBuf, 32);
    double speed = _wtof(speedBuf);
    if (speed <= 0.05) speed = 1.0;

    int loops = 1;
    bool isInfinite = (SendMessageW(g_hChkInfinite, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (isInfinite) {
        loops = 0;
    } else {
        wchar_t loopBuf[32];
        GetWindowTextW(g_hEditLoops, loopBuf, 32);
        loops = _wtoi(loopBuf);
        if (loops < 1) loops = 1;
    }

    uint32_t durationLimitSec = 0;
    bool isTimed = (SendMessageW(g_hChkTimed, BM_GETCHECK, 0, 0) == BST_CHECKED);
    if (isTimed) {
        wchar_t timeBuf[32];
        GetWindowTextW(g_hEditTime, timeBuf, 32);
        int seconds = _wtoi(timeBuf);
        if (seconds > 0) {
            durationLimitSec = static_cast<uint32_t>(seconds);
        }
    }

    wchar_t intervalBuf[32];
    GetWindowTextW(g_hEditInterval, intervalBuf, 32);
    int intervalMs = _wtoi(intervalBuf);
    if (intervalMs < 0) intervalMs = 0;
    g_lastConfiguredIntervalMs = static_cast<uint32_t>(intervalMs);

    g_currentPhase = PlaybackPhase::Playing;
    g_currentLoop = 1;
    g_totalLoops = loops;

    MacroPlayer::Instance().Start(events, speed, loops, durationLimitSec, static_cast<uint32_t>(intervalMs));
    UpdateUIStatus();
}

void StopPlayback() {
    if (MacroPlayer::Instance().IsPlaying()) {
        MacroPlayer::Instance().Stop();
    }
    if (MacroRecorder::Instance().IsRecording()) {
        MacroRecorder::Instance().Stop();
    }
    g_currentPhase = PlaybackPhase::Stopped;
    UpdateUIStatus();
}

bool SaveMacroDialog() {
    const auto& events = MacroRecorder::Instance().GetEvents();
    if (events.empty()) {
        MessageBoxW(g_hWnd, L"No recorded events to save!", L"Info", MB_OK | MB_ICONINFORMATION);
        return false;
    }

    wchar_t szFile[MAX_PATH] = L"macro.krec";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hWnd;
    ofn.lpstrFilter = L"Keyboard Macro (*.krec)\0*.krec\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"krec";

    if (GetSaveFileNameW(&ofn)) {
        if (MacroFile::SaveToFile(szFile, events)) {
            g_isUnsaved = false;
            UpdateUIStatus();
            MessageBoxW(g_hWnd, L"Macro saved successfully!", L"Success", MB_OK | MB_ICONINFORMATION);
            return true;
        } else {
            MessageBoxW(g_hWnd, L"Failed to save file.", L"Error", MB_OK | MB_ICONERROR);
            return false;
        }
    }
    return false;
}

void OpenMacroDialog() {
    if (MacroPlayer::Instance().IsPlaying() || MacroRecorder::Instance().IsRecording()) {
        return;
    }

    if (!PromptUnsavedChangesIfNeeded()) {
        return;
    }

    wchar_t szFile[MAX_PATH] = L"";
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_hWnd;
    ofn.lpstrFilter = L"Keyboard Macro (*.krec)\0*.krec\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn)) {
        std::vector<KeyEvent> loaded;
        if (MacroFile::LoadFromFile(szFile, loaded)) {
            MacroRecorder::Instance().SetEvents(loaded);
            g_isUnsaved = false;
            UpdateUIStatus();
            RefreshTimelineList();
            MessageBoxW(g_hWnd, (L"Loaded " + std::to_wstring(loaded.size()) + L" events.").c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBoxW(g_hWnd, L"Failed to load valid macro file.", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

// Edit delay / time box directly by double clicking
void EditTimelineCell(int itemIndex, int subItem) {
    auto events = MacroRecorder::Instance().GetEvents();
    if (itemIndex < 0 || itemIndex >= static_cast<int>(events.size())) return;

    // Subitem 3: Hold Duration (for key-up events)
    // Subitem 4 (or default): Delay Before (Interval)
    bool isHoldDuration = (subItem == 3);
    uint32_t currentVal = 0;
    std::wstring title = L"Edit Delay (ms)";
    std::wstring label = L"Delay before event (ms):";

    if (isHoldDuration) {
        // Find corresponding key-down event backwards
        uint16_t vk = events[itemIndex].vkCode;
        bool isUp = (events[itemIndex].flags & KEYEVENTF_KEYUP);
        if (!isUp) {
            MessageBoxW(g_hWnd, L"To edit hold duration, double-click the 'Up' row for this key, or edit the Delay column directly.", L"Tip", MB_OK | MB_ICONINFORMATION);
            return;
        }

        // Calculate current hold duration
        int downIdx = -1;
        uint32_t holdTime = 0;
        for (int i = itemIndex - 1; i >= 0; i--) {
            holdTime += events[i + 1].delayMs;
            if (events[i].vkCode == vk && !(events[i].flags & KEYEVENTF_KEYUP)) {
                downIdx = i;
                break;
            }
        }
        if (downIdx == -1) {
            MessageBoxW(g_hWnd, L"Cannot find matching Key Down event.", L"Error", MB_OK | MB_ICONERROR);
            return;
        }

        currentVal = holdTime;
        title = L"Edit Hold Duration";
        label = L"New Key Hold Duration (ms):";
    } else {
        currentVal = events[itemIndex].delayMs;
    }

    HWND hInputWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        L"#32770", title.c_str(),
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        0, 0, 260, 135, g_hWnd, NULL, NULL, NULL
    );

    RECT parentRect;
    GetWindowRect(g_hWnd, &parentRect);
    SetWindowPos(hInputWnd, NULL,
        parentRect.left + 90, parentRect.top + 140,
        260, 135, SWP_NOZORDER);

    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    HWND hLbl = CreateWindowW(L"STATIC", label.c_str(), WS_VISIBLE | WS_CHILD,
        20, 15, 210, 20, hInputWnd, NULL, NULL, NULL);
    SendMessageW(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hEdit = CreateWindowW(L"EDIT", std::to_wstring(currentVal).c_str(),
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_AUTOHSCROLL,
        20, 38, 205, 22, hInputWnd, (HMENU)201, NULL, NULL);
    SendMessageW(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnOk = CreateWindowW(L"BUTTON", L"OK", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        50, 68, 70, 24, hInputWnd, (HMENU)IDOK, NULL, NULL);
    SendMessageW(hBtnOk, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hBtnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        135, 68, 70, 24, hInputWnd, (HMENU)IDCANCEL, NULL, NULL);
    SendMessageW(hBtnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

    EnableWindow(g_hWnd, FALSE);
    SetFocus(hEdit);
    SendMessageW(hEdit, EM_SETSEL, 0, -1);

    MSG msg = {};
    bool done = false;
    while (!done && GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
            done = true;
            break;
        }
        if (msg.message == WM_COMMAND) {
            int id = LOWORD(msg.wParam);
            if (id == IDOK) {
                wchar_t buf[32];
                GetWindowTextW(hEdit, buf, 32);
                int newVal = _wtoi(buf);
                if (newVal >= 0) {
                    if (isHoldDuration) {
                        // Adjust the key up event delay so that the hold time matches newVal
                        uint16_t vk = events[itemIndex].vkCode;
                        int downIdx = -1;
                        uint32_t intermediateDelay = 0;
                        for (int i = itemIndex - 1; i >= 0; i--) {
                            if (events[i].vkCode == vk && !(events[i].flags & KEYEVENTF_KEYUP)) {
                                downIdx = i;
                                break;
                            }
                            intermediateDelay += events[i + 1].delayMs;
                        }
                        if (downIdx != -1) {
                            if (static_cast<uint32_t>(newVal) >= intermediateDelay) {
                                events[downIdx + 1].delayMs = static_cast<uint32_t>(newVal) - intermediateDelay;
                            } else {
                                events[itemIndex].delayMs = static_cast<uint32_t>(newVal);
                            }
                        }
                    } else {
                        events[itemIndex].delayMs = static_cast<uint32_t>(newVal);
                    }

                    MacroRecorder::Instance().SetEvents(events);
                    g_isUnsaved = true;
                    UpdateUIStatus();
                    RefreshTimelineList();
                }
                done = true;
                break;
            } else if (id == IDCANCEL) {
                done = true;
                break;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    EnableWindow(g_hWnd, TRUE);
    DestroyWindow(hInputWnd);
    SetFocus(g_hListTimeline);
}

void PromptEditDelay() {
    int selected = ListView_GetNextItem(g_hListTimeline, -1, LVNI_SELECTED);
    if (selected < 0) {
        MessageBoxW(g_hWnd, L"Please select an event from the list to edit its delay.", L"No Selection", MB_OK | MB_ICONINFORMATION);
        return;
    }
    EditTimelineCell(selected, 4);
}

void DeleteSelectedEvent() {
    int selected = ListView_GetNextItem(g_hListTimeline, -1, LVNI_SELECTED);
    if (selected < 0) {
        MessageBoxW(g_hWnd, L"Please select an event to delete.", L"No Selection", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::vector<KeyEvent> updated = MacroRecorder::Instance().GetEvents();
    if (selected < static_cast<int>(updated.size())) {
        updated.erase(updated.begin() + selected);
        MacroRecorder::Instance().SetEvents(updated);
        g_isUnsaved = true;
        UpdateUIStatus();
        RefreshTimelineList();

        int newCount = static_cast<int>(updated.size());
        if (newCount > 0) {
            int newSel = selected >= newCount ? newCount - 1 : selected;
            ListView_SetItemState(g_hListTimeline, newSel, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
        }
    }
}

void ClearAllEvents() {
    if (MacroRecorder::Instance().GetEventCount() == 0) return;

    if (MessageBoxW(g_hWnd, L"Are you sure you want to clear all recorded events?", L"Clear All", MB_YESNO | MB_ICONQUESTION) == IDYES) {
        MacroRecorder::Instance().SetEvents({});
        g_isUnsaved = true;
        UpdateUIStatus();
        RefreshTimelineList();
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

        g_hBrushGreen = CreateSolidBrush(RGB(220, 255, 220));
        g_hBrushAmber = CreateSolidBrush(RGB(255, 245, 200));
        g_hBrushRed   = CreateSolidBrush(RGB(255, 225, 225));
        g_hBrushGray  = CreateSolidBrush(RGB(240, 240, 240));

        // Top Row: Action Buttons
        g_hBtnRecord = CreateWindowW(L"BUTTON", L"Record (F8)", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            15, 12, 120, 34, hWnd, (HMENU)IDC_BTN_RECORD, NULL, NULL);

        g_hBtnPlay = CreateWindowW(L"BUTTON", L"Play (F9)", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            145, 12, 120, 34, hWnd, (HMENU)IDC_BTN_PLAY, NULL, NULL);

        g_hBtnStop = CreateWindowW(L"BUTTON", L"Stop (Esc)", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            275, 12, 120, 34, hWnd, (HMENU)IDC_BTN_STOP, NULL, NULL);

        // Second Row: Save / Open / Speed / Interval
        CreateWindowW(L"BUTTON", L"Save...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            15, 54, 70, 26, hWnd, (HMENU)IDC_BTN_SAVE, NULL, NULL);

        CreateWindowW(L"BUTTON", L"Open...", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            90, 54, 70, 26, hWnd, (HMENU)IDC_BTN_OPEN, NULL, NULL);

        HWND hSpeedLbl = CreateWindowW(L"STATIC", L"Speed:", WS_VISIBLE | WS_CHILD,
            175, 58, 45, 20, hWnd, NULL, NULL, NULL);
        g_hEditSpeed = CreateWindowW(L"EDIT", L"1.0", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
            225, 56, 40, 22, hWnd, (HMENU)IDC_EDIT_SPEED, NULL, NULL);

        HWND hIntervalLbl = CreateWindowW(L"STATIC", L"Interval (ms):", WS_VISIBLE | WS_CHILD,
            275, 58, 80, 20, hWnd, NULL, NULL, NULL);
        g_hEditInterval = CreateWindowW(L"EDIT", L"0", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            360, 56, 40, 22, hWnd, (HMENU)IDC_EDIT_INTERVAL, NULL, NULL);

        // Third Row: Loops & Infinite
        HWND hLoopsLbl = CreateWindowW(L"STATIC", L"Loops:", WS_VISIBLE | WS_CHILD,
            15, 92, 45, 20, hWnd, NULL, NULL, NULL);
        g_hEditLoops = CreateWindowW(L"EDIT", L"1", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            65, 90, 45, 22, hWnd, (HMENU)IDC_EDIT_LOOPS, NULL, NULL);

        g_hChkInfinite = CreateWindowW(L"BUTTON", L"Continuous Loop (until stopped)", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            120, 90, 220, 22, hWnd, (HMENU)IDC_CHK_INFINITE, NULL, NULL);

        // Fourth Row: Timed Duration
        g_hChkTimed = CreateWindowW(L"BUTTON", L"Stop after (sec):", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            15, 122, 125, 22, hWnd, (HMENU)IDC_CHK_TIMED, NULL, NULL);
        g_hEditTime = CreateWindowW(L"EDIT", L"60", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
            145, 122, 50, 22, hWnd, (HMENU)IDC_EDIT_TIME, NULL, NULL);
        EnableWindow(g_hEditTime, FALSE);

        // Toggle Timeline Button
        g_hBtnToggleView = CreateWindowW(L"BUTTON", L"▼ Timeline Inspector", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            260, 120, 140, 26, hWnd, (HMENU)IDC_BTN_TOGGLE_VIEW, NULL, NULL);

        // Fifth Row: Progress Bar for visual feedback
        g_hProgressBar = CreateWindowExW(0, PROGRESS_CLASSW, NULL,
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            15, 154, 385, 10, hWnd, (HMENU)IDC_PROGRESS_BAR, NULL, NULL);
        SendMessageW(g_hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

        // Sixth Row: Visual Banner Status label & Events
        g_hLblStatus = CreateWindowW(L"STATIC", L"⏹ Stopped / Ready", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_LEFT,
            15, 170, 250, 24, hWnd, (HMENU)IDC_LBL_STATUS, NULL, NULL);

        g_hLblEvents = CreateWindowW(L"STATIC", L"Events: 0", WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_RIGHT,
            270, 170, 130, 24, hWnd, (HMENU)IDC_LBL_EVENTS, NULL, NULL);

        // --- Collapsible Inspector Controls (Rows below) ---
        g_hListTimeline = CreateWindowExW(
            WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
            15, 205, 385, 215, hWnd, (HMENU)IDC_LIST_TIMELINE, NULL, NULL
        );
        ListView_SetExtendedListViewStyle(g_hListTimeline, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        // Insert Columns
        LVCOLUMNW lvc = {};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        lvc.cx = 40;  lvc.pszText = const_cast<wchar_t*>(L"#");             ListView_InsertColumn(g_hListTimeline, 0, &lvc);
        lvc.cx = 95;  lvc.pszText = const_cast<wchar_t*>(L"Key");           ListView_InsertColumn(g_hListTimeline, 1, &lvc);
        lvc.cx = 65;  lvc.pszText = const_cast<wchar_t*>(L"Action");        ListView_InsertColumn(g_hListTimeline, 2, &lvc);
        lvc.cx = 95;  lvc.pszText = const_cast<wchar_t*>(L"Hold Duration"); ListView_InsertColumn(g_hListTimeline, 3, &lvc);
        lvc.cx = 85;  lvc.pszText = const_cast<wchar_t*>(L"Delay After");   ListView_InsertColumn(g_hListTimeline, 4, &lvc);

        g_hBtnEditDelay = CreateWindowW(L"BUTTON", L"Edit Delay...", WS_CHILD | BS_PUSHBUTTON,
            15, 430, 100, 28, hWnd, (HMENU)IDC_BTN_EDIT_DELAY, NULL, NULL);

        g_hBtnDelEvent = CreateWindowW(L"BUTTON", L"Delete Event", WS_CHILD | BS_PUSHBUTTON,
            125, 430, 100, 28, hWnd, (HMENU)IDC_BTN_DEL_EVENT, NULL, NULL);

        g_hBtnClearAll = CreateWindowW(L"BUTTON", L"Clear All", WS_CHILD | BS_PUSHBUTTON,
            305, 430, 95, 28, hWnd, (HMENU)IDC_BTN_CLEAR_ALL, NULL, NULL);

        // Apply clean standard font
        const HWND controls[] = {
            g_hBtnRecord, g_hBtnPlay, g_hBtnStop, g_hLblStatus, g_hLblEvents,
            hSpeedLbl, g_hEditSpeed, hLoopsLbl, g_hEditLoops,
            g_hChkInfinite, g_hChkTimed, g_hEditTime,
            hIntervalLbl, g_hEditInterval, g_hBtnToggleView,
            g_hListTimeline, g_hBtnEditDelay, g_hBtnDelEvent, g_hBtnClearAll
        };
        for (HWND ctrl : controls) {
            SendMessageW(ctrl, WM_SETFONT, (WPARAM)hFont, TRUE);
        }

        // Register Global Hotkeys
        RegisterHotKey(hWnd, HOTKEY_RECORD_STOP, 0, VK_F8);
        RegisterHotKey(hWnd, HOTKEY_PLAY, 0, VK_F9);
        RegisterHotKey(hWnd, HOTKEY_EMERGENCY, 0, VK_ESCAPE);

        MacroPlayer::Instance().SetPhaseCallback([hWnd](PlaybackPhase phase, int currentLoop, int totalLoops, uint32_t intervalRemainingMs) {
            PostMessageW(hWnd, WM_USER_PHASE_UPDATE, static_cast<WPARAM>(phase), MAKELPARAM(currentLoop, intervalRemainingMs > 65535 ? 65535 : intervalRemainingMs));
            g_currentPhase = phase;
            g_currentLoop = currentLoop;
            g_totalLoops = totalLoops;
            g_intervalRemainingMs = intervalRemainingMs;
        });

        UpdateUIStatus();
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        HWND hwndCtl = (HWND)lParam;
        if (hwndCtl == g_hLblStatus) {
            SetBkMode(hdc, TRANSPARENT);
            if (MacroRecorder::Instance().IsRecording()) {
                SetTextColor(hdc, RGB(180, 0, 0));
                return (INT_PTR)g_hBrushRed;
            } else if (MacroPlayer::Instance().IsPlaying()) {
                if (g_currentPhase == PlaybackPhase::Interval) {
                    SetTextColor(hdc, RGB(160, 100, 0));
                    return (INT_PTR)g_hBrushAmber;
                } else {
                    SetTextColor(hdc, RGB(0, 130, 0));
                    return (INT_PTR)g_hBrushGreen;
                }
            } else {
                SetTextColor(hdc, RGB(60, 60, 60));
                return (INT_PTR)g_hBrushGray;
            }
        }
        break;
    }

    case WM_HOTKEY: {
        int id = (int)wParam;
        if (id == HOTKEY_RECORD_STOP) {
            ToggleRecording();
        } else if (id == HOTKEY_PLAY) {
            StartPlayback();
        } else if (id == HOTKEY_EMERGENCY) {
            StopPlayback();
        }
        break;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_BTN_RECORD:
            ToggleRecording();
            break;
        case IDC_BTN_PLAY:
            StartPlayback();
            break;
        case IDC_BTN_STOP:
            StopPlayback();
            break;
        case IDC_BTN_SAVE:
            SaveMacroDialog();
            break;
        case IDC_BTN_OPEN:
            OpenMacroDialog();
            break;
        case IDC_CHK_INFINITE: {
            bool checked = (SendMessageW(g_hChkInfinite, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(g_hEditLoops, !checked);
            break;
        }
        case IDC_CHK_TIMED: {
            bool checked = (SendMessageW(g_hChkTimed, BM_GETCHECK, 0, 0) == BST_CHECKED);
            EnableWindow(g_hEditTime, checked);
            break;
        }
        case IDC_BTN_TOGGLE_VIEW:
            SetTimelineOpen(!g_isTimelineOpen);
            break;
        case IDC_BTN_EDIT_DELAY:
            PromptEditDelay();
            break;
        case IDC_BTN_DEL_EVENT:
            DeleteSelectedEvent();
            break;
        case IDC_BTN_CLEAR_ALL:
            ClearAllEvents();
            break;
        }
        break;
    }

    case WM_NOTIFY: {
        LPNMHDR lpnmh = (LPNMHDR)lParam;
        if (lpnmh->idFrom == IDC_LIST_TIMELINE && lpnmh->code == NM_DBLCLK) {
            LPNMITEMACTIVATE lpnmitem = (LPNMITEMACTIVATE)lParam;
            if (lpnmitem->iItem >= 0) {
                // Pass the specific subitem column that was double-clicked
                EditTimelineCell(lpnmitem->iItem, lpnmitem->iSubItem);
            }
        }
        break;
    }

    case WM_USER_PHASE_UPDATE: {
        UpdateUIStatus();
        break;
    }

    case WM_CLOSE: {
        if (!PromptUnsavedChangesIfNeeded()) {
            return 0;
        }
        DestroyWindow(hWnd);
        break;
    }

    case WM_DESTROY:
        UnregisterHotKey(hWnd, HOTKEY_RECORD_STOP);
        UnregisterHotKey(hWnd, HOTKEY_PLAY);
        UnregisterHotKey(hWnd, HOTKEY_EMERGENCY);
        if (g_hBrushGreen) DeleteObject(g_hBrushGreen);
        if (g_hBrushAmber) DeleteObject(g_hBrushAmber);
        if (g_hBrushRed)   DeleteObject(g_hBrushRed);
        if (g_hBrushGray)  DeleteObject(g_hBrushGray);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow) {
    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_STANDARD_CLASSES | ICC_PROGRESS_CLASS | ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icex);

    const wchar_t CLASS_NAME[] = L"KeyMacroMainWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    RegisterClassW(&wc);

    HWND hWnd = CreateWindowExW(
        WS_EX_TOPMOST,
        CLASS_NAME,
        L"KeyTask - Keyboard Macro Recorder",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        450, 255,
        NULL, NULL, hInstance, NULL
    );

    if (!hWnd) return 0;

    g_hWnd = hWnd;
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg = {};
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
