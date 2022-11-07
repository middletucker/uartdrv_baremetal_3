/***************************************************************************//**
 * @file
 * @brief uartdrv examples functions
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
// Define module name for Power Manager debuging feature.
#define CURRENT_MODULE_NAME    "APP_COMMON_EXAMPLE_UARTDRV"
#include "em_chip.h"
#include <stdio.h>
#include <string.h>
#include "uartdrv_app.h"
#include "sl_uartdrv_instances.h"
#include "sl_power_manager.h"

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/
#define INPUT_BUFSIZE    80

#define OUTPUT_BUFSIZE (INPUT_BUFSIZE + 40)

/*******************************************************************************
 ***************************  LOCAL VARIABLES   ********************************
 ******************************************************************************/
/* Message Buffers*/
unsigned char incoming;
unsigned char outgoing;
/*******************************************************************************
 **************************   FUNCTIONS   *******************************
 ******************************************************************************/
/* LED Set-up */
void init_GPIO(){
  CHIP_Init();
  CMU_ClockEnable(cmuClock_GPIO, 1);

  GPIO_PinModeSet(gpioPortF, 4, gpioModePushPull, 0); //LED 0
  GPIO_PinModeSet(gpioPortF, 5, gpioModePushPull, 0); //LED1
  GPIO_DriveStrengthSet(gpioPortF, gpioDriveStrengthStrongAlternateStrong);
}

// Hook for power manager. The application will not prevent the
// power manager from entering sleep.
bool app_is_ok_to_sleep(void)
{
  return true;
}

// Hook for power manager. The application will not prevent the
// power manager from re-entering sleep after an interrupt is serviced.
sl_power_manager_on_isr_exit_t app_sleep_on_isr_exit(void)
{
  return SL_POWER_MANAGER_SLEEP;
}

/***************************************************************************//**
 * Initialize example.
 ******************************************************************************/
void uartdrv_app_init(void)
{
  // Require at least EM2 from Power Manager
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM2);
  init_GPIO();
}

/* Compare function for incoming vs outgoing */

bool compare(unsigned char i){
  return i==incoming;
}
// If i = incoming, return i

/* Set the message to a character */
void setMessage(unsigned char o){
  outgoing = o;
}
// not sure purpose of this


/* FSM (Finite State Machine) to track where in the protocol we are */
int messageState = 0; // The state of the message to the WGM110

void afterTransmit(struct UARTDRV_HandleData *handle,
                   Ecode_t transferStatus,
                   uint8_t *data,
                   UARTDRV_Count_t transferCount){ // function prototype
if (messageState == 0 && transferStatus == ECODE_OK && transferCount == 1)
  messageState = 1;
// If all of these conditions are met, the transaction has occurred, move onto the next state.
}

void send() {
  messageState = 0;
  int retry = 10;
  while (messageState == 0 && retry > 0){
      UARTDRV_Transmit(sl_uartdrv_leuart_vcom_handle, &outgoing, 1, afterTransmit);
      messageState = 1;
      retry--;
      sl_sleeptimer_delay_millisecond(10);
  }
}

void afterReceive(struct UARTDRV_HandleData *handle,
                  Ecode_t transferStatus,
                  uint8_t *data,
                  UARTDRV_Count_t transferCount){
  if(messageState == 1 && transferStatus == ECODE_OK && transferCount == 1)
    messageState = 2;
}

/* Send the actual message with a few retries built in */




/*Function to make effort to receive the message. It is non-blocking, which means that it will continue
 * to run the rest of the code even if it doesn't happen successfully, retries 10 times*/
bool receive() {
  messageState = 1;
  int retry = 10;
  while(messageState == 1 && retry > 0){
      UARTDRV_Receive(sl_uartdrv_leuart_vcom_handle, &incoming, 1, afterReceive);
      retry--;
      sl_sleeptimer_delay_millisecond(10);
  }
  return retry > 0; //it will return true if it is still retrying
}

/* Check if incoming matches outgoing and turn on/off LEDS */
bool checkResponse(unsigned char expected)  {
  if (compare(expected))  {
      GPIO_PinOutSet(gpioPortF, 4);
      incoming = 0;
      return true;
  }
  else {
      GPIO_PinOutClear(gpioPortF, 4);
      return false;
  }
  return false; // checkResponse function will return false by default
}

/* Loopback protocol*/

unsigned char helloMsg = 0xA5;
unsigned char helloMsgResp = 0xA5;

bool sayHello() {
  setMessage(helloMsg); //cmd_system_hello
  send();
  receive();
  return checkResponse(helloMsgResp);
}


/***************************************************************************//**
 * Check that we can say hello and get the correct response
 ******************************************************************************/
void uartdrv_app_process_action(void)
{
  GPIO_PinOutClear(gpioPortF,4);
  GPIO_PinOutClear(gpioPortF,5);
  sl_sleeptimer_delay_millisecond(250);
  if (sayHello()) {
      GPIO_PinOutSet(gpioPortF, 5);
      sl_sleeptimer_delay_millisecond(1000);
  }
  GPIO_PinOutSet(gpioPortF, 5); //short time on
  sl_sleeptimer_delay_millisecond(250);
}

/***************************************************************************//**
 * Ticking function.
 ******************************************************************************/

