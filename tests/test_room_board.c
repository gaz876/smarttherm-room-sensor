#include "room_board.h"
#include <assert.h>
#include <stdio.h>
_Static_assert(ROOM_I2C_PINS_VALID(4, 5), "default wiring");
_Static_assert(!ROOM_I2C_PINS_VALID(5, 4), "swapped wiring rejected");
int main(void) {
    unsigned accepted = 0;
    for (int sda = -1; sda <= 22; ++sda) {
        for (int scl = -1; scl <= 22; ++scl) {
            const int valid = ROOM_I2C_PINS_VALID(sda, scl);
            assert(valid == (sda == 4 && scl == 5));
            accepted += (unsigned)valid;
        }
    }
    assert(accepted == 1);
    puts("PASS: 576 GPIO pairs; only SDA4/SCL5 accepted");
    return 0;
}
