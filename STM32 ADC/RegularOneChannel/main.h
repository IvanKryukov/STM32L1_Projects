#ifndef MAIN_H
#define MAIN_H

#include "stm32l1xx.h"
#include "stm32l1xx_rcc.h"
#include "stm32l1xx_adc.h"
#include "stm32l1xx_gpio.h"
#include "stm32l1xx_usart.h"

void RCC_config(void);
void GPIO_config(void);
void SPI_config(void);
void USART_config(void);
void USART_SendBytes(uint8_t* data, uint8_t length);
void SPI1_SendBytes(char* t_data);
void ADC_config(void);
void ControlPins_config(void);
void LoRa_setState(FunctionalState state);
void BOOST_setState(FunctionalState state);
void LoRa_Setup(void);

int main(void);

#endif
