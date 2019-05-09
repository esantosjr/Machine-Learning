#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "utilities.h"

#include "eeprom.h"

/**
 * writeEEPROMByte allows to write a byte(uint8_t) to the internal eeprom
 * @param   address  starts at 0, the max size depends on the uc type
 * @param   data     byte (uint8_t)
 * @return  status   internal HAL_Status
 */
HAL_StatusTypeDef writeEEPROMByte(uint32_t address, uint32_t data) {

    HAL_StatusTypeDef  status;

    address = address*4 + EEPROM_BASE_ADDRESS;
    HAL_FLASHEx_DATAEEPROM_Unlock();  // Unprotect the EEPROM to allow writing
    status = HAL_FLASHEx_DATAEEPROM_Program(FLASH_TYPEPROGRAMDATA_WORD, address, data);
    HAL_FLASHEx_DATAEEPROM_Lock();  // Reprotect the EEPROM

    return status;
}

/**
 * readEEPROMByte reads a byte from an internal eeprom
 * @param   address  of the eeprom byte
 * @return  data     as a byte (uint8_t)
 */
uint32_t readEEPROMByte(uint32_t address) {

    uint32_t data = 0;

    address = address*4 + EEPROM_BASE_ADDRESS;
    data = *(__IO uint32_t*)address;

    return data;
}
