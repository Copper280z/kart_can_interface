#include "usb_task.h"
#include "cdc_device.h"
#include "dbc_assert.h"
#include "fifo.h"
#include "sst.h"
#include "usbd.h"

DBC_MODULE_NAME("usb handlers")
fifo_t usb_fifo;
static uint8_t usb_fifo_buffer[4096];

enum USB_Signals {
  USB_HANDLE_SIG,
  /* ... */
  USB_MAX_SIG /* the last signal */
};

typedef struct {
  SST_Task super; /* inherit SST_Task  */
  SST_TimeEvt
      handler_evt; // tasks can have multiple time events, for different actions
} USB_Task;

static USB_Task usb_inst;
SST_Task *const AO_usb = &usb_inst.super; // task might be defined in

static void USB_ctor(USB_Task *const me);
static void USB_init(USB_Task *const me, SST_Evt const *const ie);
static void USB_dispatch(USB_Task *const me, SST_Evt const *const e);
void usb_transmit();
void CEC_IRQHandler();

void usb_task_instantiate(void (*error_callback)()) {
  USB_ctor(&usb_inst);
  fifo_init(&usb_fifo, usb_fifo_buffer, 4096, error_callback);
  SST_Task_setIRQ(AO_usb, CEC_IRQn);
}
void USB_ctor(USB_Task *const me) {
  SST_Task_ctor(&me->super, (SST_Handler)&USB_init, (SST_Handler)&USB_dispatch);
  SST_TimeEvt_ctor(&me->handler_evt, USB_HANDLE_SIG, &me->super);
}
void USB_init(USB_Task *const me, SST_Evt const *const ie) {
  (void)ie;
  SST_TimeEvt_arm(&me->handler_evt, 1U, 1U);
}
void USB_dispatch(USB_Task *const me, SST_Evt const *const e) {
  switch (e->sig) {
  case USB_HANDLE_SIG: {
    usb_transmit();
    tud_task();
    break;
  }
  default: {
    DBC_ERROR(200);
  }
  }
}

void usb_transmit() {
  const uint8_t itf = 0;

  while (fifo_size(&usb_fifo)) {
    uint8_t ch;
    if (!tud_cdc_n_write_available(itf))
      return;

    // we pop a byte and send it if we're connected
    // otherwise discard it
    bool success = fifo_pop(&usb_fifo, &ch);
    if (tud_cdc_connected() && success) {
      tud_cdc_n_write_char(itf, ch);
    }
  }
  tud_cdc_n_write_flush(itf);
}
void CEC_IRQHandler() { SST_Task_activate(AO_usb); }
