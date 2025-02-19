/*******************************************************************************
* Copyright (C) 2022 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

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
#include <string.h>
#include "app.h"
#include "definitions.h"
#include "app_ble.h"
#include "app_ble/app_ble_handler.h"
#include "app_trps.h"
#include "app_adv.h"
#include "app_led.h"
#include "system/console/sys_console.h"

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

// *****************************************************************************
// *****************************************************************************
// Section: Application Callback Functions
// *****************************************************************************
// *****************************************************************************

/* TODO:  Add any necessary callback functions.
 */

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

void APP_Initialize(void) {
    /* Place the App state machine in its initial state. */
    appData.state = APP_STATE_INIT;


    appData.appQueue = xQueueCreate(96, sizeof (APP_Msg_T));
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
static void APP_ConfigBleBuiltInSrv(void) {
    GATTS_GattServiceOptions_T gattServiceOptions;

    /* GATT Service option */
    gattServiceOptions.enable = 1;
    GATTS_ConfigureBuildInService(&gattServiceOptions);
}

void APP_Tasks(void) {
    uint8_t instance;
    APP_TRP_ConnList_T *p_connList;

    APP_Msg_T appMsg[1];
    APP_Msg_T *p_appMsg;
    p_appMsg = appMsg;

    /* Check the application's current state. */
    switch (appData.state) {
            /* Application's initial state. */
        case APP_STATE_INIT:
        {
            bool appInitialized = true;
            //appData.appQueue = xQueueCreate( 10, sizeof(APP_Msg_T) );
            APP_BleStackInit();
            RTC_Timer32Start();

            /* Configure build-in GATT Services*/
            APP_ConfigBleBuiltInSrv();

            APP_LED_Init();

            APP_UpdateLocalName(0, NULL);
            APP_InitConnList();
            APP_ADV_Init();

            APP_TRP_COMMON_Init();
            APP_TRPS_Init();

            if (appInitialized) {

                appData.state = APP_STATE_SERVICE_TASKS;
                SYS_CONSOLE_MESSAGE("BLE_Throughput APP Initialized.\r\n");
            }
            break;
        }

        case APP_STATE_SERVICE_TASKS:
        {
            if (OSAL_QUEUE_Receive(&appData.appQueue, &appMsg, OSAL_WAIT_FOREVER)) {
                switch (p_appMsg->msgId) {
                    case APP_MSG_BLE_STACK_EVT:
                        // Pass BLE Stack Event Message to User Application for handling
                        APP_BleStackEvtHandler((STACK_Event_T *) p_appMsg->msgData);
                        break;

                    case APP_MSG_PROTOCOL_RSP:
                    {
                        p_connList = ((APP_TIMER_TmrElem_T *) p_appMsg->msgData)->p_tmrParam;

                        if ((p_connList != NULL) && (p_connList->trpRole == APP_TRP_SERVER_ROLE))
                            APP_TRP_COMMON_SendCheckSumCommand(p_connList);
                    }
                        break;

                    case APP_MSG_FETCH_TRP_RX_DATA:
                    {
                        p_connList = ((APP_TIMER_TmrElem_T *) p_appMsg->msgData)->p_tmrParam;
                        if ((p_connList != NULL) && (p_connList->trpRole == APP_TRP_SERVER_ROLE))

                            APP_TRPS_LeRxProc(p_connList);

                    }
                        break;

                    case APP_MSG_LED_TIMEOUT:
                    {
                        APP_LED_Elem_T *p_ledElem;

                        instance = ((APP_TIMER_TmrElem_T *) p_appMsg->msgData)->instance;
                        p_ledElem = (APP_LED_Elem_T *) ((APP_TIMER_TmrElem_T *) p_appMsg->msgData)->p_tmrParam;
                        APP_LED_StateMachine(instance, p_ledElem);
                    }
                        break;
                }
            }
            break;
        }

            /* TODO: implement your application state machine.*/


            /* The default state should never be executed. */
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


/*******************************************************************************
 End of File
 */
