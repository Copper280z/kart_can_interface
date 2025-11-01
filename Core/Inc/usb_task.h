#pragma once
#include "fifo.h"
#include "sst.h"

extern SST_Task *const AO_usb;
extern fifo_t usb_fifo;
void usb_task_instantiate(void (*error_callback)());

// void usb_transmit();
