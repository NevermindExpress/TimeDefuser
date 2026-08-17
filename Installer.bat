@echo off
setlocal

rem ====================================================
rem  TimeDefuser Service Installer
rem
rem  https://github.com/NevermindExpress/TimeDefuser
rem ====================================================

net session >nul 2>&1
if errorlevel 1 (
    echo [X] Administrator rights are required.
    goto :end
)

set "DRIVER_SRC=TimeDefuser-%PROCESSOR_ARCHITECTURE%.sys"
set "DRIVER_DST=%SystemRoot%\System32\Drivers\TimeDefuser.sys"

if not exist "%DRIVER_SRC%" (
    echo [X] Driver not found: %DRIVER_SRC%
    goto :end
)

echo [+] Installing %DRIVER_SRC%...

copy /Y "%DRIVER_SRC%" "%DRIVER_DST%" >nul
if errorlevel 1 (
    echo [X] Failed to copy driver.
    goto :end
)

rem Delete an existing service first, if present.
sc query TimeDefuser >nul 2>&1
if not errorlevel 1 (
    echo [*] Existing TimeDefuser service found.
    sc stop TimeDefuser >nul 2>&1
    sc delete TimeDefuser >nul 2>&1
)

sc create TimeDefuser ^
    type= kernel ^
    start= auto ^
    binPath= "%DRIVER_DST%" >nul

if errorlevel 1 (
    echo [X] Failed to create TimeDefuser service.
    goto :end
)

sc start TimeDefuser
if errorlevel 1 (
    echo [X] Failed to start TimeDefuser.
    goto :end
)

echo [+] TimeDefuser installed successfully.

:end
pause
endlocal