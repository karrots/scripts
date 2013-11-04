@echo off
setlocal enabledelayedexpansion

set IP=192.168.53.18
set RSYNCPATH=C:\SCRIPTS\rsync\

cd %RSYNCPATH%

echo "STOP RSYNC SERVICE" %date% %time% >rsyncd-start.log & timeout 1 > nul

C:\Windows\System32\tasklist.exe /V /FI "IMAGENAME eq rsync.exe" /FO LIST  >>rsyncd-start.log 2>>&1  & timeout 1 > nul

FOR /L %%I IN (1,1,4) DO echo|set /p=. >>rsyncd-start.log & timeout 1 > nul

echo . >>rsyncd-start.log

C:\Windows\System32\taskkill.exe /IM rsync.exe /F >>rsyncd-start.log 2>>&1  & echo . >>rsyncd-start.log  & timeout 1 > nul

del /F /Q rsyncd.pid >>rsyncd-start.log 2>>&1 & timeout 1 > nul

echo "START RSYNC SERVICE" %date% %time% >>rsyncd-start.log & timeout 1 > nul

FOR /L %%I IN (1,1,4) DO echo|set /p=. >>rsyncd-start.log & timeout 1 > nul

rsync.exe --daemon -4 --address=%IP% --config=rsyncd.conf >>rsyncd-start.log 2>>&1  & timeout 1 > nul

C:\Windows\System32\tasklist.exe /V /FI "IMAGENAME eq rsync.exe" /FO LIST  >>rsyncd-start.log 2>>&1  & timeout 1 > nul

exit
