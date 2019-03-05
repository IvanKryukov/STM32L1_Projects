#include "main.h"

void RCC_config(void)
{
	RCC_HSICmd(ENABLE);
	while (RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET){}
		
	RCC_SYSCLKConfig(RCC_SYSCLKSource_HSI);
}

void GPIO_config(void)
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

void ADC_config(void)
{
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitADC;
	GPIO_InitADC.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitADC.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitADC.GPIO_Speed = GPIO_Speed_40MHz;
	GPIO_Init(GPIOA, &GPIO_InitADC);
	
	ADC_BankSelection(ADC1, ADC_Bank_A);
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_4Cycles);
	
	ADC1->CR1 |= ADC_CR1_PDD;
	ADC1->CR1 |= ADC_CR1_PDI;
	
	ADC_InitTypeDef ADC_InitHallBat;
	ADC_InitHallBat.ADC_Resolution = ADC_Resolution_8b;
	ADC_InitHallBat.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitHallBat.ADC_NbrOfConversion = 1;
	ADC_InitHallBat.ADC_ContinuousConvMode = DISABLE;
	ADC_InitHallBat.ADC_ScanConvMode = DISABLE;
	ADC_Init(ADC1, &ADC_InitHallBat);	
	
	ADC_Cmd(ADC1, ENABLE);
}

uint8_t getValueADC(void)
{
	uint8_t temp;
	
	while((ADC1->SR & ADC_SR_RCNR));
	ADC_SoftwareStartConv(ADC1);
	while(!(ADC1->SR & ADC_SR_EOC));
	temp = (uint8_t)(ADC1->DR);
	
	return temp;
}

uint8_t dat = 0;
int main()
{	
	RCC_config();
	GPIO_config();
	ADC_config();
	
	while(1)
	{		
		dat = getValueADC();
	}
}
