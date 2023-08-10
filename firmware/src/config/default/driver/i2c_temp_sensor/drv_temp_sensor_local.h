
#ifndef DRV_TEMP_SENSOR_LOCAL_H
#define DRV_TEMP_SENSOR_LOCAL_H

#include "drv_temp_sensor_definitions.h"

typedef struct {
    bool inUse;
    bool isBusy;
    bool wrInProgress;
    bool eepChkBusyStatus;
    DRV_TEMP_SENSOR_APP_CALLBACK callback;
    uintptr_t context;
    uint8_t drvIndex;
    DRV_HANDLE i2cDrvHandle;
    uint8_t wrBuffer[17];
    DRV_TEMP_SENSOR_CONFIG_PARAMS configParams;
    DRV_TEMP_SENSOR_EVENT event;
} DRV_TEMP_SENSOR_CLIENT_OBJ;

typedef struct {
    bool isInitialized;
    uint8_t nClients;
    uint8_t maxClients;
    uint8_t i2cDrvIndex;
    const DRV_TEMP_SENSOR_DRV_INTERFACE *drvInterface;
    DRV_TEMP_SENSOR_CLIENT_OBJ *clientObj;
} DRV_TEMP_SENSOR_OBJ;

#endif      //DRV_TEMP_SENSOR_LOCAL_H