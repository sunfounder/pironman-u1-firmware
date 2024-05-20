# SPC Data Buffer Sheet

## dataBuffer

|name|addr|len|type|unit|description|
|:--|:--:|:--:|:--:|:--:|:--|
|external_input_voltage|0|2|u16|mV|-|
|external_input_current|2|2|u16|mA|-|
|output_voltage|4|2|u16|mV|-|
|output_current|6|2|u16|mA|-|
|battery_voltage|8|2|u16|mV|-|
|battery_current|10|2|int16|mA|-|
|battery_percentage|12|1|u8|%|-|
|battery_capacity|13|2|u16|mAh|-|
|power_source|15|1|u8|-| 0,  battery isn't supplying power; <br> 1,  battery is supplying power;|
|is_usb_plugged|16|1|u8|-| 0, usb is unplugged; <br> 1, usb is plugged in;|
|is_battery_plugged|17|1|u8|-| 0, battery is unplugged; <br> 1, battery is plugged in;|
|is_charging|18|1|u8|-| 0, not charging; <br> 1, charging;|
|fan_power|19|1|u8|-| 0 ~ 100|
|shutdown_request|20|1|u8|-| 1, Key shutdown request; <br> 1, Low battery shutdown request;|
|-|-|-|-|-|-|
|-|-|-|-|-|-|
|firmware_version_major|128|1|u8|-|-|
|firmware_version_minor|129|1|u8|-|-|
|firmware_version_micro|130|1|u8|-|-|
|rst_code|131|1|u8|-|mcu reset reason Code|
|rtc_year|132|1|u8|-|-|
|rtc_month|133|1|u8|-|-|
|rtc_day|134|1|u8|-|-|
|rtc_hour|135|1|u8|-|-|
|rtc_minute|136|1|u8|-|-|
|rtc_second|137|1|u8|-|-|
|rtc_ssec|138|1|u8|-|1/128 second|
|alwaysOn|139|1|u8|-|0, Enable alwaysOn <br> 1, Disable alwaysOn|
|board_id|140|1|u8|-|0, Pironman U1 <br> 1, Pironman 4 <br> 2, Pipower 3|
|powerSourceVoltage|141|2|u16|-|-|
|shutdown_pct|142|1|u8|-| Current low battery shutdown percentage threshold|
|poweroff_pct|143|1|u8|-| Current low battery poweroff percentage threshold|
|-|-|-|-|-|-|
|-|-|-|-|-|-|

## settingBuffer

|name|addr|len|type|unit|description|
|:--|:--:|:--:|:--:|:--:|:--|
|fan_power|0|1|u8|-|setting fan power (0~100)|
|rtc_year|1|1|u8|-|setting rtc_year|
|rtc_month|2|1|u8|-|setting rtc_month|
|rtc_day|3|1|u8|-|setting rtc_day|
|rtc_hour|4|1|u8|-|setting rtc_hour|
|rtc_minute|5|1|u8|-|setting rtc_minute|
|rtc_second|6|1|u8|-|setting rtc_second|
|rtc_ssec|7|1|u8|-|setting rtc 1/128 second|
|rtc_setting|8|1|u8|-|1, enable rtc setting|
|shutdown_pct|9|1|u8|-|setting low battery shutdown percentage threshold (0~100)|
|poweroff_pct|10|1|u8|-|setting low battery poweroff percentage threshold (0~100)|
|-|-|-|-|-|-|
|-|-|-|-|-|-|
