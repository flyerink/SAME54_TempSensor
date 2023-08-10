/*******************************************************************************
  MPLAB Harmony Application Source File

  Company:
    Microchip Technology Inc.

  File Name:
    app.c

  Summary:
    This file contains the source code for the MPLAB Harmony application.

  Description:
    This file contains the source code for the MPLAB Harmony application.  It
    implements the logic of the application's state machine and it may call
    API routines of other MPLAB Harmony modules in the system, such as drivers,
    system services, and middleware.  However, it does not call any of the
    system interfaces (such as the "Initialize" and "Tasks" functions) of any of
    the modules in the system or make any assumptions about when those functions
    are called.  That is the responsibility of the configuration-specific system
    files.
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include "definitions.h"
#include "app.h"

// *****************************************************************************
// *****************************************************************************
// Section: Global Data Definitions
// *****************************************************************************
// *****************************************************************************

// *****************************************************************************
/* Application Data

  Summary:
    Holds application data

  Description:
    This structure holds the application's data.

  Remarks:
    This structure should be initialized by the APP_Initialize function.

    Application strings and buffers are be defined outside this structure.
*/

APP_DATA appData;
static SYS_TIME_HANDLE timer = SYS_TIME_HANDLE_INVALID;

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

static void APP_INST0_CLIENT1_Callback (DRV_TEMP_SENSOR_EVENT event, uintptr_t context)
{
    APP_DATA *pTempSensorData = (APP_DATA *)context;

    switch (event) {
        case DRV_TEMP_SENSOR_EVENT_TEMP_READ_DONE:
            pTempSensorData->isTempReadDone = true;
            break;
        case DRV_TEMP_SENSOR_EVENT_EEPROM_WRITE_DONE:
            pTempSensorData->isEEPROMWriteDone = true;
            break;
        case DRV_TEMP_SENSOR_EVENT_EEPROM_READ_DONE:
            pTempSensorData->isEEPROMReadDone = true;
            break;
        case DRV_TEMP_SENSOR_EVENT_ERROR:
            pTempSensorData->tempSensorError = true;
            break;
    }
}


// *****************************************************************************
// *****************************************************************************
// Section: Application Local Functions
// *****************************************************************************
// *****************************************************************************


/* TODO:  Add any necessary local functions.
*/


// *****************************************************************************
// *****************************************************************************
// Section: Application Initialization and State Machine Functions
// *****************************************************************************
// *****************************************************************************

/*******************************************************************************
  Function:
    void APP_Initialize ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Initialize ( void )
{
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;

    /* TODO: Initialize your application's state machine and other
     * parameters.
     */
}


/******************************************************************************
  Function:
    void APP_Tasks ( void )

  Remarks:
    See prototype in app.h.
 */

void APP_Tasks ( void )
{
    volatile APP_DATA *pTempSensorData = &appData;
    static DRV_TEMP_SENSOR_CHN temp_channel = DRV_TEMP_SENSOR_CHN_INT;

    /* Check the application's current state. */
    switch ( appData.state ) {
        /* Application's initial state. */
        case APP_STATE_INIT: {
            /* Client-1 opens driver instance 0, and gets a handle to it */
            pTempSensorData->drvHandle = DRV_TEMP_SENSOR_Open (DRV_TEMP_SENSOR_INDEX_0, 0);
            if (pTempSensorData->drvHandle == DRV_HANDLE_INVALID) {
                printf ("Invalid handle\r\n");
                break;
            }

            /* Client-1 registers the event handler */
            DRV_TEMP_SENSOR_TransferEventHandlerSet (pTempSensorData->drvHandle, APP_INST0_CLIENT1_Callback, (uintptr_t)pTempSensorData);

            DRV_TEMP_SENSOR_CONFIG_PARAMS configParams;
            configParams.eepromAddr = 0x57;
            configParams.tempSensorAddr = 0x4C;
            configParams.transferParams.clockSpeed = 400000;

            /* Client-1 registers the transfer parameters (in other words, client specific data) */
            DRV_TEMP_SENSOR_TransferSetup (pTempSensorData->drvHandle, &configParams);
            appData.state = APP_STATE_READ_SENSOR;
            break;
        }

        case APP_STATE_READ_SENSOR: {
            pTempSensorData->isTempReadDone = false;
            pTempSensorData->tempSensorError = false;
            if (DRV_TEMP_SENSOR_TemperatureRead (pTempSensorData->drvHandle, (uint16_t *)&pTempSensorData->rawTemperatureData, temp_channel) == true) {
                appData.state = APP_STATE_READ_SENSOR_WAIT;
            } else {
                printf ("Channel %d: Temperature read error!\r\n", temp_channel);
                SYS_TIME_DelayMS (1000, &timer);
                appData.state = APP_STATE_IDLE;
            }
            break;
        }

        case APP_STATE_READ_SENSOR_WAIT: {
            if (pTempSensorData->tempSensorError == true) {
                printf ("Channel %d: Temperature read error!\r\n", temp_channel);
                SYS_TIME_DelayMS (1000, &timer);
                appData.state = APP_STATE_IDLE;
            } else if (pTempSensorData->isTempReadDone == true) {
                pTempSensorData->temperature[temp_channel] = DRV_TEMP_SENSOR_TemperatureGet (pTempSensorData->drvHandle, (uint16_t *)&pTempSensorData->rawTemperatureData);
                printf ("Channel %d: Temperature = %dC\r\n", temp_channel, pTempSensorData->temperature[temp_channel]);
                if (++ temp_channel >= DRV_TEMP_SENSOR_CHN_MAX) {
                    temp_channel = DRV_TEMP_SENSOR_CHN_INT;
                    printf("\r\n");
                    SYS_TIME_DelayMS (1000, &timer);
                    appData.state = APP_STATE_IDLE;
                } else
                    appData.state = APP_STATE_READ_SENSOR;
            }
            break;
        }

        case APP_STATE_IDLE: {
            if (SYS_TIME_DelayIsComplete (timer)) {
                appData.state = APP_STATE_READ_SENSOR;
            }
            break;
        }
    }
}


/*******************************************************************************
 End of File
 */
