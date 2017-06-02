/**
  ******************************************************************************
  * @file    mass_mal.c
  * @author  MCD Application Team
  * @version V4.0.0
  * @date    21-January-2013
  * @brief   Medium Access Layer interface
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
#include "platform_config.h"
#include "mass_mal.h"
#include "spi_sdcard.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t Mass_Memory_Size[2];
uint32_t Mass_Block_Size[2];
uint32_t Mass_Block_Count[2];
__IO uint32_t Status = 0;


extern volatile DSTATUS Stat;
extern CSDTypeDef csd;
extern uint16_t SD_SECTOR_SIZE;
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
* Function Name  : MAL_Init
* Description    : Initializes the Media on the STM32
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Init(uint8_t lun)
{
  uint16_t status = MAL_OK;

  switch (lun)
  {
    case 0:
      if(Stat!=0)
			{
				if(sd_card_init()!=0)
					return MAL_FAIL;
				SPI_SD_GETCSD();
				
			}
    break;
#ifdef USE_STM3210E_EVAL
    case 1:
      NAND_Init();
      break;
#endif
    default:
      return MAL_FAIL;
  }
  return status;
}
/*******************************************************************************
* Function Name  : MAL_Write
* Description    : Write sectors
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_Write(uint8_t lun, uint32_t Memory_Offset, uint32_t *Writebuff, uint16_t Transfer_Length)
{
	DRESULT res;
  uint16_t Length=Transfer_Length/512;
	switch (lun)
  {
    case 0:
			if(Length>1)
				res = SPI_SD_WriteMultiBlock(Memory_Offset,(uint8_t*)Writebuff,Length);
			else	
				res = SPI_SD_WriteBlock(Memory_Offset,(uint8_t*)Writebuff);
			if ( res !=RES_OK )
				{
					return MAL_FAIL;
				}      
     break;
    case 1:
      //NAND_Write(Memory_Offset, Writebuff, Transfer_Length);
     break;
     default:
      return MAL_FAIL;
  }
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : MAL_Read
* Description    : Read sectors
* Input          : None
* Output         : None
* Return         : Buffer pointer
*******************************************************************************/
uint16_t MAL_Read(uint8_t lun, uint32_t Memory_Offset, uint32_t *Readbuff, uint16_t Transfer_Length)
{
	DRESULT res;
	uint16_t lenght=Transfer_Length/512;
  switch (lun)
  {
    case 0:
			if(lenght>1)
				res=SPI_SD_ReadMultiBlock(Memory_Offset,(uint8_t*)Readbuff,lenght);
			else	
				res=SPI_SD_ReadBlock(Memory_Offset,(uint8_t*)Readbuff);
      
			if ( res != RES_OK )
      {
        return MAL_FAIL;
      }
     break;

    case 1:
      //NAND_Read(Memory_Offset, Readbuff, Transfer_Length);
     break;
    default:
      return MAL_FAIL;
  }
  return MAL_OK;
}

/*******************************************************************************
* Function Name  : MAL_GetStatus
* Description    : Get status
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
uint16_t MAL_GetStatus (uint8_t lun)
{
 if (lun == 0)
  {
   if (Stat==0)
    {
      Mass_Block_Count[0] = csd.sector_count;
      Mass_Block_Size[0] = 512;
			Mass_Memory_Size[0] = (Mass_Block_Count[0] * Mass_Block_Size[0]);
       return MAL_OK;
		}
	}	
#if 0 //def USE_STM3210E_EVAL
  else
  {
    FSMC_NAND_ReadID(&NAND_ID);
    if (NAND_ID.Device_ID != 0 )
    {
      /* only one zone is used */
      Mass_Block_Count[1] = NAND_ZONE_SIZE * NAND_BLOCK_SIZE * NAND_MAX_ZONE ;
      Mass_Block_Size[1]  = NAND_PAGE_SIZE;
      Mass_Memory_Size[1] = (Mass_Block_Count[1] * Mass_Block_Size[1]);
      return MAL_OK;
    }
  }
#endif /* USE_STM3210E_EVAL */
 // STM_EVAL_LEDOn(LED2);
  return MAL_FAIL;
}

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

