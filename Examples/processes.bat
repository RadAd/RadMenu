@echo off
tasklist /fo csv | call ..\Bin\x64Release\RadMenu.exe /header 1 /cols "PID,Image Name" | more
pause
