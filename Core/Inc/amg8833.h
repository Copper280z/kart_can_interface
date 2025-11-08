#ifndef _AMG8833_H
#define _AMG8833_H

#include "main.h"
#include "stm32h7xx_hal_i2c.h"
#include <inttypes.h>

// Only 2 possible i2c addresses, so either need additional hardware to separate
// devices or can use max of 2 per bus.
// 0x68 or 0x69
// may want to use i2c line buffer hardware
// https://www.digikey.com/en/products/detail/texas-instruments/TCA9617BDGKR/5056065
// may also want common mode choke on power rails.
// https://www.digikey.com/en/products/detail/w%C3%BCrth-elektronik/744227S/1638886

// blocking reads/writes
int amg8833_init(I2C_HandleTypeDef *hi2c, uint8_t address);

// interrupt reads
int amg8833_read_thermistor(I2C_HandleTypeDef *hi2c, uint8_t address,
                            uint8_t buf[2]);

// interrupt reads
// pixel_offset: actual pixels
// num_pixels: actual number of pixels, buffer must be 2x this value because
// pixels are 12bit
int amg8833_read_pixels(I2C_HandleTypeDef *hi2c, uint8_t address,
                        uint8_t pixel_offset, uint8_t num_pixels, uint8_t *buf);

#endif
