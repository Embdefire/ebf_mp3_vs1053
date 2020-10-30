/**
  ******************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2020-xx-xx
  * @brief   SPI FLASH文件系统例程
  ******************************************************************
  * @attention
  *
  * 实验平台:野火 STM32H743开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :http://firestm32.taobao.com
  *
  ******************************************************************
  */  
#include "stm32h7xx.h"
#include "main.h"
#include "./led/bsp_led.h"
#include "./usart/bsp_debug_usart.h"
#include "ff.h"
#include "./fatfs/drivers/fatfs_flash_qspi.h"
//#include "./delay/core_delay.h"
#include "./flash/bsp_spi_flash.h"
#include  "VS1053.h"
#include "string.h"
#include "flac.h"
/**
  ******************************************************************************
  *                              定义变量
  ******************************************************************************
  */
char SPIPath[4]; /* SPI flash逻辑驱动器路径 */
FATFS fs;													/* FatFs文件系统对象 */
FIL file;													/* 文件对象 */
FRESULT res_flash;                /* 文件操作结果 */
UINT bw;                    
uint8_t  buffer[BUFSIZE];

static void SystemClock_Config(void);
extern FATFS flash_fs;
extern Diskio_drvTypeDef  SPI_Driver;
extern void SPI_FLASH_Init(void);
/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */

int main(void)
{
	HAL_Init();
	/* 系统时钟初始化成400MHz */
	SystemClock_Config();
	/* LED 端口初始化 */
	LED_GPIO_Config();	
	LED1_ON;
	
	/* 配置串口1为：115200 8-N-1 */
	DEBUG_USART_Config();
  
  printf("****** 这是一个SPI FLASH 文件系统实验 ******\r\n");
	
	__IO uint32_t DeviceID = 0;
	__IO uint32_t FlashID = 0;
	SPI_FLASH_Init();
	HAL_Delay( 200 );
	
  //链接驱动器，创建盘符
  FATFS_LinkDriver(&SPI_Driver, SPIPath);
	//在外部SPI Flash挂载文件系统，文件系统挂载时会对SPI设备初始化
	res_flash = f_mount(&fs,"0:",1);
	
  VS_Init(); 
  printf("vs1053:%4X\n",VS_Ram_Test());
  HAL_Delay(100);
  VS_Sine_Test();	
  VS_HD_Reset();
  VS_Soft_Reset();
  
	while(1)
	{
		// 测试歌曲放在外部FLASH W25Q64中
		vs1053_player_song("0:TestFile.mp3");
		printf("MusicPlay End\n");
	}		

	/* 不再使用文件系统，取消挂载文件系统 */
	f_mount(NULL,"0:",1);

}

void vs1053_player_song(uint8_t *filepath)
{
	uint16_t i=0;
	
	VS_Restart_Play();  					
	VS_Set_All();        							 
	VS_Reset_DecodeTime();
	
	if(strstr((const char*)filepath,".flac")||strstr((const char*)filepath,".FLAC"))
		VS_Load_Patch((uint16_t*)vs1053b_patch,VS1053B_PATCHLEN);
	
	res_flash=f_open(&file,(const TCHAR*)filepath,FA_READ);

	if(res_flash==0)
	{ 
		VS_SPI_SpeedHigh();				   
		while(1)
		{
			i=0;	
			res_flash=f_read(&file,buffer,BUFSIZE,(UINT*)&bw);		
			do
			{  	
				if(VS_Send_MusicData(buffer+i)==0)
				{
					i+=32;
				}
			}while(i<bw);
			
			if(bw!=BUFSIZE||res_flash!=0)
			{
				break;	  
			}
			LED2_TOGGLE;
		}
		f_close(&file);
	}	  					     	  
}

/**
  * @brief  System Clock 配置
  *         system Clock 配置如下: 
	*            System Clock source  = PLL (HSE)
	*            SYSCLK(Hz)           = 480000000 (CPU Clock)
	*            HCLK(Hz)             = 240000000 (AXI and AHBs Clock)
	*            AHB Prescaler        = 2
	*            D1 APB3 Prescaler    = 2 (APB3 Clock  120MHz)
	*            D2 APB1 Prescaler    = 2 (APB1 Clock  120MHz)
	*            D2 APB2 Prescaler    = 2 (APB2 Clock  120MHz)
	*            D3 APB4 Prescaler    = 2 (APB4 Clock  120MHz)
	*            HSE Frequency(Hz)    = 25000000
	*            PLL_M                = 5
	*            PLL_N                = 192
	*            PLL_P                = 2
	*            PLL_Q                = 4
	*            PLL_R                = 2
	*            VDD(V)               = 3.3
	*            Flash Latency(WS)    = 4
  * @param  None
  * @retval None
  */
/**
  * @brief System Clock Configuration
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  HAL_StatusTypeDef ret = HAL_OK;
  
  /*使能供电配置更新 */
  MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0);

  /* 当器件的时钟频率低于最大系统频率时，电压调节可以优化功耗，
		 关于系统频率的电压调节值的更新可以参考产品数据手册。  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
 
  /* 启用HSE振荡器并使用HSE作为源激活PLL */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
  RCC_OscInitStruct.CSIState = RCC_CSI_OFF;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;

  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLQ = 24;  /* fdcan_ker_ck = 40 MHz */
 
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
  
	/* 选择PLL作为系统时钟源并配置总线时钟分频器 */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK  | \
																 RCC_CLOCKTYPE_HCLK    | \
																 RCC_CLOCKTYPE_D1PCLK1 | \
																 RCC_CLOCKTYPE_PCLK1   | \
                                 RCC_CLOCKTYPE_PCLK2   | \
																 RCC_CLOCKTYPE_D3PCLK1);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;  
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2; 
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2; 
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2; 
  ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
}
/****************************END OF FILE***************************/


