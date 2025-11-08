#pragma once
#include "can_defs.h"
#include <inttypes.h>

// blue - CAN_H
// green - CAN_L
// red - +Vb
// black - GND
// yellow - digital_out

void config_can();
void can_tx_msg(CanMsg *msg);
