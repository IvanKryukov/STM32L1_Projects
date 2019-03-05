#include "main.h"

void GPIO_Settings(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitTypeDef GPIO_SimplePort;
	
	GPIO_SimplePort.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_SimplePort.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9;
	GPIO_SimplePort.GPIO_OType = GPIO_OType_PP;
	GPIO_SimplePort.GPIO_Speed = GPIO_Speed_10MHz;
	
	GPIO_Init(GPIOC, &GPIO_SimplePort);
	
	GPIO_ResetBits(GPIOC, GPIO_Pin_2 | GPIO_Pin_8 | GPIO_Pin_9);
	GPIO_SetBits(GPIOC, GPIO_Pin_5);
}

void SPI_Settings(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitSPI;
	SPI_InitTypeDef  SPI_InitTransmit;
	
	GPIO_PinAFConfig(GPIO_SPI, GPIO_PinSource5, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIO_SPI, GPIO_PinSource6, GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIO_SPI, GPIO_PinSource7, GPIO_AF_SPI1);
	
	GPIO_InitSPI.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitSPI.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitSPI.GPIO_OType = GPIO_OType_PP;
	GPIO_InitSPI.GPIO_PuPd =  GPIO_PuPd_NOPULL;
	GPIO_InitSPI.GPIO_Speed = GPIO_Speed_40MHz;	
	GPIO_Init(GPIO_SPI, &GPIO_InitSPI);
	
	GPIO_InitSPI.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitSPI.GPIO_Pin = NSS_Pin;
	GPIO_InitSPI.GPIO_OType = GPIO_OType_PP;
	GPIO_InitSPI.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIO_SPI, &GPIO_InitSPI);
	
	SPI_InitTransmit.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitTransmit.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
	SPI_InitTransmit.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitTransmit.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitTransmit.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitTransmit.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitTransmit.SPI_Mode = SPI_Mode_Master;
	SPI_InitTransmit.SPI_NSS = SPI_NSS_Soft;
	SPI_Init(SPI1, &SPI_InitTransmit);
	SPI_Cmd(SPI1, ENABLE);
}

void SPI1_SendByte(char t_data)
{
	GPIO_ResetBits(GPIO_SPI, NSS_Pin);
	SPI1->DR = t_data;
	while(!(SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE)));
	GPIO_SetBits(GPIO_SPI, NSS_Pin);
}

void SPI1_SendBytes(char* t_data)
{
	GPIO_ResetBits(GPIO_SPI, NSS_Pin);
	while(*t_data)
	{
		SPI1->DR = *t_data++;
		while(!(SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE)));
	}
	while(SPI_GetFlagStatus(SPI1, SPI_FLAG_BSY));
	GPIO_SetBits(GPIO_SPI, NSS_Pin);
}

int main()
{	
	GPIO_Settings();
	SPI_Settings();
	
	while(1)
	{			
		SPI1_SendBytes("Hello World!");	
	}
}
