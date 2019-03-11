#include "main.h"

uint8_t r_data[255];
uint32_t i = 0;

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
	SPI_InitTypeDef  SPI_InitLora;
	
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
	
	SPI_InitLora.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
	SPI_InitLora.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_128;
	SPI_InitLora.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitLora.SPI_CPOL = SPI_CPOL_Low;
	SPI_InitLora.SPI_CPHA = SPI_CPHA_1Edge;
	SPI_InitLora.SPI_FirstBit = SPI_FirstBit_MSB;
	SPI_InitLora.SPI_Mode = SPI_Mode_Master;
	SPI_InitLora.SPI_NSS = SPI_NSS_Soft;
	SPI_Init(SPI1, &SPI_InitLora);
	SPI_Cmd(SPI1, ENABLE);
}

void SPI1_SendBytes(char* t_data)
{
	GPIO_ResetBits(GPIO_SPI, NSS_Pin);
	while(*t_data)
	{
		while(!(SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE)));
		SPI1->DR = *t_data++;
	}
	while(SPI_GetFlagStatus(SPI1, SPI_FLAG_BSY));
	GPIO_SetBits(GPIO_SPI, NSS_Pin);
}

void LoRa_Setup(uint32_t freq)
{
	uint8_t b = 0;
	
	GPIOC->ODR ^= GPIO_ODR_ODR_8;
	if (!LoRa_begin(freq))
	{
		while(!b)
		{
			b = 0;
			b = LoRa_begin(freq);	
			for (i = 0; i < 100; i++);
		}		
	}
	
	writeRegister(REG_OP_MODE, MODE_STDBY);
	writeRegister(REG_OP_MODE, MODE_TX);
	
	GPIOC->ODR ^= GPIO_ODR_ODR_8;
}

uint8_t irqF = 0, pLength = 0;
int main()
{
	
	GPIO_Settings();
	SPI_Settings();
	
	LoRa_Setup(915E6);
	
	while(1)
	{
		irqF = 0;
		writeRegister(REG_FIFO_ADDR_PTR, 0); // reset FIFO address
		writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE); // set LoRa to RX mode
		
		/* waiting for RxDone interrupt */
		while (!(irqF & IRQ_RX_DONE_MASK)) 
		{
			irqF = readRegister(REG_IRQ_FLAGS);		
			GPIOC->ODR ^= GPIO_ODR_ODR_9;			
		}
		writeRegister(REG_IRQ_FLAGS, irqF);
		
		/* reading received data if CRC is right */
		if (!(irqF & IRQ_PAYLOAD_CRC_ERROR_MASK)) 
		{
			pLength = readRegister(REG_RX_NB_BYTES);
			for(i = 0; i < pLength; i++) 
			{
				r_data[i] = readRegister(REG_FIFO);
			}
		}
	}
}
