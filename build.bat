@echo off
REM LoRaScope Build Script for Windows

echo ========================================
echo LoRaScope Build Script
echo ========================================
echo.

REM Check if PlatformIO is installed
where pio >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: PlatformIO is not installed or not in PATH
    echo Please install PlatformIO: https://platformio.org/install
    exit /b 1
)

REM Set default environment
set ENV=m5cardputer

REM Parse command line arguments
if "%1"=="" (
    echo Usage: build.bat [environment] [command]
    echo.
    echo Environments:
    echo   m5cardputer        - M5Cardputer standard (default)
    echo   m5cardputer_adv    - M5Cardputer ADV
    echo   m5cardputer_sx1262 - M5Cardputer with SX1262 module
    echo   m5cardputer_rf95   - M5Cardputer with RF95 module
    echo.
    echo Commands:
    echo   clean              - Clean build artifacts
    echo   build              - Build firmware (default)
    echo   upload             - Upload to device
    echo   monitor            - Open serial monitor
    echo   all                - Clean, build and upload
    echo.
    echo Examples:
    echo   build.bat                      - Build for m5cardputer
    echo   build.bat m5cardputer_adv      - Build for m5cardputer_adv
    echo   build.bat m5cardputer build    - Build for m5cardputer
    echo   build.bat m5cardputer upload   - Upload to device
    echo.
    set /p ENV="Enter environment (default: m5cardputer): "
    if "%ENV%"=="" set ENV=m5cardputer
    set CMD=build
) else (
    set ENV=%1
    if "%2"=="" (
        set CMD=build
    ) else (
        set CMD=%2
    )
)

echo.
echo Environment: %ENV%
echo Command: %CMD%
echo.

REM Execute command
if "%CMD%"=="clean" (
    echo Cleaning build artifacts...
    pio run -e %ENV% -t clean
) else if "%CMD%"=="build" (
    echo Building firmware...
    pio run -e %ENV%
) else if "%CMD%"=="upload" (
    echo Uploading to device...
    pio run -e %ENV% -t upload
) else if "%CMD%"=="monitor" (
    echo Opening serial monitor...
    pio device monitor -p COM3
) else if "%CMD%"=="all" (
    echo Cleaning, building and uploading...
    pio run -e %ENV% -t clean
    pio run -e %ENV% -t upload
) else (
    echo ERROR: Unknown command '%CMD%'
    exit /b 1
)

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo SUCCESS: Command completed
    echo ========================================
) else (
    echo.
    echo ========================================
    echo ERROR: Command failed
    echo ========================================
    exit /b 1
)

pause
