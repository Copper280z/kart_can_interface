#ifndef _TIRE_TEMPS_H
#define _TIRE_TEMPS_H

#include "main.h"
#include "sst.h"

extern SST_Task *const AO_tires;
void tire_temps_task_instantiate(void (*error_callback)(),
                                 I2C_HandleTypeDef *hi2c, uint8_t num_sensors,
                                 uint8_t sensor_addr[4],
                                 uint8_t sensor_type[4]);
#endif
