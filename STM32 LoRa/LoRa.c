#include "LoRa.h"

uint8_t _implicitHeaderMode = 0;
uint32_t _frequency = 0;
uint8_t _packetIndex = 0;
int8_t _reset = 1;
uint8_t _onReceive = 0;

/***************************
 * SPI base interface pack *
 ***************************/
uint8_t SPILora_transfer(uint8_t address, uint8_t value)
{
	uint8_t r_data = 0;
	
	GPIO_ResetBits(GPIO_SPI, NSS_Pin);
	/* Write Address Byte */
	SPI1->DR = address;
	while(!(SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE)));	// waiting for complete transmit (while TX is not empty)
	/* Write Payload Byte */
	SPI1->DR = value;
	while(!(SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE)));	// waiting for complete transmit (while TX is not empty)
	/* Read Request Byte */
	while(!(SPI_GetFlagStatus(SPI1, SPI_FLAG_RXNE)));	// waiting for complete receive (while RX is empty)
	r_data = SPI1->DR;																// if RX is not empty => read buffer
	
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

/************************************
 * Starting communication with LoRa *
 ************************************/
uint8_t LoRa_begin(uint32_t frequency)
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
	
	//writeRegister(REG_DIO_MAPPING_1, 0x00);

  return 1;
}

void explicitHeaderMode(void)
{
  _implicitHeaderMode = 0;
  writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) & 0xfe);
}

void implicitHeaderMode(void)
{
  _implicitHeaderMode = 1;
  writeRegister(REG_MODEM_CONFIG_1, readRegister(REG_MODEM_CONFIG_1) | 0x01);
}

/****************************
 * Send TX settings to LoRa *
 ****************************/
uint8_t LoRa_beginPacket(uint8_t length)
{
	writeRegister(REG_OP_MODE, MODE_STDBY);
	
	writeRegister(REG_PAYLOAD_LENGTH, 0);
	writeRegister(REG_FIFO_ADDR_PTR, 0);
	
  return 1;
}

/***********************************************
 * Take LoRa to TX mode - transmitting payload *
 ***********************************************/
void LoRa_endPacket(void)
{
  // put in TX mode
	writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

  // wait for TX done
  while ((readRegister(REG_IRQ_FLAGS) & IRQ_TX_DONE_MASK) == 0);
	writeRegister(REG_IRQ_FLAGS, IRQ_TX_DONE_MASK);
}

uint8_t LoRa_parsePacket(uint8_t size)
{
  uint8_t packetLength = 0;
  uint8_t irqFlags;
	irqFlags = readRegister(REG_IRQ_FLAGS);
	
  if (size > 0) 
	{
    implicitHeaderMode();
    writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
  } 
	else 
	{
    explicitHeaderMode();
  }
  // clear IRQ's
  writeRegister(REG_IRQ_FLAGS, irqFlags);

  if ((irqFlags & IRQ_RX_DONE_MASK) && ((irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0)) 
	{
    // received a packet
    _packetIndex = 0;
    // read packet length
    if (_implicitHeaderMode) 
		{
      packetLength = readRegister(REG_PAYLOAD_LENGTH);
    } 
		else 
		{
      packetLength = readRegister(REG_RX_NB_BYTES);
    }		
		writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR)); // set FIFO address to current RX address
    idle(); // put in standby mode
  } 
	
	else if (readRegister(REG_OP_MODE) != (MODE_LONG_RANGE_MODE | MODE_RX_SINGLE)) 
	{
    // not currently in RX mode
    writeRegister(REG_FIFO_ADDR_PTR, 0); // reset FIFO address
    // put in single RX mode
    writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_SINGLE);
  }

  return packetLength;
}

void receive(int size)
{
  if (size > 0) {
    implicitHeaderMode();

    writeRegister(REG_PAYLOAD_LENGTH, size & 0xff);
  } else {
    explicitHeaderMode();
  }

  writeRegister(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_RX_CONTINUOUS);
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

uint8_t get_length(uint8_t* data)
{
	uint8_t data_len = 0;
	while (*data++) data_len++;
	return data_len;
}

void LoRa_write(uint8_t* data)
{	
	uint8_t sum_len = 0;
	uint8_t data_len = get_length(data);
	uint8_t current_len = readRegister(REG_PAYLOAD_LENGTH);

	if ((current_len + data_len) > MAX_PKT_LENGTH) data_len = MAX_PKT_LENGTH - current_len;
	sum_len = current_len + data_len;
	writeRegister(REG_PAYLOAD_LENGTH, sum_len);
	
	while (*data) writeRegister(REG_FIFO, *data++);
	
	
}

void LoRa_writeBytes(uint8_t* data, uint8_t length)
{	
	writeRegister(REG_FIFO_ADDR_PTR, 0);
	writeRegister(REG_PAYLOAD_LENGTH, length);
	
	while (length--) writeRegister(REG_FIFO, *data++);
	writeRegister(REG_OP_MODE, MODE_TX);
}

uint8_t LoRa_readStart(void)
{
	uint8_t packetLength;
	uint8_t irqFlags = readRegister(REG_IRQ_FLAGS);
	
	writeRegister(REG_OP_MODE, MODE_RX_SINGLE);
	while ((readRegister(REG_IRQ_FLAGS) & IRQ_RX_DONE_MASK) == 0)
	{
		
	}
	
	if ((irqFlags & IRQ_RX_DONE_MASK) && (irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) 
	{
		packetLength = readRegister(REG_RX_NB_BYTES);
	}
	
	return packetLength;
}

uint8_t LoRa_read(void)
{
	uint8_t irqFlags = 0, ret = 0;
	
	writeRegister(REG_FIFO_RX_BASE_ADDR, 0);
	writeRegister(REG_OP_MODE, MODE_RX_SINGLE);
	irqFlags = readRegister(REG_IRQ_FLAGS);
	
	if ((irqFlags & IRQ_RX_DONE_MASK) != 0)
	{
		ret = readRegister(REG_FIFO);
		writeRegister(REG_IRQ_FLAGS, irqFlags);
	}
	
	return ret;
}

uint8_t LoRa_readBytes(void)
{

}

void handleDio0Rise()
{
  int irqFlags = readRegister(REG_IRQ_FLAGS);

  // clear IRQ's
  writeRegister(REG_IRQ_FLAGS, irqFlags);

  if ((irqFlags & IRQ_PAYLOAD_CRC_ERROR_MASK) == 0) {
    // received a packet
    _packetIndex = 0;

    // read packet length
    int packetLength = _implicitHeaderMode ? readRegister(REG_PAYLOAD_LENGTH) : readRegister(REG_RX_NB_BYTES);

    // set FIFO address to current RX address
    writeRegister(REG_FIFO_ADDR_PTR, readRegister(REG_FIFO_RX_CURRENT_ADDR));
		/*
    if (_onReceive) {
      _onReceive(packetLength);
    }
		*/
    // reset FIFO address
    writeRegister(REG_FIFO_ADDR_PTR, 0);
  }
}
