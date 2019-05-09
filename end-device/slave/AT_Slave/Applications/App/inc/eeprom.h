#define EEPROM_BASE_ADDRESS 0x08080000UL

/**
* writeEEPROMByte allows to write a byte(uint8_t) to the internal eeprom
* @param   address  starts at 0, the max size depends on the uc type
* @param   data     byte (uint8_t)
* @return  status   internal HAL_Status
*/
HAL_StatusTypeDef writeEEPROMByte(uint32_t address, uint32_t data);

/**
* readEEPROMByte reads a byte from an internal eeprom
* @param   address  of the eeprom byte
* @return  data     as a byte (uint8_t)
*/
uint32_t readEEPROMByte(uint32_t address);

void write_eeprom(uint32_t value, uint32_t address);

uint32_t read_eeprom(uint32_t address);
