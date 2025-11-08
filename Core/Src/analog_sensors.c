#include "analog_sensors.h"
#include "can.h"
#include "can_defs.h"
#include "dbc_assert.h"
#include "main.h"
#include "sst.h"
#include "stm32h743xx.h"
#include <inttypes.h>
#include <stdio.h>

DBC_MODULE_NAME("Analog")

static INIT_AnalogMsg(msg);

enum Analog_Signals {
  ANALOG_MESSAGE_SIG,
  ANALOG_FILTER_SIG,
  /* ... */
  ANALOG_MAX_SIG /* the last signal */
};
typedef struct {
  SST_Task super; /* inherit SST_Task  */
  SST_TimeEvt
      handler_evt; // tasks can have multiple time events, for different actions
} Analog_Task;

static Analog_Task analog_inst;
SST_Task *const AO_analog = &analog_inst.super; // task might be defined in

static void Analog_ctor(Analog_Task *const me);
static void Analog_init(Analog_Task *const me, SST_Evt const *const ie);
static void Analog_dispatch(Analog_Task *const me, SST_Evt const *const e);
void SPDIF_RX_IRQHandler();

void analog_task_instantiate(void (*error_callback)()) {
  Analog_ctor(&analog_inst);
  SST_Task_setIRQ(AO_analog, SPDIF_RX_IRQn);
}

static void Analog_ctor(Analog_Task *const me) {
  SST_Task_ctor(&me->super, (SST_Handler)&Analog_init,
                (SST_Handler)&Analog_dispatch);
  SST_TimeEvt_ctor(&me->handler_evt, ANALOG_MESSAGE_SIG, &me->super);
}
static void Analog_init(Analog_Task *const me, SST_Evt const *const ie) {
  (void)ie;
  msg.data.analog.rear_brake_pressure = 1;
  msg.data.analog.throttle_position = 2;
  SST_TimeEvt_arm(&me->handler_evt, 10U, 10U);
}
static void Analog_dispatch(Analog_Task *const me, SST_Evt const *const e) {
  switch (e->sig) {
  case ANALOG_MESSAGE_SIG: {
    msg.data.analog.rear_brake_pressure += 1;
    msg.data.analog.throttle_position += 1;
    can_tx_msg(&msg);
    printf("Analog Msg!, TIM8->CNT: %d\r\n", TIM8->CNT);
    break;
  }
  case ANALOG_FILTER_SIG: {
    // unused for now
    break;
  }
  default:
    DBC_ERROR(300);
  }
  //
}
void SPDIF_RX_IRQHandler() {
  SST_Task_activate(AO_analog); //
}
