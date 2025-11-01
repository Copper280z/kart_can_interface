#ifndef _ANALOG_SENSORS_H
#define _ANALOG_SENSORS_H

#include "sst.h"

extern SST_Task *const AO_analog;
void analog_task_instantiate(void (*error_callback)());

#endif
