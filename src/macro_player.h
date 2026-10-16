#pragma once
#include <windows.h>
#include <vector>
#include <atomic>
#include <thread>
#include <functional>
#include "types.h"

enum class PlaybackPhase {
    Stopped = 0,
    Playing = 1,
    Interval = 2
};

class MacroPlayer {
public:
    static MacroPlayer& Instance() {
        static MacroPlayer instance;
        return instance;
    }

    // Reports phase changes and progress info (currentLoop, totalLoops, intervalRemainingMs)
    using PhaseCallback = std::function<void(PlaybackPhase phase, int currentLoop, int totalLoops, uint32_t intervalRemainingMs)>;

    void SetPhaseCallback(PhaseCallback cb) {
        m_callback = cb;
    }

    bool Start(const std::vector<KeyEvent>& events, double speedMultiplier = 1.0, int loopCount = 1, uint32_t durationLimitSec = 0, uint32_t loopDelayMs = 0) {
        if (m_isPlaying.load() || events.empty()) return false;

        m_stopRequested.store(false);
        m_isPlaying.store(true);

        if (m_thread.joinable()) {
            m_thread.join();
        }

        m_thread = std::thread([this, events, speedMultiplier, loopCount, durationLimitSec, loopDelayMs]() {
            PlaybackLoop(events, speedMultiplier, loopCount, durationLimitSec, loopDelayMs);
            m_isPlaying.store(false);
            if (m_callback) m_callback(PlaybackPhase::Stopped, 0, 0, 0);
        });

        return true;
    }

    void Stop() {
        if (m_isPlaying.load()) {
            m_stopRequested.store(true);
        }
    }

    bool IsPlaying() const {
        return m_isPlaying.load();
    }

    ~MacroPlayer() {
        Stop();
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

private:
    MacroPlayer() : m_isPlaying(false), m_stopRequested(false) {}

    // High precision sleep using waitable timer or QPC
    static void PreciseSleep(uint32_t ms, const std::atomic<bool>& stopFlag) {
        if (ms == 0) return;

        LARGE_INTEGER freq, start, cur;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&start);

        double targetTicks = (static_cast<double>(ms) / 1000.0) * static_cast<double>(freq.QuadPart);

        // For long intervals, yield/sleep coarsely to avoid 100% CPU usage
        if (ms > 15) {
            DWORD coarseMs = ms - 10;
            DWORD startTick = GetTickCount();
            while (GetTickCount() - startTick < coarseMs) {
                if (stopFlag.load()) return;
                Sleep(1);
            }
        }

        // Spin-wait for the final precise microseconds
        while (!stopFlag.load()) {
            QueryPerformanceCounter(&cur);
            if (static_cast<double>(cur.QuadPart - start.QuadPart) >= targetTicks) {
                break;
            }
        }
    }

    void PlaybackLoop(const std::vector<KeyEvent>& events, double speedMultiplier, int loopCount, uint32_t durationLimitSec, uint32_t loopDelayMs) {
        if (speedMultiplier <= 0.05) speedMultiplier = 1.0;

        LARGE_INTEGER freq, playbackStart, cur;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&playbackStart);

        int loopsCompleted = 0;
        // loopCount <= 0 means continuous/infinite loop until stopped
        while (!m_stopRequested.load()) {
            loopsCompleted++;
            if (m_callback) {
                m_callback(PlaybackPhase::Playing, loopsCompleted, loopCount, 0);
            }

            for (const auto& ev : events) {
                if (m_stopRequested.load()) break;

                // Check duration limit before waiting
                if (durationLimitSec > 0) {
                    QueryPerformanceCounter(&cur);
                    double elapsed = static_cast<double>(cur.QuadPart - playbackStart.QuadPart) / static_cast<double>(freq.QuadPart);
                    if (elapsed >= durationLimitSec) {
                        return;
                    }
                }

                uint32_t adjustedDelay = static_cast<uint32_t>(static_cast<double>(ev.delayMs) / speedMultiplier);
                PreciseSleep(adjustedDelay, m_stopRequested);

                if (m_stopRequested.load()) break;

                // Check duration limit after waiting
                if (durationLimitSec > 0) {
                    QueryPerformanceCounter(&cur);
                    double elapsed = static_cast<double>(cur.QuadPart - playbackStart.QuadPart) / static_cast<double>(freq.QuadPart);
                    if (elapsed >= durationLimitSec) {
                        return;
                    }
                }

                INPUT input = {};
                input.type = INPUT_KEYBOARD;
                input.ki.wVk = ev.vkCode;
                input.ki.wScan = ev.scanCode;
                input.ki.dwFlags = ev.flags;
                input.ki.time = 0;
                input.ki.dwExtraInfo = 0;

                SendInput(1, &input, sizeof(INPUT));
            }

            if (loopCount > 0 && loopsCompleted >= loopCount) {
                break;
            }

            // Check duration limit at end of cycle
            if (durationLimitSec > 0) {
                QueryPerformanceCounter(&cur);
                double elapsed = static_cast<double>(cur.QuadPart - playbackStart.QuadPart) / static_cast<double>(freq.QuadPart);
                if (elapsed >= durationLimitSec) {
                    break;
                }
            }

            // Delay between repetitions with live countdown visual cues
            if (loopDelayMs > 0 && !m_stopRequested.load()) {
                LARGE_INTEGER intervalStart, intervalNow;
                QueryPerformanceCounter(&intervalStart);

                while (!m_stopRequested.load()) {
                    QueryPerformanceCounter(&intervalNow);
                    double elapsedMs = (static_cast<double>(intervalNow.QuadPart - intervalStart.QuadPart) / static_cast<double>(freq.QuadPart)) * 1000.0;
                    if (elapsedMs >= loopDelayMs) {
                        break;
                    }

                    uint32_t remainingMs = static_cast<uint32_t>(loopDelayMs - elapsedMs);
                    if (m_callback) {
                        m_callback(PlaybackPhase::Interval, loopsCompleted, loopCount, remainingMs);
                    }

                    uint32_t sleepChunk = remainingMs > 50 ? 50 : remainingMs;
                    PreciseSleep(sleepChunk, m_stopRequested);
                }
            }
        }
    }

    std::atomic<bool> m_isPlaying;
    std::atomic<bool> m_stopRequested;
    std::thread m_thread;
    PhaseCallback m_callback;
};
