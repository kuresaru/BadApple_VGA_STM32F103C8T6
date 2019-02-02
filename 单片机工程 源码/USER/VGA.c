#include "stm32f10x.h"

__align(4) u8 fb1[200][52];
static vu16 vline = 0;
static vu8 vflag = 0; //如果是1说明现在正在有效显示区
static vu8 vdraw = 0;

void VGA_InitSPI() {
	NVIC_InitTypeDef nvic;
	SPI_InitTypeDef spi;
	DMA_InitTypeDef dma;
	
	DMA_DeInit(DMA1_Channel3);
	dma.DMA_PeripheralBaseAddr = (uint32_t) &(SPI1->DR);
	dma.DMA_MemoryBaseAddr = (u32) &fb1[0][0];
	dma.DMA_DIR = DMA_DIR_PeripheralDST;
	dma.DMA_BufferSize = 52;
	dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
	dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	dma.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	dma.DMA_Mode = DMA_Mode_Normal;
	dma.DMA_Priority = DMA_Priority_VeryHigh;
	dma.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel3, &dma);
	
	spi.SPI_Direction = SPI_Direction_1Line_Tx;
	spi.SPI_Mode = SPI_Mode_Master;
	spi.SPI_DataSize = SPI_DataSize_8b;
	spi.SPI_CPOL = SPI_CPOL_Low;
	spi.SPI_CPHA = SPI_CPHA_1Edge;
	spi.SPI_NSS = SPI_NSS_Soft;
	spi.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
	spi.SPI_FirstBit = SPI_FirstBit_MSB;
	spi.SPI_CRCPolynomial = 0;
	SPI_Init(SPI1, &spi);
	SPI_CalculateCRC(SPI1, DISABLE);
	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
	SPI_Cmd(SPI1, ENABLE);
	
	nvic.NVIC_IRQChannel = DMA1_Channel3_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
	DMA1_Channel3->CCR &= ~1;
	DMA1_Channel3->CNDTR = 52;
	DMA1_Channel3->CMAR = (u32) &fb1[0][0];
	DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);
}

void VGA_InitTIM() {
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_OCInitTypeDef TIM_OCStructure;
	// --- 行同步信号配置 ---
	/*
	初始化TIM1高级定时器
	把行同步信号当成PWM处理
	*/
	//TIM1时钟配置
	TIM_DeInit(TIM1);
	TIM_TimeBaseStructure.TIM_Prescaler = 0; //预分频
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = 2048; //35156.25Hz
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
	
	//T1 通道1
	TIM_OCStructure.TIM_OCMode = TIM_OCMode_PWM2; //pwm模式2
	TIM_OCStructure.TIM_OutputState = TIM_OutputState_Enable; //正向通道有效 PA8
	TIM_OCStructure.TIM_OutputNState = TIM_OutputNState_Disable; //必须关上 否则和SPI2(SD卡)的CLK冲突
	TIM_OCStructure.TIM_Pulse = 144; //2us行同步时间
	TIM_OCStructure.TIM_OCPolarity = TIM_OCPolarity_Low;//输出极性
	//TIM_OCStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;//互补输出极性
	TIM_OCStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;
	//TIM_OCStructure.TIM_OCNIdleState = TIM_OCNIdleState_Set;
	TIM_OC1Init(TIM1, &TIM_OCStructure);
	
	//T1 通道2
	TIM_OCStructure.TIM_OCMode = TIM_OCMode_Inactive;
	TIM_OCStructure.TIM_Pulse = 352; //行同步时间+后消隐
	TIM_OC2Init(TIM1, &TIM_OCStructure);
	
	TIM_CtrlPWMOutputs(TIM1, ENABLE);//TIM1主输出
	
	TIM_SelectMasterSlaveMode(TIM1, TIM_MasterSlaveMode_Enable); //TIM1主模式
	TIM_SelectOutputTrigger(TIM1, TIM_TRGOSource_Update); //用更新事件作为触发输出
	
	// --- 场同步信号配置 ---
	TIM_SelectSlaveMode(TIM2, TIM_SlaveMode_Gated); //当触发信号为高  计数器时钟使能
	TIM_SelectInputTrigger(TIM2, TIM_TS_ITR0); //TIM2的触发源为TIM1内部触发0
	
	//TIM2时钟配置
	TIM_TimeBaseStructure.TIM_Period = 625;
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

  TIM_OCStructure.TIM_OCMode = TIM_OCMode_PWM2;
  TIM_OCStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCStructure.TIM_Pulse = 2;
  TIM_OCStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
  TIM_OC2Init(TIM2, &TIM_OCStructure);
	
	TIM_OCStructure.TIM_OCMode = TIM_OCMode_Inactive;
	TIM_OCStructure.TIM_Pulse = 24;
	TIM_OC4Init(TIM2, &TIM_OCStructure);
	
	TIM_CtrlPWMOutputs(TIM2, ENABLE);

	//TIM2中断配置
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	TIM_ITConfig(TIM2, TIM_IT_CC4, ENABLE);
	
	//TIM1中断配置
	NVIC_InitStructure.NVIC_IRQChannel = TIM1_CC_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	TIM_ITConfig(TIM1, TIM_IT_CC2, ENABLE);

	//启动定时
  TIM_Cmd(TIM2, ENABLE);
	TIM_Cmd(TIM1, ENABLE);
}

void VGA_ClearScreen() {
	u8 i,j;
	for (i = 0; i < 200; i++) {
		for (j = 0; j < 52; j++) {
			fb1[i][j] = 0;
		}
	}
}

void VGA_Init() {
	VGA_InitSPI();
	VGA_InitTIM();
	VGA_ClearScreen();
}

void VGA_SetBuf(u8 x, u8 y, u8 dat) {
	fb1[y][x] = dat;
}

//行后消隐后
__irq void TIM1_CC_IRQHandler() {
	if (vflag){
		DMA1_Channel3->CCR = 0x93;
	}
	TIM1->SR = 0xFFFB; //~TIM_IT_CC2;
}

//场后消隐后
__irq void TIM2_IRQHandler() {
	vflag = 1;
	TIM2->SR = 0xFFEF;
}

__irq void DMA1_Channel3_IRQHandler() {
	DMA1->IFCR = DMA1_IT_TC3;
	DMA1_Channel3->CCR  = 0x92;
	DMA1_Channel3->CNDTR = 52;
	vdraw++;
	if (vdraw == 3) {
		vdraw = 0;
		vline++;
		if (vline == 200) {
			vdraw = vline = vflag = 0;
			DMA1_Channel3->CMAR = (u32) &fb1[0][0];
		} else {
			DMA1_Channel3->CMAR += 52;
		}
	}
}
