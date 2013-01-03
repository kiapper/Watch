#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "DebugUart.h"
#include "Messages.h"
#include "MessageQueues.h"
#include "OneSecondTimers.h"
#include "LcdBuffer.h"
#include "LcdTask.h"
#include "LcdDisplay.h"
#include "BufferPool.h"
#include "Display.h"
#include "IdlePage.h"
#include "IdlePageMain.h"
#include "hal_lcd.h"

#define DISPLAY_TASK_QUEUE_LENGTH   8
#define DISPLAY_TASK_STACK_DEPTH	(configMINIMAL_STACK_SIZE + 90)
#define DISPLAY_TASK_PRIORITY       (tskIDLE_PRIORITY + 1)

extern const unsigned char pMetaWatchSplash[NUM_LCD_ROWS*NUM_LCD_COL_BYTES];

xTaskHandle DisplayHandle;

static void DisplayQueueMessageHandler(tHostMsg* pMsg);
void SendMyBufferToLcd(unsigned char TotalRows);

static tHostMsg* pDisplayMsg;
static tTimerId IdleModeTimerId;
static tTimerId ApplicationModeTimerId;
static tTimerId NotificationModeTimerId;

/* the internal buffer */
#define STARTING_ROW                  ( 0 )
#define WATCH_DRAWN_IDLE_BUFFER_ROWS  ( 30 )
#define PHONE_IDLE_BUFFER_ROWS        ( 66 )

tLcdLine pMyBuffer[NUM_LCD_ROWS];

static unsigned char nvIdleBufferConfig;
static unsigned char nvIdleBufferInvert;

static void InitialiazeIdleBufferConfig(void);
static void InitializeIdleBufferInvert(void);

void SaveIdleBufferInvert(void);

static unsigned char AllowConnectionStateChangeToUpdateScreen;

/*! Display the startup image or Splash Screen */
static void DisplayStartupScreen(void)
{
    CopyRowsIntoMyBuffer(pMetaWatchSplash, STARTING_ROW, NUM_LCD_ROWS);
//    PrepareMyBufferForLcd(STARTING_ROW, NUM_LCD_ROWS);
    SendMyBufferToLcd(NUM_LCD_ROWS);
}

/*! Allocate ids and setup timers for the display modes */
static void AllocateDisplayTimers(void)
{
  IdleModeTimerId = AllocateOneSecondTimer();

  ApplicationModeTimerId = AllocateOneSecondTimer();

  NotificationModeTimerId = AllocateOneSecondTimer();

}

static void SetupSplashScreenTimeout(void)
{
  SetupOneSecondTimer(IdleModeTimerId,
                      ONE_SECOND*3,
                      NO_REPEAT,
                      SplashTimeoutMsg,
                      NO_MSG_OPTIONS);

  StartOneSecondTimer(IdleModeTimerId);

  AllowConnectionStateChangeToUpdateScreen = 0;
}

void StopAllDisplayTimers(void)
{
  StopOneSecondTimer(IdleModeTimerId);
  StopOneSecondTimer(ApplicationModeTimerId);
  StopOneSecondTimer(NotificationModeTimerId);

}

/*! LCD Task Main Loop
 *
 * \param pvParameters
 *
 */
static void DisplayTask(void *pvParameters)
{
    if ( QueueHandles[DISPLAY_QINDEX] == 0 )
    {
        PrintString("Display Queue not created!\r\n");
    }

    DisplayStartupScreen();

    InitialiazeIdleBufferConfig();
    InitializeIdleBufferInvert();
    InitializeDisplaySeconds();
    InitializeTimeFormat();
    InitializeDateFormat();
    AllocateDisplayTimers();

    InitIdlePage(IdleModeTimerId, pMyBuffer);

    SetupSplashScreenTimeout();

    while (1) {
        if ( pdTRUE == xQueueReceive(QueueHandles[DISPLAY_QINDEX],
                                &pDisplayMsg, portMAX_DELAY) ) {

            DisplayQueueMessageHandler(pDisplayMsg);

            BPL_FreeMessageBuffer(&pDisplayMsg);
        }
    }
}

/*! Handle the messages routed to the display queue */
static void DisplayQueueMessageHandler(tHostMsg* pMsg)
{
    unsigned char Type = pMsg->Type;
    switch (Type) {
    case IdleUpdate:
        IdlePageHandler(&IdlePageMain);
        break;

    case SplashTimeoutMsg:
        AllowConnectionStateChangeToUpdateScreen = 1;
	    IdlePageHandler(&IdlePageMain);
        break;

    default:
        PrintStringAndHex("<<Unhandled Message>> in Lcd Display Task: Type 0x", Type);
        break;
    }
}

/******************************************************************************/

/*! Initialize the LCD display task
 *
 * Initializes the display driver, clears the display buffer and starts the
 * display task
 *
 * \return none, result is to start the display task
 */
void InitializeDisplayTask(void)
{
    QueueHandles[DISPLAY_QINDEX] =
        xQueueCreate( DISPLAY_TASK_QUEUE_LENGTH, MESSAGE_QUEUE_ITEM_SIZE  );

    // task function, task name, stack len , task params, priority, task handle
    xTaskCreate(DisplayTask,
              "DISPLAY",
              DISPLAY_TASK_STACK_DEPTH,
              NULL,
              DISPLAY_TASK_PRIORITY,
              &DisplayHandle);
}

unsigned char GetIdleBufferConfiguration(void)
{
  return nvIdleBufferConfig;
}

void SendMyBufferToLcd(unsigned char TotalRows)
{
    tHostMsg* pOutgoingMsg;

    BPL_AllocMessageBuffer(&pOutgoingMsg);
    ((tUpdateMyDisplayMsg*)pOutgoingMsg)->Type = UpdateMyDisplayLcd;
    ((tUpdateMyDisplayMsg*)pOutgoingMsg)->TotalLines = TotalRows;
    ((tUpdateMyDisplayMsg*)pOutgoingMsg)->pMyDisplay = (unsigned char*)pMyBuffer;
    RouteMsg(&pOutgoingMsg);
}

static void InitialiazeIdleBufferConfig(void)
{
    nvIdleBufferConfig = WATCH_CONTROLS_TOP;
}

static void InitializeIdleBufferInvert(void)
{
    nvIdleBufferInvert = 0;
}

void ToggleIdleBufferInvert(void)
{
	if ( nvIdleBufferInvert == 1 )
	{
		nvIdleBufferInvert = 0;
	}
	else
	{
		nvIdleBufferInvert = 1;
	}
}

unsigned char QueryInvertDisplay(void)
{
  return nvIdleBufferInvert;
}

void SaveIdleBufferInvert(void)
{
}
