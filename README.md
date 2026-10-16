# KeyTask - Keyboard-Only Macro Recorder & Player

A lightweight, high-precision native Windows C++ tool (like TinyTask, but dedicated exclusively to keyboard inputs).

## Key Features

1. **Duration & Timing Accuracy**:
   - Uses Windows High-Resolution Performance Counter (`QueryPerformanceCounter`) with sub-millisecond precision.
   - Accurately captures both press (`WM_KEYDOWN`) and release (`WM_KEYUP`) timings to reproduce hold durations (crucial for games, combos, and held keys).
2. **Collapsible Macro Event Timeline / Inspector**:
   - Click **`▼ Timeline Inspector`** to open the interactive event table:
     - `#`: Event index in sequence
     - `Key`: Human-readable key name (e.g. `Ctrl`, `Shift`, `W`, `Space`, `Enter`)
     - `Action`: `Down` or `Up`
     - `Hold Duration`: Calculated holding time in milliseconds (e.g., `120 ms`)
     - `Delay After`: Interval in milliseconds before the event
   - **Edit Delay**: Select any event and click **Edit Delay...** (or double-click the row) to tweak dead time or intervals.
   - **Delete Event**: Remove accidental key presses.
   - **Clear All**: Reset the macro timeline.
3. **Visual Cues & Phase Progress**:
   - **Green Banner**: Active playback with loop counter (`▶ PLAYING - Loop 1 / 5`).
   - **Amber Banner & Progress Bar**: Repetition interval countdown showing real-time remaining seconds (`⏳ INTERVAL: Waiting 1.5s...`).
   - **Red Banner**: Active recording (`● RECORDING... [F8 to Stop]`).
   - **Gray Banner**: Stopped / Ready state.
4. **Delay Between Repetitions (`Interval (ms)`)**:
   - Set an explicit delay in milliseconds between consecutive macro loops.
5. **Continuous / Infinite Looping**:
   - Check **"Continuous Loop (until stopped)"** to replay indefinitely until you press **`Esc`** or click **Stop**.
6. **Timed Repetition**:
   - Check **"Stop after (sec):"** to automatically finish playback after the given duration.
7. **Unsaved Changes Prompt**:
   - Prompts you to Save, Discard, or Go Back if you attempt to record a new macro, open a file, or close the window while having unsaved keystrokes.
8. **Speed Multiplier**:
   - Configurable speed (e.g. `1.0` for 1x, `2.0` for 2x, `0.5` for half speed).
9. **Global System-Wide Hotkeys**:
   - **`F8`**: Start / Stop Recording
   - **`F9`**: Play Recorded Macro
   - **`Escape`**: Emergency Stop / Abort Playback
10. **File Persistence (`.krec`)**:
    - Save and load your keystroke sequences anytime.

## Compilation

```bat
build.bat
```
