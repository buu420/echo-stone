@echo off
setlocal DisableDelayedExpansion
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\Install.ps1"
set "MvcIAccessResult=%ERRORLEVEL%"
echo(
if "%MvcIAccessResult%"=="0" (
    echo Echo Stone installed. Start MVC Infinite from Steam.
) else (
    echo Installation stopped. See the message above.
    pause
)
exit /b %MvcIAccessResult%
