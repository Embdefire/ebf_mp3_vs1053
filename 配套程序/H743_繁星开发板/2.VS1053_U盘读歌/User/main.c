/**
  ******************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2019-xx-xx
  * @brief   GPS����
  ******************************************************************
  * @attention
  *
  * ʵ��ƽ̨:Ұ�� STM32H743������ 
  * ��̳    :http://www.firebbs.cn
  * �Ա�    :https://fire-stm32.taobao.com
  *
  ******************************************************************
  */  
#include "stm32h7xx.h"
#include "main.h"
#include "string.h"
#include "./led/bsp_led.h" 
#include "./usart/bsp_usart.h"
#include "./delay/core_delay.h"   
#include "usb_host.h"
#include "ff.h"
#include  "VS1053.h"
#include "string.h"
#include "flac.h"

extern DMA_HandleTypeDef DMA_Handle;

char SDPath[4];                /* SD�߼�������·�� */
FATFS fs[2];
FIL file;
FRESULT res; 
UINT br, bw;            					/* File R/W count */
uint8_t  buffer[BUFSIZE];

extern uint8_t USBDevice_Ready;
/**
  * @brief  ������
  * @param  ��
  * @retval ��
  */
int main(void)
{
	uint16_t state;
	HAL_Init();
	/* ϵͳʱ�ӳ�ʼ����480MHz */
  SystemClock_Config();
	
	/* LED �˿ڳ�ʼ�� */
	LED_GPIO_Config();	 

  LED1_ON;

	MX_USB_HOST_Init();

	/*���ڳ�ʼ��*/
	UARTx_Config();
  printf("��ӭʹ��Ұ��H743������ ���ֲ�������!\r\n");
  while(1)
	{
		MX_USB_HOST_Process();
		if(USBDevice_Ready == 1)
		{
			//���ⲿU�̹����ļ�ϵͳ���ļ�ϵͳ����ʱ���U�̳�ʼ��
			res = f_mount(&fs[0],"0:",1);  
			if(res != FR_OK)
			{
			  printf("f_mount 1ERROR!������������U��Ȼ�����¸�λ������!\r\n");
			}

#if Other_Part
			res = f_mount(&sd_fs[1],"1:",1);
		  
			if(res != FR_OK)
			{
			  printf("f_mount 2ERROR!������������U��Ȼ�����¸�λ������!");
			}
#endif
			VS_Init();
			state = VS_Ram_Test();
			printf("vs1053:%4X\n",state);
			if(state != 0x83FF)
			{
				printf("vs1053��ʼ��ʧ��\r\n");
				while(1);
			}
			HAL_Delay(100);
			VS_Sine_Test();	
			VS_HD_Reset();
			VS_Soft_Reset();
			while(1)
			{
				// ���Ը�������SD����Ŀ¼��
				vs1053_player_song("0:TestFile.mp3");
				printf("MusicPlay End\n");
			}
		}
	}
}

void vs1053_player_song(uint8_t *filepath)
{
	uint16_t i=0;
	
	VS_Restart_Play();  					
	VS_Set_All();        							 
	VS_Reset_DecodeTime();
	
	if(strstr((const char*)filepath,".flac")||strstr((const char*)filepath,".FLAC"))
		VS_Load_Patch((uint16_t*)vs1053b_patch,VS1053B_PATCHLEN);
	
	res=f_open(&file,(const TCHAR*)filepath,FA_READ);

	if(res==0)
	{ 
		VS_SPI_SpeedHigh();				   
		while(1)
		{
			i=0;	
			res=f_read(&file,buffer,BUFSIZE,(UINT*)&bw);		
			do
			{  	
				if(VS_Send_MusicData(buffer+i)==0)
				{
					i+=32;
				}
			}while(i<bw);
			
			if(bw!=BUFSIZE||res!=0)
			{
				break;	  
			}
			LED2_TOGGLE;
		}
		f_close(&file);
	}	  					     	  
}
/**
  * @brief  System Clock ����
  *         system Clock ��������: 
	*            System Clock source  = PLL (HSE)
	*            SYSCLK(Hz)           = 480000000 (CPU Clock)
	*            HCLK(Hz)             = 200000000 (AXI and AHBs Clock)
	*            AHB Prescaler        = 2
	*            D1 APB3 Prescaler    = 2 (APB3 Clock  100MHz)
	*            D2 APB1 Prescaler    = 2 (APB1 Clock  100MHz)
	*            D2 APB2 Prescaler    = 2 (APB2 Clock  100MHz)
	*            D3 APB4 Prescaler    = 2 (APB4 Clock  100MHz)
	*            HSE Frequency(Hz)    = 25000000
	*            PLL_M                = 5
	*            PLL_N                = 160
	*            PLL_P                = 2
	*            PLL_Q                = 4
	*            PLL_R                = 2
	*            VDD(V)               = 3.3
	*            Flash Latency(WS)    = 4
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  HAL_StatusTypeDef ret = HAL_OK;
  
  /*ʹ�ܹ������ø��� */
  MODIFY_REG(PWR->CR3, PWR_CR3_SCUEN, 0);

  /* ��������ʱ��Ƶ�ʵ������ϵͳƵ��ʱ����ѹ���ڿ����Ż����ģ�
		 ����ϵͳƵ�ʵĵ�ѹ����ֵ�ĸ��¿��Բο���Ʒ�����ֲᡣ  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}
 
  /* ����HSE������ʹ��HSE��ΪԴ����PLL */
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
  RCC_OscInitStruct.PLL.PLLQ = 24;
 
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
  if(ret != HAL_OK)
  {
    while(1) { ; }
  }
  
	/* ѡ��PLL��Ϊϵͳʱ��Դ����������ʱ�ӷ�Ƶ�� */
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
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USB|RCC_PERIPHCLK_SPI1|RCC_PERIPHCLK_SPI2;
  PeriphClkInitStruct.PLL3.PLL3M = 5;
  PeriphClkInitStruct.PLL3.PLL3N = 192;
  PeriphClkInitStruct.PLL3.PLL3P = 3;
  PeriphClkInitStruct.PLL3.PLL3Q = 20;
  PeriphClkInitStruct.PLL3.PLL3R = 2;
  PeriphClkInitStruct.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_2;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = RCC_PLL3VCOWIDE;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;
  PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
  PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
  PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL3;
	
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    while(1) { ; }
  }
  /** Enable USB Voltage detector 
  */
  HAL_PWREx_EnableUSBVoltageDetector();
}

void Delay(__IO uint32_t nCount)	 //�򵥵���ʱ����
{
	for(; nCount != 0; nCount--);
}
/****************************END OF FILE***************************/
