@echo off
REM File: run_openocd.bat
REM Created on: 14/12/12
REM Author: Adam Goodwin
REM
REM Description: This batch file runs OpenOCD if it isn't already running.
REM
REM You must specify the OpenOCD executable, the path to the executable, and the
REM configuration file to be used.

REM Set the OpenOCD executable name here:
set OCD=openocd-x64-0.6.1.exe

REM Set the absolute path to the OpenOCD executable here:
set OCDP=C:\Program Files\openocd-0.6.1\bin-x64\

REM Set the configuration file here:
set OCD_CFG=uc-sam7-usbjtag.cfg
REM set OCD_CFG=openocd.cfg 



REM Check if OpenOCD is running, storing the count of how many open instances of
REM OpenOCD there are.
FOR /F %%A IN ('tasklist /FI "IMAGENAME eq %OCD%" /NH ^| find /C "%OCD%"') DO (
    set COUNT=%%A
)

REM Run OpenOCD if necessary:
if %COUNT% EQU 0 (
    echo OpenOCD is not running. Starting OpenOCD...
    start "OpenOCD" /D . /MIN "%OCDP%%OCD%" "-f" "%OCD_CFG%" 
) ELSE (
    echo There is already %COUNT% instance^(s^) of OpenOCD running.
)

