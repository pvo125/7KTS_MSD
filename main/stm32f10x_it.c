/**
  ******************************************************************************
  * @file    stm32f4xx_it.c 
  * @author  MCD Application Team
  * @version V1.3.0
  * @date    13-November-2013
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
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

/* Includes ------------------------------------------------------------------*/
#include "spi_sdcard.h"
#include "LCD_Driver.h" 
#include "usb_regs.h"

#include "main.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "usb_pwr.h"

/** @addtogroup STM32F4xx_StdPeriph_Examples
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

GPIO_InitTypeDef GPIO_InitStruct;
extern __IO uint32_t bDeviceState;
volatile uint8_t batt_indic=0xf;
volatile uint8_t time_backlight=23;
extern _7KTS_STATE volatile _7kts_State;

extern volatile uint8_t result;
extern volatile uint8_t timeout_delay;

extern volatile uint8_t battview_mode;
extern volatile uint8_t tx7kt_transaction;
extern volatile uint8_t rx7kt_transaction;
volatile uint8_t count_DMAtransfer;
extern volatile uint16_t count_AddrEEPROM;

extern uint16_t ADC1Buff[];
extern uint8_t VBAT[];
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

extern void Change_Freq(_7KTS_SPEED speed);
	



/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
/*void SysTick_Handler(void)
{
	
}*/

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f40xx.s/startup_stm32f427x.s).                         */
/******************************************************************************/

/**
  * @brief  This function handles RTC_IRQHandler interrupt request.
  * @param  None
  * @retval None
  */
void TIM2_IRQHandler(void)
{
	
	TIM2->SR&=~TIM_SR_UIF; 			//Сбрасываем флаг UIF
	
	if((_7kts_State==USB_MODE)||(_7kts_State==CHARGE_MODE))
		{
				if((batt_indic==0xB)&&(CHARGE_INDIC_PORT->IDR & CHARGE_INDIC_PIN))
					batt_indic=0xB;
				else 
				{
					batt_indic--;
					if(batt_indic==0xA)
						batt_indic=0xf;
				}
		}
	if(time_backlight!=0)
			time_backlight--;
	else if((_7kts_State==RS232_MODE)||(_7kts_State==NODISK_MODE)) 
	{
	/*Настройка вывода  SW_POWER_PIN для управления  EN NCP551SN33   */
		GPIO_InitStruct.GPIO_Pin =SW_POWER_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(SW_POWER_PORT, &GPIO_InitStruct);
	}
	
	if(time_backlight<18)
		TIM_PWM->CCER&=~TIM_CCER_CC4E;		//Выключаем подсветку если была включена
		
	timeout_delay--;
	if(!timeout_delay)
	{
		result=1;
		rx7kt_transaction=0;
	}
}	

void EXTI1_IRQHandler(void)
{
	Delay(5000);
	if(USB_DETECT_PORT->IDR & USB_DETECT_PIN)
	{		
		EXTI->IMR&=~(EXTI_IMR_MR10|EXTI_IMR_MR6);
		Change_Freq(PLL);
		USB_Cable_Config(ENABLE);
		//fSuspendEnabled=FALSE;
		_7kts_State=CHARGE_MODE;
		USB_Init();
		refresh=1;
		time_backlight=23;
		TIM_PWM->CCER|=TIM_CCER_CC4E;		//Включаем подсветку если была выключена
	}
	else
	{
		GPIO_InitStruct.GPIO_Pin =SW_POWER_PIN;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(SW_POWER_PORT, &GPIO_InitStruct);	
	}
	EXTI_ClearITPendingBit(EXTI_Line1);	
}	

void EXTI9_5_IRQHandler(void)
{
		Delay(100000);
		if(!(CardDetect_Port->IDR & GPIO_IDR_IDR_Detect))		// Если карта вставлена в слот
		{
			Stat&=~STA_NODISK;
			_7kts_State=SDINIT_MODE;
			time_backlight=23;
			TIM_PWM->CCER|=TIM_CCER_CC4E;		//Включаем подсветку если была выключена
			refresh=1;
		}
		else
		{
			Stat = STA_NODISK|STA_NOINIT;	/* Disk status */
			_7kts_State=NODISK_MODE;			// Режим ожидания карты
			time_backlight=23;
			TIM_PWM->CCER|=TIM_CCER_CC4E;		//Включаем подсветку если была выключена			
			refresh=1;
		}
	EXTI_ClearITPendingBit(EXTI_Line6);	
}
/**
  * @brief  This function handles EXTI1_IRQHandler interrupt request.
  * @param  None
  * @retval None
  */
void EXTI15_10_IRQHandler(void)
{		
		//uint32_t temp;
		//temp=(EXTI->PR&EXTI_PR_PR10)||(EXTI->PR&EXTI_PR_PR11);
		switch(EXTI->PR){
		case EXTI_PR_PR11:
			time_backlight=23;
			TIM_PWM->CCER|=TIM_CCER_CC4E;		//Включаем подсветку если была выключена
			refresh=1;
			EXTI_ClearITPendingBit(EXTI_Line11);
		break;	
		case EXTI_PR_PR10:
			Delay(5000);
			time_backlight=23;
			TIM_PWM->CCER|=TIM_CCER_CC4E;		//Включаем подсветку если была выключена
						
			if(menu_punkt==BATVIEW_MENU)
				menu_punkt=NODATA_MENU;
			else
				menu_punkt++;
			refresh=1;
			EXTI_ClearITPendingBit(EXTI_Line10);
		break;
	}
	EXTI_ClearITPendingBit(EXTI_Line6);	// На всякий случай когда карту вытащили или вставили на границе времени вкл-выкл.	
}
/**
  * @brief  This function handles ADC1_2_IRQHandler interrupt request.
  * @param  None
  * @retval None
  */
void ADC1_2_IRQHandler (void){
	uint16_t temp=0,temp1;
	
	ADC1->CR2&=~ADC_CR2_ADON;
	ADC1->CR1&=~ADC_CR1_SCAN;
	
	temp=(ADC1Buff[0]+ADC1Buff[1]+ADC1Buff[2]+ADC1Buff[3] )/4;
	temp=(6590*temp)/4095;
	if((4000<=temp)&&(temp<4200))
		batt_indic=0xB;
	else if((3900<=temp)&&(temp<4000))
		batt_indic=0xC;
	else if((3800<=temp)&&(temp<3900))
		batt_indic=0xD;
	else if((3700<=temp)&&(temp<3800))
		batt_indic=0xE;
	else if((3500<=temp)&&(temp<3700))
		batt_indic=0xF;
	else if(temp<3500)
	{
		_7kts_State=BATLOW_MODE;
		EXTI->IMR&=~(EXTI_IMR_MR10|EXTI_IMR_MR6|EXTI_IMR_MR11);
	}
	VBAT[0]=temp/1000|0x30;
	VBAT[2]=(temp%1000)/100|0x30;
	temp1=(temp%1000)%100;
	VBAT[3]=(temp1/10)|0x30;
	VBAT[4]=(temp1%10)|0x30;
	
	ADC_SWITCH_PORT->BSRR=ADC_SWITCH_OFF;
	
	time_backlight=23;
	TIM_PWM->CCER|=TIM_CCER_CC4E;		//Включаем подсветку если была выключена
}


/**
  * @brief  This function handles USB_LP_CAN1_RX0_IRQHandler interrupt request.
  * @param  None
  * @retval None
  */
void USB_LP_CAN1_RX0_IRQHandler(void){

	 USB_Istr();
}
/**
  * @brief  This function handles USBWakeUp_IRQHandler interrupt request.
  * @param  None
  * @retval None
  */
void USBWakeUp_IRQHandler (void){
	
	EXTI_ClearITPendingBit(EXTI_Line18);
}

/**
  * @brief  This function handles DMA1_Channel4_IRQHandler interrupt request.
  * @param  None
  * @retval None
  */
void DMA1_Channel4_IRQHandler(void)
{
	tx7kt_transaction=0;
	DMA1->IFCR|=DMA_IFCR_CTCIF4;		/*Clear TC5 flag */
}

/**
  * @brief  This function handles DMA1_Channel5_IRQHandler interrupt request.
  * @param  None
  * @retval None
  */
void DMA1_Channel5_IRQHandler(void)
{
	count_AddrEEPROM+=256;
	timeout_delay=2;
	rx7kt_transaction=0;
	DMA1->IFCR|=DMA_IFCR_CTCIF5;		/*Clear TC5 flag */
}


/**
  * @}
  */ 

/**
  * @}
  */ 

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
