/* Copyright 2022 @ Keychron (https://www.keychron.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "quantum.h"
#include "indicator.h"
#include "transport.h"

#define HC595_STCP B0
#define HC595_SHCP A1
#define HC595_DS A7

pin_t row_pins[MATRIX_ROWS] = MATRIX_ROW_PINS;
pin_t col_pins[MATRIX_COLS] = MATRIX_COL_PINS;

extern indicator_config_t indicator_config;
static uint32_t           power_on_indicator_timer_buffer;
extern bool bat_low_led_pin_state;

#ifndef POWER_ON_LED_DURATION
#    define POWER_ON_LED_DURATION 3000
#endif

static inline void HC595_delay(uint16_t n) {
    while (n-- > 0) {
        asm volatile("nop" ::: "memory");
    };
}

static void HC595_output(uint32_t data) {
    uint8_t i;
    uint8_t n = 1;

    for (i = 0; i < (MATRIX_COLS + 3); i++) {
        writePinLow(HC595_SHCP);

        if (data & 0x1)
            writePinHigh(HC595_DS);
        else
            writePinLow(HC595_DS);

        data >>= 1;

        HC595_delay(n);

        writePinHigh(HC595_SHCP);
        HC595_delay(n);
    }

    HC595_delay(n);
    writePinLow(HC595_STCP);
    HC595_delay(n);
    writePinHigh(HC595_STCP);
}

static inline void setPinOutput_writeLow(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        setPinOutput(pin);
        writePinLow(pin);
    }
}

static inline void setPinInputHigh_atomic(pin_t pin) {
    ATOMIC_BLOCK_FORCEON {
        setPinInputHigh(pin);
    }
}

static inline uint8_t readMatrixPin(pin_t pin) {
    if (pin != NO_PIN) {
        return readPin(pin);
    } else {
        return 1;
    }
}

static bool select_col(uint8_t col) {
    pin_t    pin   = col_pins[col];
    uint32_t value = 0;

    if (pin != NO_PIN) {
        setPinOutput_writeLow(pin);
        return true;
    } else {
        if (power_on_indicator_timer_buffer) {
            if (sync_timer_elapsed32(power_on_indicator_timer_buffer) > POWER_ON_LED_DURATION) {
                power_on_indicator_timer_buffer = 0;
                HC595_output((~(0x1 << (21 - col - 1))) & (7 << 0));
            } else {
                HC595_output((~(0x1 << (21 - col - 1))));
            }
        } else {
            if (get_transport() == TRANSPORT_BLUETOOTH) {
                if (indicator_config.value) {
                    if (indicator_config.value & 0x80) {
                        value = ~(0x1 << (21 - col - 1)) & ~(3 << 0);
                        HC595_output(value);
                    } else {
                        value = (~(0x1 << (21 - col - 1))) & ~(7 << 0);
                        HC595_output(value);
                    }
                } else {
                    if (host_keyboard_led_state().caps_lock) {
                        value = (~(0x1 << (21 - col - 1))) & ~(5 << 0);
                        HC595_output(value);
                    } else {
                        value = (~(0x1 << (21 - col - 1))) & ~(7 << 0);
                        HC595_output(value);
                    }
                }
                if (bat_low_led_pin_state) {
                    HC595_output(value | (1 << 0));
                }
            } else {
                if (host_keyboard_led_state().caps_lock) {
                    HC595_output((~(0x1 << (21 - col - 1))) & ~(5 << 0));
                } else {
                    HC595_output((~(0x1 << (21 - col - 1))) & ~(7 << 0));
                }
            }
        }
        return true;
    }
    return false;
}

static void unselect_col(uint8_t col) {
    pin_t    pin   = col_pins[col];
    uint32_t value = 0;

    if (pin != NO_PIN) {
        setPinInputHigh_atomic(pin);
    } else {
        if (power_on_indicator_timer_buffer) {
            if (sync_timer_elapsed32(power_on_indicator_timer_buffer) > POWER_ON_LED_DURATION) {
                power_on_indicator_timer_buffer = 0;
                HC595_output(0x1FFFFF & ~(7 << 0));
            } else {
                HC595_output(0x1FFFFF);
            }
        } else {
            if (get_transport() == TRANSPORT_BLUETOOTH) {
                if (indicator_config.value) {
                    if (indicator_config.value & 0x80) {
                        if (col == (MATRIX_COLS - 1)) {
                            value = 0x1FFFFF & ~(3 << 0);
                            HC595_output(value);
                        }
                    } else {
                        if (col == (MATRIX_COLS - 1)) {
                            if (col == (MATRIX_COLS - 1)) {
                                value = 0x1FFFFF & ~(7 << 0);
                                HC595_output(value);
                            }
                        }
                    }
                } else {
                    if (host_keyboard_led_state().caps_lock) {
                        if (col == (MATRIX_COLS - 1)) {
                            if (col == (MATRIX_COLS - 1)) {
                                value = 0x1FFFFF & ~(5 << 0);
                                HC595_output(value);
                            }
                        }
                    } else {
                        if (col == (MATRIX_COLS - 1)) {
                            if (col == (MATRIX_COLS - 1)) {
                                value = 0x1FFFFF & ~(7 << 0);
                                HC595_output(value);
                            }
                        }
                    }
                }
                if (bat_low_led_pin_state) {
                    HC595_output(value | (1 << 0));
                }
            } else {
                if (host_keyboard_led_state().caps_lock) {
                    if (col == (MATRIX_COLS - 1)) {
                        HC595_output(0x1FFFFF & ~(5 << 0));
                    }
                } else {
                    if (col == (MATRIX_COLS - 1)) {
                        HC595_output(0x1FFFFF & ~(7 << 0));
                    }
                }
            }
        }
    }
}

static void unselect_cols(void) {
    if (col_pins[0] != NO_PIN) setPinInputHigh_atomic(col_pins[0]);
    HC595_output(0xFFFFFFFF);
    power_on_indicator_timer_buffer = sync_timer_read32() | 1;
}

void select_all_cols(void) {
    if (col_pins[0] != NO_PIN) setPinOutput_writeLow(col_pins[0]);
    if (host_keyboard_led_state().caps_lock) {
        HC595_output(0x00000000 | (2 << 0));
    } else {
        HC595_output(0x00000000);
    }
}

void matrix_read_rows_on_col(matrix_row_t current_matrix[], uint8_t current_col) {
    // Select col
    if (!select_col(current_col)) { // select col
        return;                     // skip NO_PIN col
    }

    HC595_delay(100);

    // For each row...
    for (uint8_t row_index = 0; row_index < MATRIX_ROWS; row_index++) {
        // Check row pin state
        if (readMatrixPin(row_pins[row_index]) == 0) {
            // Pin LO, set col bit
            current_matrix[row_index] |= (MATRIX_ROW_SHIFTER << current_col);
            // key_pressed = true;
        } else {
            // Pin HI, clear col bit
            current_matrix[row_index] &= ~(MATRIX_ROW_SHIFTER << current_col);
        }
    }

    unselect_col(current_col);
    HC595_delay(100);
}

void matrix_init_custom(void) {
    setPinOutput(HC595_DS);
    setPinOutput(HC595_STCP);
    setPinOutput(HC595_SHCP);
    for (uint8_t x = 0; x < MATRIX_ROWS; x++) {
        if (row_pins[x] != NO_PIN) {
            setPinInputHigh_atomic(row_pins[x]);
        }
    }
    unselect_cols();
}

bool matrix_scan_custom(matrix_row_t current_matrix[]) {
    bool matrix_has_changed = false;

    matrix_row_t curr_matrix[MATRIX_ROWS] = {0};

    // Set col, read rows
    for (uint8_t current_col = 0; current_col < MATRIX_COLS; current_col++) {
        matrix_read_rows_on_col(curr_matrix, current_col);
    }

    matrix_has_changed = memcmp(current_matrix, curr_matrix, sizeof(curr_matrix)) != 0;
    if (matrix_has_changed) memcpy(current_matrix, curr_matrix, sizeof(curr_matrix));

    return matrix_has_changed;
}
