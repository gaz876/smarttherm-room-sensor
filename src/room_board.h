#ifndef ROOM_BOARD_H
#define ROOM_BOARD_H
/* Provisional SuperMini profile: preserve BOOT, USB, UART and flash pins. */
#define ROOM_I2C_SDA_GPIO 4
#define ROOM_I2C_SCL_GPIO 5
#define ROOM_I2C_PINS_VALID(sda, scl) \
    ((sda) == ROOM_I2C_SDA_GPIO && (scl) == ROOM_I2C_SCL_GPIO)
#endif
