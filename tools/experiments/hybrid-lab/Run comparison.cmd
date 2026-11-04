@echo off
setlocal
set "LAB_RUN=%~dp0results\run-%RANDOM%-%RANDOM%"
"%~dp0horde_hybrid_lab.exe" "%LAB_RUN%" 640 360 24
if errorlevel 1 (pause & exit /b 1)
start "" "%LAB_RUN%\index.html"
