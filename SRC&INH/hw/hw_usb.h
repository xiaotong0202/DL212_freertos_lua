/**
  ******************************************************************************
  * @file    hw_config.h
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Hardware Configuration & Setup
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2013 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HW_CONFIG_H
#define __HW_CONFIG_H

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx.h"
#include "usb_type.h"

 
#define USB_DETECT_PIN           GPIO_Pin_15
#define USB_DETECT_PORT          GPIOB
#define USB_DETECT_CLK           RCC_AHBPeriph_GPIOB
#define USB_DETECT_PINSOURCE     EXTI_PinSource15
#define USB_DETECT_PORTSOURCE    EXTI_PortSourceGPIOB
#define USB_DETECT_EXTI_LINE     EXTI_Line15
#define USB_DETECT_EXTI_IRQn     EXTI15_10_IRQn
#define USB_DETECT()             GPIO_ReadInputDataBit(USB_DETECT_PORT,USB_DETECT_PIN)

#define MASS_MEMORY_START     0x04002000
#define BULK_MAX_PACKET_SIZE  0x00000040
  
#define ID1 (0x1FF80050)
#define ID2 (0x1FF80054)
#define ID3 (0x1FF80064)
 
#if !defined(USB_USE_EXTERNAL_PULLUP)
  #define STM32L15_USB_CONNECT         SYSCFG_USBPuCmd(ENABLE)
  #define STM32L15_USB_DISCONNECT      SYSCFG_USBPuCmd(DISABLE)
#elif defined(USB_USE_EXTERNAL_PULLUP) 
  #define STM32L15_USB_CONNECT         GPIO_ResetBits(USB_DISCONNECT, USB_DISCONNECT_PIN)
  #define STM32L15_USB_DISCONNECT      GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN)
#endif
 
/* Exported functions ------------------------------------------------------- */
void Set_System(void);
void Set_USBClock(void);
void Enter_LowPowerMode(void);
void Leave_LowPowerMode(void);
void USB_Interrupts_Config(void);
void USB_Cable_Config (FunctionalState NewState);
void Get_SerialNum(void);
void LCD_Control(void);
uint32_t CDC_Send_DATA (uint8_t *ptrBuffer, uint8_t Send_length);
uint32_t CDC_Receive_DATA(void);
/* External variables --------------------------------------------------------*/

#endif  /*__HW_CONFIG_H*/
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
