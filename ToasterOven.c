// **** Include libraries here ****
// Standard libraries
#include <stdio.h>
#include <string.h>
// CSE13E Support Library
#include "BOARD.h"

// Lab specific libraries
#include "Leds.h"
#include "Adc.h"
#include "Ascii.h"
#include "Buttons.h"
#include "Oled.h"
#include "OledDriver.h"

// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>

// **** Set any macros or preprocessor directives here ****
// Set a macro for resetting the timer, makes the code a little clearer.
#define TIMER_2HZ_RESET() (TMR1 = 0)

// **** Set any local typedefs here ****
typedef enum
{
    SETUP,
    SELECTOR_CHANGE_PENDING,
    COOKING,
    RESET_PENDING,
    EXTRA_CREDIT
} OvenState;

typedef struct
{
    OvenState state;
    // Add more members to this struct
    uint16_t initialCookTime; // Cooking time
    uint16_t cookingTimeLeft; // Cooking time
    uint16_t temperature;     // Temp setting
    uint16_t buttonPressTime; // Button press time
    uint8_t mode;             // Cooking mode
} OvenData;

// Enum for the different cooking modes of the toaster
typedef enum
{
    BAKE,
    TOAST,
    BROIL
} CookingStates;

// Variables declared below
// Consist of various flags and counters and variables for storing/keeping track of values
static OvenData ovenData;
static char currentLEDS;
static uint8_t buttonEvent = BUTTON_EVENT_NONE;
static uint8_t inverted, adcChange, shiftTemperature = FALSE;
static uint16_t TIMER_TICK = FALSE;
static uint16_t freeTime, timerFree, timeCounter = 0;
static uint16_t adcValue, LEDSInterval, remainder, tempHolder;

// **** Define any module-level, global, or external variables here ****
// Constants defined below
#define LONG_PRESS 5
#define MINUTE(x) x / 60
#define SECONDS(x) x % 60
#define TEMP_ADJUST(x) x + 300
#define DEFAULT_TEMP 350
#define BROIL_TEMP 500
#define ALL_LEDS_ON 0xFF
#define CLEAR_LEDS 0x00
#define BITMASK_CHAR 0x03FC

// **** Put any helper functions here ****

/*This function will update your OLED to reflect the state .*/
void updateOvenOLED(OvenData ovenData)
{
    // update OLED here
    char OvenPrint[60] = "";
    char topOn[6], topOff[6], botOn[6], botOff[6];
    sprintf(topOn, "%s%s%s%s%s", OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON);
    sprintf(topOff, "%s%s%s%s%s", OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF,
            OVEN_TOP_OFF);
    sprintf(botOn, "%s%s%s%s%s", OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON,
            OVEN_BOTTOM_ON);
    sprintf(botOff, "%s%s%s%s%s", OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF,
            OVEN_BOTTOM_OFF);

    // Switch determines what exactly we need to print out
    switch (ovenData.mode)
    {
    case BAKE:
        if (!(ovenData.state == COOKING || ovenData.state == RESET_PENDING))
        {
            if (!shiftTemperature)
            {
                sprintf(OvenPrint, "|%s| Mode: Bake\n|     | >Time: %d:%02d\n|-----|  Temp: %d%sF"
                                   "\n|%s|",
                        topOff, MINUTE(ovenData.initialCookTime), SECONDS(ovenData.initialCookTime),
                        ovenData.temperature, DEGREE_SYMBOL, botOff);
            }
            else
            {
                sprintf(OvenPrint, "|%s| Mode: Bake\n|     |  Time: %d:%02d\n|-----| >Temp: %d%sF"
                                   "\n|%s|",
                        topOff, MINUTE(ovenData.initialCookTime), SECONDS(ovenData.initialCookTime),
                        ovenData.temperature, DEGREE_SYMBOL, botOff);
            }
        }
        else
        {
            sprintf(OvenPrint, "|%s| Mode: Bake\n|     |  Time: %d:%02d\n|-----|  Temp: %d%sF"
                               "\n|%s|",
                    topOn, MINUTE(ovenData.initialCookTime), SECONDS(ovenData.cookingTimeLeft),
                    ovenData.temperature, DEGREE_SYMBOL, botOn);
        }
        break;
    case TOAST:
        if (!(ovenData.state == COOKING || ovenData.state == RESET_PENDING))
        {
            sprintf(OvenPrint, "|%s| Mode: Toast\n|     |  Time: %d:%02d\n|-----|"
                               "\n|%s|",
                    topOff, MINUTE(ovenData.initialCookTime), SECONDS(ovenData.initialCookTime),
                    botOff);
        }
        else
        {
            sprintf(OvenPrint, "|%s| Mode: Toast\n|     |  Time: %d:%02d\n|-----|"
                               "\n|%s|",
                    topOff, MINUTE(ovenData.initialCookTime), SECONDS(ovenData.cookingTimeLeft),
                    botOn);
        }
        break;
    case BROIL:
        if (!(ovenData.state == COOKING || ovenData.state == RESET_PENDING))
        {
            sprintf(OvenPrint, "|%s| Mode: Broil\n|     |  Time: %d:%02d\n|-----|  Temp: 500%sF"
                               "\n|%s|",
                    topOff, MINUTE(ovenData.initialCookTime), SECONDS(ovenData.initialCookTime),
                    DEGREE_SYMBOL, botOff);
        }
        else
        {
            sprintf(OvenPrint, "|%s| Mode: Broil\n|     |  Time: %d:%02d\n|-----|  Temp: 500%sF"
                               "\n|%s|",
                    topOn, MINUTE(ovenData.initialCookTime), SECONDS(ovenData.cookingTimeLeft),
                    DEGREE_SYMBOL, botOff);
        }
        break;
    }

    // Clears oled in order to reset it
    OledClear(OLED_COLOR_BLACK);
    OledDrawString(OvenPrint);

    if (ovenData.state == EXTRA_CREDIT)
    {
        (inverted) ? OledSetDisplayNormal() : OledSetDisplayInverted();
    }
    // update oled to make sure changes show
    OledUpdate();
}

/*This function will execute your state machine.
 * It should ONLY run if an event flag has been set.*/
void runOvenSM(void)
{
    switch (ovenData.state)
    {
    case SETUP:
        if (adcChange)
        {
            adcValue = AdcRead();
            // Shifts 2 bits because there are six zeros at the end ands with the mask 0000001111111100 to get the top 8 bits
            // Shift the integer two bits to get rid of extra zeros at the end in order to get the isolated top 8 bits
            adcValue = (adcValue & BITMASK_CHAR) >> 2;

            // Editing the temperature, scale the adcValue by adding 300 to get within the range of desired temperatures
            // Otherwise, add 1 to get within the range of cooking times
            if (ovenData.mode == BAKE && shiftTemperature)
            {
                ovenData.temperature = TEMP_ADJUST(adcValue);
            }

            else
            {
                ovenData.initialCookTime = adcValue + 1;
                ovenData.cookingTimeLeft = ovenData.initialCookTime;
            }

            // Update the oled
            updateOvenOLED(ovenData);
        }

        // If button 3 is pressed, we move to selector change pending
        if (buttonEvent & BUTTON_EVENT_3DOWN)
        {
            ovenData.buttonPressTime = freeTime;
            ovenData.state = SELECTOR_CHANGE_PENDING;
        }

        // Cooking starts if button 4 is pressed
        if (buttonEvent & BUTTON_EVENT_4DOWN)
        {
            timerFree = freeTime;
            ovenData.state = COOKING;
            updateOvenOLED(ovenData);
            LEDS_SET(ALL_LEDS_ON);
            LEDSInterval = (ovenData.initialCookTime * 5) / 8;
            remainder = (ovenData.initialCookTime * 5) % 8;
            timeCounter = 0;
        }
        break;
    case SELECTOR_CHANGE_PENDING:
        // Calculates the time that has passed when a button is pressed
        if (buttonEvent & BUTTON_EVENT_3UP)
        {
            uint16_t elapsedTime = freeTime - ovenData.buttonPressTime;
            if (elapsedTime >= LONG_PRESS)
            {
                if (ovenData.mode == BAKE)
                {
                    if (shiftTemperature)
                    {
                        shiftTemperature = FALSE;
                    }
                    else
                    {
                        shiftTemperature = TRUE;
                    }
                }
                updateOvenOLED(ovenData);
                ovenData.state = SETUP;
            }
            else
            {
                // Otherwise, change between the modes
                (ovenData.mode == BROIL) ? ovenData.mode = BAKE : ovenData.mode++;

                if (ovenData.mode == BROIL)
                {
                    tempHolder = ovenData.temperature;
                    ovenData.temperature = 500;
                }
                else if (ovenData.mode == BAKE)
                {
                    ovenData.temperature = tempHolder;
                }
                updateOvenOLED(ovenData);
                ovenData.state = SETUP;
            }
        }
        break;
    case COOKING:
        if (TIMER_TICK)
        {
            // Keeps track of the remaining time left
            timeCounter++;
            if (remainder > 0 && timeCounter == LEDSInterval + 1)
            {
                currentLEDS = LEDS_GET();
                timeCounter = 0;
                remainder--;
                LEDS_SET(currentLEDS << 1);
            }

            if (remainder == 0 && timeCounter == LEDSInterval)
            {
                currentLEDS = LEDS_GET();
                timeCounter = 0;
                remainder--;
                LEDS_SET(currentLEDS << 1);
            }

            // When the cook time is 0, we go to the finished, flashing extra credit mode
            if (ovenData.cookingTimeLeft == 0)
            {
                ovenData.cookingTimeLeft = ovenData.initialCookTime;
                ovenData.state = EXTRA_CREDIT;
                updateOvenOLED(ovenData);
                LEDS_SET(CLEAR_LEDS);
                break;
            }

            // If the freeTime - the free time that was stored is divisible by 5, time to change oled
            if ((freeTime - timerFree) % 5 == 0)
            {
                ovenData.cookingTimeLeft--;
                updateOvenOLED(ovenData);
            }
        }

        // When button 4 is pressed, user wants to reset
        if (buttonEvent & BUTTON_EVENT_4DOWN)
        {
            ovenData.state = RESET_PENDING;
            ovenData.buttonPressTime = freeTime;
        }
        break;
    case RESET_PENDING:

        // while we determine if a timer is reset, there is a possibility that a LONG_PRESS could pass or a LED could turn off in the time it takes the button to be pressed.
        if (TIMER_TICK)
        {
            timeCounter++;
            if ((remainder > 0 && timeCounter == LEDSInterval + 1) ||
                (remainder == 0 && timeCounter == LEDSInterval))
            {
                currentLEDS = LEDS_GET();
                timeCounter = 0;
                remainder--;
                LEDS_SET(currentLEDS << 1);
            }
            if ((freeTime - timerFree) % 5 == 0)
            {
                if (ovenData.cookingTimeLeft)
                {
                    ovenData.cookingTimeLeft--;
                    updateOvenOLED(ovenData);
                }
            }
            if (freeTime - ovenData.buttonPressTime >= LONG_PRESS)
            {
                ovenData.cookingTimeLeft = ovenData.initialCookTime;
                ovenData.state = SETUP;
                updateOvenOLED(ovenData);
                LEDS_SET(CLEAR_LEDS);
                break;
            }
        }
        if (buttonEvent & BUTTON_EVENT_4UP)
        {
            ovenData.state = COOKING;
        }
        if (freeTime - ovenData.buttonPressTime < LONG_PRESS)
        {
            ovenData.state = COOKING;
        }
        break;
    case EXTRA_CREDIT:

        // extra credit case
        // when cooking is done it routes here, and if the screen looks normal it becomes
        // inverted, otherwise if it is inverted it becomes normal, creating a flashing effect.
        // this is done with a flag
        if (TIMER_TICK)
        {
            if (inverted)
            {
                inverted = FALSE;
            }
            else
            {
                inverted = TRUE;
            }
            updateOvenOLED(ovenData);
        }
        if (buttonEvent & BUTTON_EVENT_4UP)
        {
            inverted = TRUE;
            updateOvenOLED(ovenData);
            ovenData.state = SETUP;
            updateOvenOLED(ovenData);
        }
        break;
    }
}

int main()
{
    BOARD_Init();

    // initalize timers and timer ISRs:
    // <editor-fold defaultstate="collapsed" desc="TIMER SETUP">

    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a .01s timer.

    T2CON = 0;                           // everything should be off
    T2CONbits.TCKPS = 0b100;             // 1:16 prescaler
    PR2 = BOARD_GetPBClock() / 16 / 100; // interrupt at .5s intervals
    T2CONbits.ON = 1;                    // turn the timer on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T2IF = 0; // clear the interrupt flag before configuring
    IPC2bits.T2IP = 4; // priority of  4
    IPC2bits.T2IS = 0; // subpriority of 0 arbitrarily
    IEC0bits.T2IE = 1; // turn the interrupt on

    // Configure Timer 3 using PBCLK as input. We configure it using a 1:256 prescaler, so each timer
    // tick is actually at F_PB / 256 Hz, so setting PR3 to F_PB / 256 / 5 yields a .2s timer.

    T3CON = 0;                          // everything should be off
    T3CONbits.TCKPS = 0b111;            // 1:256 prescaler
    PR3 = BOARD_GetPBClock() / 256 / 5; // interrupt at .5s intervals
    T3CONbits.ON = 1;                   // turn the timer on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T3IF = 0; // clear the interrupt flag before configuring
    IPC3bits.T3IP = 4; // priority of  4
    IPC3bits.T3IS = 0; // subpriority of 0 arbitrarily
    IEC0bits.T3IE = 1; // turn the interrupt on;

    // </editor-fold>

    printf("Welcome to maeacost's Lab07 (Toaster Oven).  Compiled on %s %s.", __TIME__, __DATE__);

    // initialize state machine (and anything else you need to init) here
    ovenData.buttonPressTime = FALSE;
    ovenData.initialCookTime, ovenData.cookingTimeLeft = TRUE;
    ovenData.temperature = DEFAULT_TEMP;
    ovenData.state = SETUP;
    ovenData.mode = BAKE;

    OledInit();
    ButtonsInit();
    AdcInit();
    LEDS_INIT();

    updateOvenOLED(ovenData);
    while (1)
    {
        // Add main loop code here:
        if (buttonEvent != BUTTON_EVENT_NONE || adcChange || TIMER_TICK)
        {
            runOvenSM();
            buttonEvent = BUTTON_EVENT_NONE;
            adcChange, TIMER_TICK = FALSE;
        }
    };
}

/*The 5hz timer is used to update the free-running timer and to generate TIMER_TICK events*/
void __ISR(_TIMER_3_VECTOR, ipl4auto) TimerInterrupt5Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 12;

    // Add event-checking code here
    TIMER_TICK = TRUE;
    freeTime++;
}

/*The 100hz timer is used to check for button and ADC events*/
void __ISR(_TIMER_2_VECTOR, ipl4auto) TimerInterrupt100Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;

    // Add event-checking code here
    adcChange = AdcChanged();
    buttonEvent = ButtonsCheckEvents();
}