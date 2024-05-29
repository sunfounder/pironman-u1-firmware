set baud=921600
set chip=esp32s2

esptool.exe --chip %chip% --baud %baud% --before default_reset erase_flash

pause
