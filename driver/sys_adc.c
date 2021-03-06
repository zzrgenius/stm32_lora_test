 
#include "stm32f10x_dma.h"
 
#include "stm32f10x_adc.h"  
#include "sys_adc.h"
  
  #define AD_BUFSIZE 32
u16 AD_Value[AD_BUFSIZE] ;
u16 gAdcFilter1, gAdcFilter2 ;
void sys_adc_dma_config(void)
{
    DMA_InitTypeDef DMA_InitStructure;
		NVIC_InitTypeDef NVIC_InitStructure;
		RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	
		NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn; 
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2; 
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 
    NVIC_Init(&NVIC_InitStructure);          // Enable the DMA Interrupt 
	
    DMA_DeInit(DMA1_Channel1);//将DMA的通道1寄存器重设为缺省值
    DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&ADC1->DR; //DMA外设ADC基地址
    DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&AD_Value;//DMA内存基地址
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC; //内存作为数据传输的目的地
    DMA_InitStructure.DMA_BufferSize  = AD_BUFSIZE;//DMA通道的DMA缓存的大小
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//外设地址寄存器不变
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable; //内存地址寄存器递增
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;//数据宽度为16位
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;//数据宽度为16位
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;//工作在 
    DMA_InitStructure.DMA_Priority = DMA_Priority_High; //DMA通道?x拥有高优先级
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;//DMA通道x没有设置为内存到内存传输
    DMA_Init(DMA1_Channel1,&DMA_InitStructure);//根据DMA_InitStruct中指定的参数初始化DMA的通道 
		DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);   //open interrupt  use for filter
    DMA_Cmd(DMA1_Channel1,ENABLE); //启动DMA通道?
		
		ADC_SoftwareStartConvCmd(ADC1,ENABLE);
}

 void sys_adc_init(void)
{
    ADC_InitTypeDef  ADC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO,ENABLE);
    GPIO_InitStructure.GPIO_Pin  =  GPIO_Pin_1| GPIO_Pin_0;
  //  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA,&GPIO_InitStructure); //默认速度为两兆
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);
    ADC_DeInit(ADC1);
    ADC_InitStructure.ADC_Mode                  =   ADC_Mode_Independent;  //独立模式
    ADC_InitStructure.ADC_ScanConvMode          =  	ENABLE;  // DISABLE;    //连续多通道模式
    ADC_InitStructure.ADC_ContinuousConvMode    =   ENABLE;      //连续转换
    ADC_InitStructure.ADC_ExternalTrigConv      =   ADC_ExternalTrigConv_None; //转换不受外界决定
    ADC_InitStructure.ADC_DataAlign             =   ADC_DataAlign_Right;   //右对齐
    ADC_InitStructure.ADC_NbrOfChannel          =   2;       //扫描通道数
    ADC_Init(ADC1,&ADC_InitStructure);
    ADC_RegularChannelConfig(ADC1,ADC_Channel_0, 1,ADC_SampleTime_55Cycles5); //
		ADC_RegularChannelConfig(ADC1,ADC_Channel_1, 2,ADC_SampleTime_55Cycles5); //
 //	ADC_RegularChannelConfig(ADC1,ADC_Channel_16, 2,ADC_SampleTime_55Cycles5);
		ADC_DMACmd(ADC1,ENABLE);//
    ADC_Cmd  (ADC1,ENABLE);             //使能或者失能指定的ADC
//		ADC_TempSensorVrefintCmd(ENABLE);
    //ADC_SoftwareStartConvCmd(ADC1,ENABLE);//使能或者失能指定的ADC的软件转换启动功能
    ADC_ResetCalibration(ADC1);//复位指定的ADC1的校准寄存器
    while(ADC_GetResetCalibrationStatus(ADC1));//获取ADC1复位校准寄存器的状态,设置状态则等�
    ADC_StartCalibration(ADC1);//开始指定ADC1的校准状态
    while(ADC_GetCalibrationStatus(ADC1)); //获取指定ADC1
		
		sys_adc_dma_config();
}

 
 //u16 adc_filter[3];

 
//Temperature= (1.42 - ADC_Value*3.3/4096)*1000/4.35 + 25
void filter(void)
{
	u32  sum1 = 0;
	u32 sum2 = 0;
 
   u8 i;
   for(i=0;i<(AD_BUFSIZE>>1);i++)
   {
	   sum1 +=	 AD_Value[2*i] ;
		 sum2 += 	 AD_Value[2*i+1];   
   }
   gAdcFilter1   = (u16) (sum1/(AD_BUFSIZE>>1)) ;
	 gAdcFilter2	 = (u16) (sum2/(AD_BUFSIZE>>1)) ;
}
u16 CurrDataCounterEnd = 0;
void DMA1_Channel1_IRQHandler(void)
{
	if(DMA_GetITStatus(DMA1_IT_TC1) != RESET)
	{
		DMA_Cmd(DMA1_Channel1,DISABLE);
		 CurrDataCounterEnd=DMA_GetCurrDataCounter(DMA1_Channel1);
		DMA_ClearITPendingBit(DMA1_IT_TC1);
		filter();		
		DMA_SetCurrDataCounter(DMA1_Channel1,AD_BUFSIZE);
		DMA_Cmd(DMA1_Channel1,ENABLE);
	}
}
 
