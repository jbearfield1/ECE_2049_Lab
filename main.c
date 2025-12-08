#include <msp430.h>
#include "peripherals.h"

// --- Global Variables ---
volatile unsigned long global_time_seconds = 29089811;
volatile unsigned char new_second_event = 0;

// --- Function Prototypes ---
void configure_timer_a2(void);
void displayTime(unsigned long int inTime);
enum DISPLAY_STATE {RUN = 0, EDIT_DAY = 1, EDIT_MONTH = 2, EDIT_HOUR = 3, EDIT_MINUTE = 4, EDIT_SEC = 5};
unsigned int edit_month, edit_day, edit_hour, edit_min, edit_sec;


void initButtons(void) {
    // S1 (P2.1)
    P2DIR &= ~BIT1;
    P2REN |= BIT1;
    P2OUT |= BIT1;

    // S2 (P1.1)
    P1DIR &= ~BIT1;
    P1REN |= BIT1;
    P1OUT |= BIT1;
}

void configure_timer_a2() {
    TA2CTL = TASSEL_1 + MC_1 + ID_0 + TACLR; // ACLK, Up Mode, /1 divider

    // 32768 Hz / 1 Hz = 32768 ticks
    TA2CCR0 = 32767; //32768 - 1 = 32767

    TA2CCTL0 |= CCIE;
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {
    global_time_seconds++;
    new_second_event = 1;
}


void displayTime(unsigned long int inTime) {

    const char *monthNames[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};
    const uint8_t daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    unsigned int sec = inTime % 60;
    unsigned int min = (inTime / 60) % 60;
    unsigned int hr = (inTime / 3600) % 24;

    unsigned long totalDays = inTime / 86400;
    unsigned int currentMonth = 0;
    unsigned int currentDay = 0;
    unsigned int i = 0;
    for (i = 0; i < 12; i++) {
        if (totalDays < daysInMonth[i]) {
            currentMonth = i;
            currentDay = totalDays + 1;
            break;
        } else {
            totalDays -= daysInMonth[i];
        }
    }

    char dateStr[7];
    char timeStr[9];

    //Build date string: MMM DD
    dateStr[0] = monthNames[currentMonth][0];
    dateStr[1] = monthNames[currentMonth][1];
    dateStr[2] = monthNames[currentMonth][2];
    dateStr[3] = ' ';

    //Build day string
    dateStr[4] = (currentDay / 10) + '0'; // Tens digit
    dateStr[5] = (currentDay % 10) + '0'; // Ones digit
    dateStr[6] = '\0';


    // Hours
    timeStr[0] = (hr / 10) + '0';
    timeStr[1] = (hr % 10) + '0';
    timeStr[2] = ':';

    // Minutes
    timeStr[3] = (min / 10) + '0';
    timeStr[4] = (min % 10) + '0';
    timeStr[5] = ':';

    // Seconds
    timeStr[6] = (sec / 10) + '0';
    timeStr[7] = (sec % 10) + '0';
    timeStr[8] = '\0';

    Graphics_drawStringCentered(&g_sContext, (uint8_t *)dateStr, AUTO_STRING_LENGTH, 48, 35, OPAQUE_TEXT);
    Graphics_drawStringCentered(&g_sContext, (uint8_t *)timeStr, AUTO_STRING_LENGTH, 48, 55, OPAQUE_TEXT);

    Graphics_flushBuffer(&g_sContext);
}

void breakDownTime(unsigned long int inTime) {
    unsigned long totalDays = inTime / 86400;
    edit_sec = inTime % 60;
    edit_min = (inTime / 60) % 60;
    edit_hour = (inTime / 3600) % 24;

    edit_month = 0;
    edit_day = 0;

    for (int i = 0; i < 12; i++) {
        if (totalDays < daysInMonth[i]) {
            edit_month = i;
            edit_day = totalDays + 1;
            break;
        } else {
            totalDays -= daysInMonth[i];
        }
    }
}

unsigned long int reconstructSeconds(void) {
    unsigned long int total_seconds = 0;

    for(int i = 0; i < edit_month; i++) {
        total_seconds += daysInMonth[i] * 86400;
    }

    total_seconds += (edit_day - 1) * 86400;
    total_seconds += edit_hour * 3600;
    total_seconds += edit_min * 60;
    total_seconds += edit_sec;

    return total_seconds;
}

void main(void)
{
    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer

    initLeds();
    initButtons();
    configDisplay();
    configure_timer_a2();

    __enable_interrupt();

    Graphics_clearDisplay(&g_sContext);

    enum GAME_STATE RUN;
    while (1)
    {
        s1_clicked = s1Clicked();
        s2_clicked = s2Clicked();
        switch(state) {
        case RUN:
            if (new_second_event) {
                new_second_event = 0;
                displayTime(global_time_seconds);
                }

            if (s1_clicked) {
                breakDownTime(global_time_seconds);
                state = EDIT_MONTH;
                Graphics_clearDisplay(&g_sContext);
                }
            break;
        case EDIT_MONTH:
            //Need scroll wheel logic here
            displayEditScreen();

            if (s1_clicked) {
                state = EDIT_DAY;
            }
            if (s2_clicked) {
                global_time_seconds = reconstructSeconds();
                state = RUN;
                Graphics_clearDisplay(&g_sContext);
            }
            break;
        case EDIT_DAY:
            //Need scroll wheel logic here
            displayEditScreen();

            if (s1_clicked) {
                state = EDIT_HOUR;
            }
            if (s2_clicked) {
                global_time_seconds = reconstructSeconds();
                state = RUN;
                Graphics_clearDisplay(&g_sContext);
            }
            break;
        case EDIT_HOUR:
            //Need scroll wheel logic here
            displayEditScreen();

            if (s1_clicked) {
                state = EDIT_MINUTE;
            }
            if (s2_clicked) {
                global_time_seconds = reconstructSeconds();
                state = RUN;
                Graphics_clearDisplay(&g_sContext);
            }
            break;
        case EDIT_MINUTE:
            //Need scroll wheel logic here
            displayEditScreen();

            if (s1_clicked) {
                state = EDIT_SEC;
            }
            if (s2_clicked) {
                global_time_seconds = reconstructSeconds();
                state = RUN;
                Graphics_clearDisplay(&g_sContext);
            }
            break;
        case EDIT_SEC:
            //Need scroll wheel logic here
            displayEditScreen();

            if (s1_clicked) {
                state = EDIT_MONTH;
            }
            if (s2_clicked) {
                global_time_seconds = reconstructSeconds();
                state = RUN;
                Graphics_clearDisplay(&g_sContext);
            }
            break;
        }
    }
}
