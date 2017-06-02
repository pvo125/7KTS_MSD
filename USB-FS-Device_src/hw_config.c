/**
  ******************************************************************************
  * @file    hw_config.c
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


/* Includes ------------------------------------------------------------------*/

#include "hw_config.h"
#include "stm32_it.h"
#include "mass_mal.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "usb_lib.h"
#include "main.h"
#include "spi_sdcard.h"
#include "LCD_Driver.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
ErrorStatus HSEStartUpStatus;

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len);


extern uint8_t send_cmd(uint8_t cmd,uint32_t arg);
/* Private functions ---------------------------------------------------------*/

void WKUP_Callback(void){
		uint8_t n;
		GPIO_InitTypeDef  GPIO_InitStruct;
		
	//USB_Disconnect_Config();
		
	if((_7kts_State!=USB_MODE)&&(_7kts_State!=CHARGE_MODE))
	{
		USB_Cable_Config(DISABLE);
		/*	PB2=ADC_SWITCH_PIN 	*/
		GPIO_InitStruct.GPIO_Pin=ADC_SWITCH_PIN;
		GPIO_InitStruct.GPIO_Mode=GPIO_Mode_Out_OD;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_Init(ADC_SWITCH_PORT, &GPIO_InitStruct);
		ADC_SWITCH_PORT->BSRR=ADC_SWITCH_ON;
		
		ADC1->CR2|=ADC_CR2_RSTCAL;
		while(ADC1->CR2&ADC_CR2_RSTCAL) {};		
		ADC1->CR2|=ADC_CR2_CAL;
		while(ADC1->CR2&ADC_CR2_CAL) {};
		
		ADC1->CR2|=ADC_CR2_SWSTART;
	}
	else
	{
		USB_Cable_Config(ENABLE);
	}
	
	/*Настройка вывода  SW_POWER_PIN  */
	GPIO_InitStruct.GPIO_Pin =SW_POWER_PIN;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(SW_POWER_PORT, &GPIO_InitStruct);
	GPIO_SetBits(SW_POWER_PORT, SW_POWER_PIN); // включаем транзистор*
	
	/* Вывод CHARGE_INDIC_PIN в input floating*/
	GPIO_InitStruct.GPIO_Pin=CHARGE_INDIC_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(CHARGE_INDIC_PORT,&GPIO_InitStruct);
	
	/*	Настроим вывод PB8 как CardDetect	*/
	GPIO_InitStruct.GPIO_Pin=CardDetect_Pin;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_2MHz;			
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IPU;
	GPIO_Init(CardDetect_Port,&GPIO_InitStruct);	
	
	/*	Восстановим режим выводов для SW PA14 PA13 AF PP	 */
	GPIO_InitStruct.GPIO_Pin =GPIO_Pin_13|GPIO_Pin_15;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPU;
  GPIO_Init(GPIOA, &GPIO_InitStruct);
	
	GPIO_InitStruct.GPIO_Pin =GPIO_Pin_14;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IPD;
	GPIO_Init(GPIOA, &GPIO_InitStruct);
			
		/*Настройка вывода PB10 вторая кнопка */
	GPIO_InitStruct.GPIO_Pin =GPIO_Pin_10;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOB, &GPIO_InitStruct);
		 
	LCD_Init();
  /* Вывод TIM4_CH4  PWM управления подсветкой LED */
	GPIO_InitStruct.GPIO_Pin=LCD_PWM;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_Init(LCD_PORT_LED,&GPIO_InitStruct);
	
	/*	USART1_TX PA9   		*/	
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_AF_PP;
	GPIO_InitStruct.GPIO_Speed=GPIO_Speed_2MHz;
	GPIO_Init(GPIOA,&GPIO_InitStruct);	
	/* USART1_RX PA10		Input floating	*/
	GPIO_InitStruct.GPIO_Pin=GPIO_Pin_10;
	GPIO_InitStruct.GPIO_Mode=GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA,&GPIO_InitStruct);	
		
/********************  SD card leav Idle state   *****************************************************/
		if(!(CardDetect_Port->IDR & GPIO_IDR_IDR_Detect))		// Если карта присутствует в слоте
		{
			if(Stat)
				_7kts_State=SDINIT_MODE;
			else
			{
				/* Configure I/O for Flash Chip select */
				GPIO_InitStruct.GPIO_Pin   = SPI_SD_CS;
				GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_Out_PP;
				GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
				GPIO_Init(SPI_SD_CS_PORT, &GPIO_InitStruct);
				/* De-select the Card: Chip Select high */
				DESELECT();
				/* Configure SPI pins: SCK and MOSI with default alternate function (not re-mapped) push-pull */
				GPIO_InitStruct.GPIO_Pin   = SPI_SD_SCK | SPI_SD_MOSI;
				GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
				GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_AF_PP;
				GPIO_Init(SPI_SD_Port, &GPIO_InitStruct);
				/* Configure MISO as Input with internal pull-up */
				GPIO_InitStruct.GPIO_Pin   = SPI_SD_MISO;
				GPIO_InitStruct.GPIO_Mode  = GPIO_Mode_IPU;
				GPIO_Init(SPI_SD_Port, &GPIO_InitStruct);
				SPI_SD->CR1|=SPI_CR1_BR_1|SPI_CR1_BR_2;	//SPI_BaudRatePrescaler_128  110   48000000/128=187500*2
				if(send_cmd(CMD0,0)==1)
				{	// Шлем ACMD41 с битом HCS=1 пока карта не выйдет из Idle state или таймер не закончится
					SysTick->LOAD=1000000*6;							//1000ms
					SysTick->VAL=0;
					do{
						n=send_cmd(ACMD41,1UL<<30);
						}	while(n&&(!(SysTick->CTRL&SysTick_CTRL_COUNTFLAG)));
				}
				DESELECT();
				SPI_SD->CR1&=~SPI_CR1_BR;				//SPI_BaudRatePrescaler_2    000		48000000/2=24000000	
			}
		}
		else					// Если карта отсутствует в слоте		
		{
			Stat = STA_NODISK|STA_NOINIT;	/* Disk status */
			_7kts_State=NODISK_MODE;			// Режим ожидания карты 
		}
/**********************************************************************************************************/			
		TIM2->CR1 |= TIM_CR1_CEN; 	// Начать отсчёт!	
		NVIC_ClearPendingIRQ(TIM2_IRQn);
		NVIC_EnableIRQ(TIM2_IRQn);
		
		//EXTI->FTSR|=EXTI_FTSR_TR1;
}	

/*******************************************************************************
* Function Name  : Set_System
* Description    : Configures Main system clocks & power
* Input          : None.
* Return         : None.
*******************************************************************************/
void Set_System(void)
{
/* MAL configuration */
  MAL_Config();

}

/*******************************************************************************
* Function Name  : Set_USBClock
* Description    : Configures USB Clock input (48MHz)
* Input          : None.
* Return         : None.
*******************************************************************************/
void Set_USBClock(void)
{

}

/*******************************************************************************
* Function Name  : Enter_LowPowerMode
* Description    : Power-off system clocks and power while entering suspend mode
* Input          : None.
* Return         : None.
*******************************************************************************/
void Enter_LowPowerMode(void)
{
  /* Set the device state to suspend */
  bDeviceState = SUSPENDED;
}

/*******************************************************************************
* Function Name  : Leave_LowPowerMode
* Description    : Restores system clocks and power while exiting suspend mode
* Input          : None.
* Return         : None.
*******************************************************************************/
void Leave_LowPowerMode(void)
{
  DEVICE_INFO *pInfo = &Device_Info;
		
 		/* Set the device state to the correct state */
  	if (pInfo->Current_Configuration != 0)
		{
    /* Device configured */
		bDeviceState = CONFIGURED;
		}
		else
		{
		bDeviceState = ATTACHED;
		
		}
		/*Enable SystemCoreClock*/
		SystemInit();
}
	

/*******************************************************************************
* Function Name  : USB_Interrupts_Config
* Description    : Configures the USB interrupts
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_Interrupts_Config(void)
{
  //NVIC_InitTypeDef NVIC_InitStructure; 
  
  /* 2 bit for pre-emption priority, 2 bits for subpriority */
  //NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
  
#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
    /* Enable the USB Wake-up interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USB_FS_WKUP_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
#elif defined(STM32F37X)
  /* Enable the USB interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USB_LP_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
  /* Enable the USB Wake-up interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USBWakeUp_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
  
#else
  //NVIC_SetPriority(USB_HP_CAN1_TX_IRQn,0);
	NVIC_SetPriority(USB_LP_CAN1_RX0_IRQn,2);
	NVIC_SetPriority(USBWakeUp_IRQn,1);
	
	//NVIC_EnableIRQ(USB_HP_CAN1_TX_IRQn);
	NVIC_EnableIRQ(USB_LP_CAN1_RX0_IRQn);
	/* Enable the USB Wake-up interrupt */
	NVIC_EnableIRQ(USBWakeUp_IRQn);
	    
#endif /* STM32L1XX_XD */

  
#if defined(STM32F10X_HD) || defined(STM32F10X_XL) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS) 
  
	
	
	
#endif /* STM32L1XX_MD */
 
}

/*******************************************************************************
* Function Name  : Led_Config
* Description    : configure the Read/Write LEDs.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Led_Config(void)
{
  /* Configure the LEDs */
 /* STM_EVAL_LEDInit(LED1);
  STM_EVAL_LEDInit(LED2);  
  STM_EVAL_LEDInit(LED3);
  STM_EVAL_LEDInit(LED4);  */
}

/*******************************************************************************
* Function Name  : Led_RW_ON
* Description    : Turn ON the Read/Write LEDs.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Led_RW_ON(void)
{
  //STM_EVAL_LEDOn(LED3);
}

/*******************************************************************************
* Function Name  : Led_RW_OFF
* Description    : Turn off the Read/Write LEDs.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Led_RW_OFF(void)
{
  //STM_EVAL_LEDOff(LED3);
}
/*******************************************************************************
* Function Name  : USB_Configured_LED
* Description    : Turn ON the Read/Write LEDs.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_Configured_LED(void)
{
 //STM_EVAL_LEDOn(LED1);
}

/*******************************************************************************
* Function Name  : USB_NotConfigured_LED
* Description    : Turn off the Read/Write LEDs.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void USB_NotConfigured_LED(void)
{
  //STM_EVAL_LEDOff(LED1);
}

/*******************************************************************************
* Function Name  : USB_Cable_Config
* Description    : Software Connection/Disconnection of USB Cable.
* Input          : None.
* Return         : Status
*******************************************************************************/
void USB_Cable_Config (FunctionalState NewState)
{
	GPIO_InitTypeDef GPIO_InitStruct;		
	
	if(NewState==ENABLE)
	{
		GPIO_InitStruct.GPIO_Pin = USB_DISCONNECT_PIN;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_OD;
		GPIO_Init(USB_DISCONNECT_PORT, &GPIO_InitStruct);
		
		GPIO_ResetBits(USB_DISCONNECT_PORT, USB_DISCONNECT_PIN);
	}
	else
	{
		
		GPIO_InitStruct.GPIO_Pin = USB_DISCONNECT_PIN;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN_FLOATING;
		GPIO_Init(USB_DISCONNECT_PORT, &GPIO_InitStruct);
		
		//GPIO_ResetBits(USB_DISCONNECT_PORT, USB_DISCONNECT_PIN);
	
	}

}

/*******************************************************************************
* Function Name  : Get_SerialNum.
* Description    : Create the serial number string descriptor.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
void Get_SerialNum(void)
{
  uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

  Device_Serial0 = *(uint32_t*)ID1;
  Device_Serial1 = *(uint32_t*)ID2;
  Device_Serial2 = *(uint32_t*)ID3;

  Device_Serial0 += Device_Serial2;

  if (Device_Serial0 != 0)
  {
    IntToUnicode (Device_Serial0, &MASS_StringSerial[2] , 8);
    IntToUnicode (Device_Serial1, &MASS_StringSerial[18], 4);
  }
}

/*******************************************************************************
* Function Name  : HexToChar.
* Description    : Convert Hex 32Bits value into char.
* Input          : None.
* Output         : None.
* Return         : None.
*******************************************************************************/
static void IntToUnicode (uint32_t value , uint8_t *pbuf , uint8_t len)
{
  uint8_t idx = 0;
  
  for( idx = 0 ; idx < len ; idx ++)
  {
    if( ((value >> 28)) < 0xA )
    {
      pbuf[ 2* idx] = (value >> 28) + '0';
    }
    else
    {
      pbuf[2* idx] = (value >> 28) + 'A' - 10; 
    }
    
    value = value << 4;
    
    pbuf[ 2* idx + 1] = 0;
  }
}

/*******************************************************************************
* Function Name  : MAL_Config
* Description    : MAL_layer configuration
* Input          : None.
* Return         : None.
*******************************************************************************/
void MAL_Config(void)
{
  MAL_Init(0);

#if defined(STM32F10X_HD) || defined(STM32F10X_XL)
  /* Enable the FSMC Clock */
  //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_FSMC, ENABLE);
 // MAL_Init(1);
#endif /* STM32F10X_HD | STM32F10X_XL */
}

#if !defined (USE_STM32L152_EVAL) 
/*******************************************************************************
* Function Name  : USB_Disconnect_Config
* Description    : Disconnect pin configuration
* Input          : None.
* Return         : None.
*******************************************************************************/
void USB_Disconnect_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStruct;		

  /* USB_DISCONNECT_PIN used as USB pull-up */
  GPIO_InitStruct.GPIO_Pin = USB_DISCONNECT_PIN;
  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(USB_DISCONNECT_PORT, &GPIO_InitStruct);
	
#endif
  
}
//#endif /* USE_STM3210B_EVAL or USE_STM3210E_EVAL */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
