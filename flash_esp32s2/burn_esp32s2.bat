@echo off
chcp 65001

set baud=921600
set chip=esp32s2
set boot_app0="bin/boot_app0.bin"
set bootloader="bin/bootloader.bin"
set partitions="bin/partitions.bin"
set app="bin/pironman-u1-firmware-0.0.9.bin"
set run_count=0

:erase_and_burn
set /a "run_count+=1"
echo.
echo # %run_count% #############################################

@REM erase
echo.
echo ## erase ##
esptool.exe --chip %chip% --baud %baud% --before default_reset erase_flash

@REM burn
echo.
echo ## burn ##
esptool.exe --chip %chip% --baud %baud% --before default_reset ^
write_flash -e -z --flash_mode dio --flash_freq 80m --flash_size 4MB ^
0x1000 %bootloader% ^
0x8000 %partitions% ^
0xe000 %boot_app0% ^
0x10000 %app%

echo.
echo Press the follow to continue:
echo [1]Repeat Operation        [2] Exit
choice /C 12 /N /M ":"
if "%errorlevel%"=="1" (
    goto erase_and_burn
) else if "%errorlevel%"=="2" (
    echo Exit
    exit
)



