# SHT40 codec

Reference: Sensirion SHT4x datasheet, https://sensirion.com/media/documents/33FD6951/662A593A/HT_DS_Datasheet_SHT4x.pdf and manufacturer driver https://github.com/Sensirion/embedded-sht/blob/master/sht4x/sht4x.c .

CRC-8 on each two-byte word; temperature and humidity conversion; humidity clamping. Calibration limits +/-10 C and +/-30 percentage points are project policy, not sensor specifications. Invalid frames clear the output. Use monotonic milliseconds. I2C transport must invalidate readings on error. Host build: `cc -std=c11 -Wall -Wextra -Werror -pedantic -Isrc src/sht40.c tests/test_sht40.c -lm -o /tmp/test_sht40 && /tmp/test_sht40`. No hardware or target build claim.
