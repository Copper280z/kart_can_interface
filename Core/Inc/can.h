#pragma once
#include "can_defs.h"
#include <inttypes.h>

void config_can();
void can_tx_msg(CanMsg *msg);
