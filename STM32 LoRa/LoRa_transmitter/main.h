#ifndef MAIN_H
#define MAIN_H

#include "stm32l1xx.h"
#include "stm32l1xx_rcc.h"
#include "stm32l1xx_adc.h"
#include "stm32l1xx_gpio.h"
#include "stm32l1xx_spi.h"
#include "stm32l1xx_exti.h"
#include "LoRa.h"

#define GPIO_SPI 	GPIOA
#define NSS_Pin 	GPIO_Pin_4
#define SPI_Lora	SPI1

void GPIO_Settings(void);
void SPI_Settings(void);
void SPI1_SendBytes(char* t_data);
int main(void);

#endif
