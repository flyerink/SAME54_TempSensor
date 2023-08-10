
#ifndef DRV_TEMP_SENSOR_H
#define DRV_TEMP_SENSOR_H

#include "drv_temp_sensor_definitions.h"
#include "drv_temp_sensor_local.h"

//Public API interface, function declarations
SYS_MODULE_OBJ DRV_TEMP_SENSOR_Initialize( const SYS_MODULE_INDEX drvIndex, const SYS_MODULE_INIT* const init);

DRV_HANDLE DRV_TEMP_SENSOR_Open(const SYS_MODULE_INDEX drvIndex, const DRV_IO_INTENT ioIntent);

void DRV_TEMP_SENSOR_Close(const DRV_HANDLE handle);

void DRV_TEMP_SENSOR_TransferEventHandlerSet( const DRV_HANDLE handle, const DRV_TEMP_SENSOR_APP_CALLBACK callback, const uintptr_t context);

uint8_t DRV_TEMP_SENSOR_TemperatureGet(const DRV_HANDLE handle, uint16_t* temperatureData);

bool DRV_TEMP_SENSOR_TemperatureRead(const DRV_HANDLE handle, uint16_t* temperatureData);

bool DRV_TEMP_SENSOR_EEPROMWrite(const DRV_HANDLE handle, uint8_t memAddr, uint8_t* wrBuffer, uint8_t nWrBytes);

bool DRV_TEMP_SENSOR_EEPROMRead(const DRV_HANDLE handle, uint8_t memAddr, uint8_t* rdBuffer, uint8_t nRdBytes);

bool DRV_TEMP_SENSOR_TransferSetup(const DRV_HANDLE handle, DRV_TEMP_SENSOR_CONFIG_PARAMS* configParams);

#endif      //DRV_TEMP_SENSOR_H

