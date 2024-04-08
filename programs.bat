@if not exist programs.txt call :generate programs.txt
@for /F "tokens=1-2 delims=|" %%i in ('Bin\x64Release\RadMenu.exe /il /delim ^^^| /f programs.txt') do @call %%j
@exit /b

:generate
@echo off
setlocal
set FILE=%1
set DELIM=^^^|
rem set DELIM=,

echo Generating %FILE%
for %%d in (
  "%APPDATA%\Microsoft\Windows\Start Menu"
  "%ProgramData%\Microsoft\Windows\Start Menu"
) do @forfiles /P %%d /S /M *.lnk /C "cmd /c echo @fname%DELIM%@path&& (echo @fname 1>&2)" >> %FILE%

echo on
