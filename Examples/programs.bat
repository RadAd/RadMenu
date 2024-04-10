@if not exist programs.txt call :generate programs.txt
@for /F %%i in ('..\Bin\x64Release\RadMenu.exe /il /dm fname /f programs.txt') do @call %%i
@rem errorlevel 1 pause
@exit /b

:generate
@echo off
setlocal
set FILE=%1

echo Generating %FILE%
for %%d in (
  "%APPDATA%\Microsoft\Windows\Start Menu\"
  "%ProgramData%\Microsoft\Windows\Start Menu\"
) do @call :doall %%d >> %FILE%

sort %1 /O %1

echo on
@goto :eof

:doall
for /R %1 %%i in (*.lnk) do @call :do "%%i"
goto :eof

:do
echo.%~1
echo.%~n1 1>&2
goto :eof
