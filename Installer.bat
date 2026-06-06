@echo off
rem ====================================================
rem	TimeDefuser Service Installer Script
rem
rem	https://github.com/NevermindExpress/TimeDefuser
rem ====================================================

net session >nul 2>&1
if %errorlevel% equ 0 (
	copy TimeDefuser-%processor_architecture%.sys %windir%\System32\Drivers\TimeDefuser.sys
	sc create TimeDefuser type= kernel start= auto binPath= %windir%\System32\Drivers\TimeDefuser.sys
	sc start TimeDefuser
) else (
	echo Administrator rights are required.
)
pause