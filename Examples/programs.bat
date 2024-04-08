@if not exist programs.txt call :generate programs.txt
@for /F "tokens=1-2 delims=|" %%i in ('..\Bin\x64Release\RadMenu.exe /il /delim ^^^| /f programs.txt') do @call %%j
@rem errorlevel 1 pause
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
) do @call :doall %%d >> %FILE%

sort %1 /O %1

echo on
@goto :eof

:doall
for /R %1 %%i in (*.lnk) do @call :do "%%i"
rem forfiles /P %1 /S /M *.lnk /C "cmd /c echo @fname%DELIM%@path&& (echo @fname 1>&2)"


:do
echo "%~n1"%DELIM%%1
echo "%~n1" 1>&2
goto :eof
