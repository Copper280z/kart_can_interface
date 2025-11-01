#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "main.h"
#include "sst.h"
#include "sst_port.h"

/*
##############################################################################
SST Boiler plate
##############################################################################
*/

void DBC_fault_handler(char const *const module, int const label) {
  /*
   * NOTE: add here your application-specific error handling
   */
  (void)module;
  (void)label;

  HAL_GPIO_WritePin(blue_led_GPIO_Port, blue_led_Pin, false);
  /* set PRIMASK to disable interrupts and stop SST right here */
  __asm volatile("cpsid i");

  NVIC_SystemReset();
}

void SST_onStart(void) {
  SystemCoreClockUpdate();
  /* set up the SysTick timer to fire at kHz rate */
  SysTick_Config((SystemCoreClock / 1000U) + 1U);
  /* set priorities of ISRs used in the system */
  NVIC_SetPriority(SysTick_IRQn, 0U);
}
void SST_onIdle(void) {}

/*
##############################################################################
SST Boiler plate
##############################################################################
*/
// enum Signals {
//   TIMEOUT1_SIG,
//   TIMEOUT2_SIG,
//   /* ... */
//   MAX_SIG /* the last signal */
// };
// typedef struct {
//   SST_Task super;  /* inherit SST_Task  */
//   SST_TimeEvt te1; // tasks can have multiple time events, for different
//   actions
//
// } Task;
//
// /*Task init
//  * SST calls this function in SST_Task_start
//  */
// void task_init(Task *const me, SST_Evt const *const ie) {
//   (void)ie; /* unused parameter */
//   // ctr: Counter, ticks until the task runs
//   // interval: reset value for Counter
//   SST_TimeEvt_arm(&me->te1, 2U, 2U);
// }
//
// // Task handler, everything the task does happens in here
// void task_dispatch(Task *const me, SST_Evt const *const e) {
//   // useful stuff goes here
// }
//
// // We init the task and the TimeEvt with it's signal, the signal can be used
// in
// // the handler to tell which event triggered if there are multiple
// void task_ctor(Task *const me) {
//   SST_Task_ctor(&me->super, (SST_Handler)&task_init,
//                 (SST_Handler)&task_dispatch);
//   SST_TimeEvt_ctor(&me->te1, TIMEOUT1_SIG, &me->super);
// }
//
// static Task Task_inst; // doesn't need to be global, but that's easy here
// SST_Task *const AO_Task = &Task_inst.super; // task might be defined in
// another
//                                             // file, and only a pointer
//                                             exposed
//
// // pick an otherwise unused IRQ and assign it's handler to deal with our task
// void EXTI15_IRQHandler(void) { SST_Task_activate(AO_Task); }
#endif
