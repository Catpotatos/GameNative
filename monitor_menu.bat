@echo off
REM Quick monitoring utilities for ANGLE Bionic testing
REM Usage: Place in project root and run from PowerShell

:menu
cls
echo ====================================
echo   ANGLE BIONIC MONITORING MENU
echo ====================================
echo.
echo 1. Watch Extraction Speed (live)
echo 2. Watch ANGLE Initialization (live)
echo 3. Watch All Filtered Events (live)
echo 4. Check Device Metrics
echo 5. Check Full System Logs (last 50 lines)
echo 6. View Real-time Monitor Log (last 20 lines)
echo 7. Show Device CPU Usage (live, 10 iterations)
echo 8. Show Device Memory Usage
echo 9. Get Device Info
echo 0. Exit
echo.
set /p choice="Choose option: "

if "%choice%"=="1" (
    cls
    echo [EXTRACTION SPEED MONITORING]
    echo Watching TarCompressorUtils logs...
    echo Press Ctrl+C to stop.
    echo.
    adb logcat TarCompressorUtils -v short
    goto menu
)

if "%choice%"=="2" (
    cls
    echo [ANGLE INITIALIZATION MONITORING]
    echo Watching ANGLE env var setup...
    echo Press Ctrl+C to stop.
    echo.
    adb logcat XServerScreen BionicProgramLauncherComponent -v short
    goto menu
)

if "%choice%"=="3" (
    cls
    echo [ALL FILTERED EVENTS]
    echo Watching: TarCompressor + ANGLE + Bionic messages
    echo Press Ctrl+C to stop.
    echo.
    adb logcat -v short | find /i "TarCompressor" & find /i "ANGLE" & find /i "Bionic"
    goto menu
)

if "%choice%"=="4" (
    cls
    echo [DEVICE METRICS - Last 10 snapshots]
    echo.
    PowerShell -Command "Get-Content device_metrics.log -Tail 20"
    pause
    goto menu
)

if "%choice%"=="5" (
    cls
    echo [FULL SYSTEM LOGS - Last 50 lines]
    echo.
    PowerShell -Command "Get-Content device_logs_full.txt -Tail 50"
    pause
    goto menu
)

if "%choice%"=="6" (
    cls
    echo [REAL-TIME MONITOR LOG - Last 20 events]
    echo.
    PowerShell -Command "Get-Content real_time_monitor.log -Tail 20"
    pause
    goto menu
)

if "%choice%"=="7" (
    cls
    echo [CPU USAGE - 10 iterations, 2 second intervals]
    echo.
    for /l %%i in (1,1,10) do (
        echo === Iteration %%i ===
        adb shell top -n 1 -b -H | find "app.gamenative"
        timeout /t 2 /nobreak
    )
    pause
    goto menu
)

if "%choice%"=="8" (
    cls
    echo [DEVICE MEMORY USAGE]
    echo.
    adb shell dumpsys meminfo app.gamenative
    pause
    goto menu
)

if "%choice%"=="9" (
    cls
    echo [DEVICE INFORMATION]
    echo.
    echo Device Model:
    adb shell getprop ro.product.model
    echo.
    echo Android Version:
    adb shell getprop ro.build.version.release
    echo.
    echo Serial:
    adb shell getprop ro.boot.serialno
    echo.
    echo Available Storage:
    adb shell df /data
    echo.
    pause
    goto menu
)

if "%choice%"=="0" (
    exit /b
)

echo Invalid choice!
timeout /t 2
goto menu

