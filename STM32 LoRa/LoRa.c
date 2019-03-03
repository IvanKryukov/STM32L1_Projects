#include "LoRa.h"



uint8_t _implicitHeaderMode = 0;
uint32_t _frequency = 0;
uint8_t _packetIndex = 0;
int8_t _reset = 1;
uint8_t _onReceive = 0;


uint8_t SPILora_transfer(uint8_t address, uint8_t value)
{
	uint8_t r_data = 0;
	
	GPIO_ResetBits(GPIO_SPI, NSS_Pin);
	
	SPI1->DR = address;
	while(!(SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE)));
	
	SPI1->DR = value;
	while(!(SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE)));
	
	while(!(SPI_GetFlagStatus(SPI1, SPI_FLAG_RXNE)));
	r_data = SPI1->DR;
	
	while(SPI_GetFlagStatus(SPI1, SPI_FLAG_BSY));
	
	GPIO_SetBits(GPIO_SPI, NSS_Pin);
	
	return r_data;
}

void writeRegister(uint8_t address, uint8_t value)
{
	SPILora_transfer(address | 0x80, value);
}

uint8_t readRegister(uint8_t address)
{
	return SPILora_transfer(address & 0x7f, 0x00);	
}

int LoRa_begin(uint32_t frequency)
{
	uint32_t i = 0;
	/* Start SPI */
	
	if (_reset != -1) 
	{
    // perform reset
		GPIO_ResetBits(GPIOC, GPIO_Pin_5);
    for(i = 0; i < 100000; i++);
    GPIO_SetBits(GPIOC, GPIO_Pin_5);
    for(i = 0; i < 100000; i++);
  }

  // check version
  uint8_t version;
	version = readRegister(REG_VERSION);
  if (version != 0x12) 
	{
		return 0;
	}

  // put in sleep mode
  sleep();

  // set frequency
  setFrequency(frequency);

  // set base addresses
  writeRegister(REG_FIFO_TX_BASE_ADDR, 0);
  writeRegister(REG_FIFO_RX_BASE_ADDR, 0);

  // set LNA boost
  writeRegister(REG_LNA, readRegister(REG_LNA) | 0x03);

  // set auto AGC
  writeRegister(REG_MODEM_CONFIG_3, 0x04);

  // set output power to 17 dBm
  setTxPower(20, PA_OUTPUT_PA_BOOST_PIN);

  // put in standby mode
  idle();

  return 1;
}

uint8_t write(uint8_t *buffer, uint8_t size)
{
	uint8_t i;
	int currentLength = readRegister(REG_PAYLOAD_LENGTH);

  // check size
  if ((currentLength + size) > MAX_PKT_LENGTH) size = MAX_PKT_LENGTH - currentLength;

  // write data
  for (i = 0; i < size; i++) writeRegister(REG_FIFO, buffer[i]);

  // update length
  writeRegister(REG_PAYLOAD_LENGTH, currentLength + size);
  return size;
}

int8_t read(void)
{
  if (!available()) {
    return -1;
  }

  _packetIndex++;

  return readRegister(REG_FIFO);
}

uint8_t available(void)
{
  return (readRegister(REG_RX_NB_BYTES) - _packetIndex);
}

void idle(void)
{
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
}

void sleep(void)
{
  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
}

void setTxPower(int level, int outputPin)
{
  if (PA_OUTPUT_RFO_PIN == outputPin) 
	{
    // RFO
    if (level < 0) level = 0;
    else if (level > 14) level = 14;   

    writeRegister(REG_PA_CONFIG, 0x70 | level);
  } 
	else 
	{
    // PA BOOST
    if (level < 2) level = 2;
    else if (level > 17) level = 17;
    writeRegister(REG_PA_CONFIG, PA_BOOST | (level - 2));
  }
}

void setFrequency(uint32_t frequency)
{
  _frequency = frequency;

  uint64_t frf = ((uint64_t)frequency << 19) / 32000000;

  writeRegister(REG_FRF_MSB, (uint8_t)(frf >> 16));
  writeRegister(REG_FRF_MID, (uint8_t)(frf >> 8));
  writeRegister(REG_FRF_LSB, (uint8_t)(frf >> 0));
}

void LoRa_write(char* data)
{	
	
	writeRegister(REG_OP_MODE, MODE_TX);
	writeRegister(REG_FIFO_ADDR_PTR, 0);
	writeRegister(REG_PAYLOAD_LENGTH, get_length(data));
	
	while (*data) writeRegister(REG_FIFO, *data++);
}

uint8_t get_length(char* data)
{
	uint8_t data_len = 0;
	while (*data++) data_len++;
	return data_len;
}

