@echo off
echo Building KeyTask (Keyboard Macro Tool)...
g++ -O2 -std=c++17 src/main.cpp -o KeyTask.exe -mwindows -municode -lcomctl32 -lcomdlg32
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    exit /b %errorlevel%
)
echo [SUCCESS] KeyTask.exe built successfully!
