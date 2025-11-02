#include "amg8833.h"
#include <stdio.h>

// blocking reads/writes
int amg8833_init(I2C_HandleTypeDef *hi2c, uint8_t address) { return 0; }

// interrupt reads
int amg8833_read_thermistor(I2C_HandleTypeDef *hi2c, uint8_t address,
                            uint8_t buf[2]) {
  uint8_t buffer[1] = {0x0E};
  HAL_StatusTypeDef status =
      HAL_I2C_Master_Transmit(hi2c, address << 1, buffer, 1, 1000);
  if (status != HAL_OK) {
    printf("Failed to start amg8833 thermistor read: %d\r\n", status);
    return status;
  }
  status = HAL_I2C_Master_Receive_IT(hi2c, address << 1, buf, 2);
  if (status != HAL_OK) {
    printf("Failed to recv amg8833 thermistor: %d\r\n", status);
    return status;
  }
  return status;
}

// interrupt reads
int amg8833_read_pixels(I2C_HandleTypeDef *hi2c, uint8_t address,
                        uint8_t pixel_offset, uint8_t num_pixels,
                        uint8_t *buf) {

  uint8_t pix_reg[1] = {0x80 + pixel_offset * 2};
  HAL_StatusTypeDef status =
      HAL_I2C_Master_Transmit(hi2c, address << 1, pix_reg, 1, 10);
  if (status != HAL_OK) {
    printf("Failed to start measure tires with error: %d\r\n", status);
    return status;
  }
  status = HAL_I2C_Master_Receive_IT(hi2c, address << 1, buf, num_pixels * 2);
  if (status != HAL_OK) {
    printf("Failed to recv measure tires with error: %d\r\n", status);
    return status;
  }
  return status;
}
