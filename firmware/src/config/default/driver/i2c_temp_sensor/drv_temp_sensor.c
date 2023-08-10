//driver_xxx_definitions.h : contains data structures and definitions that will be needed by initialization.c file
//This file along with the driver_xxx.h file gets included in the definitions.h file which in turn is included in the initialization.c file
//driver_xxx_local.h : contains data structures and definitions that are local to the driver
//-->includes driver_xxx_definitions file as the driver_xxx_local may use some of the definitions defined in this file
//driver_xxx.h : contains data structues, definitions and function prototypes. This file will be included by application using this driver.
//-->includes driver_xxx_definitions file as the driver_xxx_local may use some of the definitions defined in this file
//driver_xxx.c : contains driver implementation and public APIs.
//-->includes driver_xxx.h file

//------------------------------------------------------------------------------------------------

#include "drv_temp_sensor.h"
#include "driver/i2c/drv_i2c.h"
#include <string.h>

#define APP_TEMP_TEMPERATURE_REG_ADDR               0x00

DRV_TEMP_SENSOR_OBJ gDrvTempSensorObj[DRV_TEMP_SENSOR_INSTANCES_NUMBER];

static void DRV_TEMP_SENSOR_DRVEventHandler ( DRV_I2C_TRANSFER_EVENT event, DRV_I2C_TRANSFER_HANDLE transferHandle, uintptr_t context )
{
    DRV_TEMP_SENSOR_CLIENT_OBJ *clientObj = (DRV_TEMP_SENSOR_CLIENT_OBJ *)context;
    DRV_TEMP_SENSOR_OBJ *dObj = &gDrvTempSensorObj[clientObj->drvIndex];
    DRV_I2C_TRANSFER_HANDLE transHandle;

    if (clientObj->wrInProgress == true && event == DRV_I2C_TRANSFER_EVENT_COMPLETE) {
        clientObj->wrInProgress = false;

        // Initiate checking of EEPROM's write busy status
        clientObj->wrBuffer[0] = 0xFF;

        dObj->drvInterface->write (clientObj->i2cDrvHandle, clientObj->configParams.eepromAddr, (void *)clientObj->wrBuffer, 1, &transHandle);

        if (transHandle != DRV_I2C_TRANSFER_HANDLE_INVALID) {
            clientObj->eepChkBusyStatus = true;
        }
    } else if (clientObj->eepChkBusyStatus == true) {
        if (event == DRV_I2C_TRANSFER_EVENT_ERROR) {
            // EEPROM is still busy with the internal write. Keep checking for the EEPROM write busy status.
            clientObj->wrBuffer[0] = 0xFF;

            dObj->drvInterface->write (clientObj->i2cDrvHandle, clientObj->configParams.eepromAddr, (void *)clientObj->wrBuffer, 1, &transHandle);

            if (transHandle == DRV_I2C_TRANSFER_HANDLE_INVALID) {
                clientObj->eepChkBusyStatus = false;
            }
        } else {
            // EEPROM internal write is completed.
            clientObj->eepChkBusyStatus = false;
        }
    }

    if (clientObj->wrInProgress == false && clientObj->eepChkBusyStatus == false) {
        if (event != DRV_I2C_TRANSFER_EVENT_COMPLETE) {
            clientObj->event = DRV_TEMP_SENSOR_EVENT_ERROR;
        }

        clientObj->isBusy = false;

        if (clientObj->callback != NULL) {
            clientObj->callback (clientObj->event, clientObj->context);
        }
    }
}

static DRV_TEMP_SENSOR_CLIENT_OBJ *DRV_TEMP_SENSOR_ClientObjGet (const DRV_HANDLE handle)
{
    uint32_t drvIndex = handle >> 8;
    uint32_t clientIndex = handle & 0xFF;

    if (drvIndex < DRV_TEMP_SENSOR_INSTANCES_NUMBER) {
        if (clientIndex < gDrvTempSensorObj[drvIndex].maxClients) {
            if (gDrvTempSensorObj[drvIndex].clientObj[clientIndex].inUse == true) {
                return &gDrvTempSensorObj[drvIndex].clientObj[clientIndex];
            }
        }
    }

    return NULL;
}

static int32_t DRV_TEMP_SENSOR_FreeClientGet (DRV_TEMP_SENSOR_OBJ *dObj)
{
    for (int32_t i = 0; i < dObj->maxClients; i++) {
        if (dObj->clientObj[i].inUse == false) {
            dObj->clientObj[i].inUse = true;
            return i;
        }
    }

    return -1;
}

SYS_MODULE_OBJ DRV_TEMP_SENSOR_Initialize (
    const SYS_MODULE_INDEX drvIndex,
    const SYS_MODULE_INIT *const init
)
{
    DRV_TEMP_SENSOR_OBJ *dObj     = NULL;
    DRV_TEMP_SENSOR_INIT *tempSensorInit = (DRV_TEMP_SENSOR_INIT *)init;

    /* Validate the request */
    if (drvIndex >= DRV_TEMP_SENSOR_INSTANCES_NUMBER) {
        return SYS_MODULE_OBJ_INVALID;
    } else {
        dObj = &gDrvTempSensorObj[drvIndex];
    }

    /* Is the driver instance already initialized? */
    if (dObj->isInitialized == true) {
        return SYS_MODULE_OBJ_INVALID;
    }

    dObj->nClients = 0;
    dObj->drvInterface = tempSensorInit->drvInterface;
    dObj->clientObj = (DRV_TEMP_SENSOR_CLIENT_OBJ *)tempSensorInit->clientObj;
    dObj->maxClients = tempSensorInit->maxClients;
    dObj->i2cDrvIndex = tempSensorInit->i2cDrvIndex;
    dObj->isInitialized = true;

    return (SYS_MODULE_OBJ)drvIndex;
}

DRV_HANDLE DRV_TEMP_SENSOR_Open (
    const SYS_MODULE_INDEX drvIndex,
    const DRV_IO_INTENT ioIntent
)
{
    int32_t clientObjIndex;
    DRV_TEMP_SENSOR_OBJ *dObj     = NULL;
    DRV_TEMP_SENSOR_CLIENT_OBJ *clientObj = NULL;

    /* Validate the request */
    if (drvIndex >= DRV_TEMP_SENSOR_INSTANCES_NUMBER) {
        return DRV_HANDLE_INVALID;
    }

    dObj = &gDrvTempSensorObj[drvIndex];

    /* Driver must be initialized before it can be opened by a client. Also, it must not already be in use (opened state). */
    if (dObj->isInitialized == false || dObj->nClients >= dObj->maxClients) {
        return DRV_HANDLE_INVALID;
    } else {
        clientObjIndex = DRV_TEMP_SENSOR_FreeClientGet (dObj);

        if (clientObjIndex >= 0) {
            clientObj = &dObj->clientObj[clientObjIndex];
            clientObj->i2cDrvHandle = dObj->drvInterface->open (dObj->i2cDrvIndex, 0);
            dObj->drvInterface->callbackRegister (clientObj->i2cDrvHandle, DRV_TEMP_SENSOR_DRVEventHandler, (uintptr_t)clientObj);
            clientObj->drvIndex = drvIndex;
            dObj->nClients += 1;

            return ((uint32_t)drvIndex << 8) | (clientObjIndex);
        }
    }

    return DRV_HANDLE_INVALID;
}

void DRV_TEMP_SENSOR_Close (const DRV_HANDLE handle)
{
    DRV_TEMP_SENSOR_CLIENT_OBJ *clientObj = DRV_TEMP_SENSOR_ClientObjGet (handle);
    DRV_TEMP_SENSOR_OBJ *dObj = NULL;

    if (clientObj != NULL) {
        dObj = &gDrvTempSensorObj[clientObj->drvIndex];
        //TODO: Call I2C driver's close API
        dObj->drvInterface->close (dObj->i2cDrvIndex);
        dObj->nClients--;
        clientObj->inUse = false;
    }
}

void DRV_TEMP_SENSOR_TransferEventHandlerSet (
    const DRV_HANDLE handle,
    const DRV_TEMP_SENSOR_APP_CALLBACK callback,
    const uintptr_t context
)
{
    DRV_TEMP_SENSOR_CLIENT_OBJ *clientObj = DRV_TEMP_SENSOR_ClientObjGet (handle);

    if (clientObj != NULL) {
        clientObj->callback = callback;
        clientObj->context = context;
    }
}

uint8_t DRV_TEMP_SENSOR_TemperatureGet (const DRV_HANDLE handle, uint16_t *temperatureData)
{
    int16_t temp;
    (void)handle;

    // Convert the temperature value read from sensor to readable format (Degree Celsius)
    // For demonstration purpose, temperature value is assumed to be positive.
    // The maximum positive temperature measured by sensor is +125 C
    temp = (*temperatureData << 8) | (*temperatureData >> 8);
    temp = (temp >> 7) * 0.5;
    return (uint8_t)temp;
}

bool DRV_TEMP_SENSOR_TemperatureRead (const DRV_HANDLE handle, uint16_t *temperatureData)
{
    DRV_TEMP_SENSOR_CLIENT_OBJ *clientObj = DRV_TEMP_SENSOR_ClientObjGet (handle);
    DRV_TEMP_SENSOR_OBJ *dObj     = NULL;
    DRV_I2C_TRANSFER_HANDLE transferHandle;

    if (clientObj == NULL) {
        return false;
    }

    if (clientObj->isBusy == true) {
        return false;
    }

    clientObj->isBusy = true;
    dObj = &gDrvTempSensorObj[clientObj->drvIndex];
    clientObj->wrBuffer[0] = APP_TEMP_TEMPERATURE_REG_ADDR;
    clientObj->event = DRV_TEMP_SENSOR_EVENT_TEMP_READ_DONE;
    dObj->drvInterface->writeRead (clientObj->i2cDrvHandle, clientObj->configParams.tempSensorAddr, (void *)clientObj->wrBuffer, 1, (void *)temperatureData, 2, &transferHandle);

    if (transferHandle != DRV_I2C_TRANSFER_HANDLE_INVALID) {
        return true;
    } else {
        clientObj->isBusy = false;
        return false;
    }
}

bool DRV_TEMP_SENSOR_TransferSetup (const DRV_HANDLE handle, DRV_TEMP_SENSOR_CONFIG_PARAMS *configParams)
{
    DRV_TEMP_SENSOR_CLIENT_OBJ *clientObj = DRV_TEMP_SENSOR_ClientObjGet (handle);
    DRV_TEMP_SENSOR_OBJ *dObj     = NULL;

    if (clientObj == NULL) {
        return false;
    }

    dObj = &gDrvTempSensorObj[clientObj->drvIndex];
    clientObj->configParams = *configParams;
    dObj->drvInterface->transferSetup (clientObj->i2cDrvHandle, (DRV_I2C_TRANSFER_SETUP *)&clientObj->configParams.transferParams);

    return true;
}
