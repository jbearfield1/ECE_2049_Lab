/************** ECE2049 DEMO CODE ******************/
/**************  13 March 2019   ******************/
/***************************************************/

#include <msp430.h>

/* Peripherals.c and .h are where the functions that implement
 * the LEDs and keypad, etc are. It is often useful to organize
 * your code by putting like functions together in files.
 * You include the header associated with that file(s)
 * into the main file of your project. */
#include "peripherals.h"
#include <stdlib.h>

// Function Prototypes
#define MAX_SEQ_LEN 32
void swDelay(char numLoops);
void config_buttons(void);
uint8_t get_button_states(void);
int pitchToTicks(int pitch);
int pitchToLED(int pitch);
int playNote(int pitch);


// Declare globals here
enum GAME_STATE {WELCOME = 0, PLAY_SEQ = 1, CHECK_INP = 2, FAIL_ERROR = 3};
int gameSeq[MAX_SEQ_LEN];
int seqLen = 0;
int playerPos = 0;
int i;

int pitches[] = {0, 880, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831};

struct Note {
    int pitch;
    int duration;
};

struct Note song[] = {
                      // --- A SECTION ---
                      {4, 8}, {0, 8},
                      {4, 8}, {0, 8},
                      {11, 12}, {0, 8},
                      {11, 12}, {0, 8},
                      {1, 8}, {0, 8},
                      {1, 8}, {0, 8},
                      {11, 16}, {0, 8},

                      {9, 8}, {0, 8},
                      {9, 8}, {0, 8},
                      {8, 12}, {0, 8},
                      {8, 12}, {0, 8},
                      {6, 8}, {0, 8},
                      {6, 8}, {0, 8},
                      {4, 16}, {0, 8},

                      // --- B SECTION ---
                      {11, 8}, {0, 8},
                      {11, 8}, {0, 8},
                      {9, 12}, {0, 8},
                      {9, 12}, {0, 8},
                      {8, 8}, {0, 8},
                      {8, 8}, {0, 8},
                      {6, 16}, {0, 8},

                      {11, 8}, {0, 8},
                      {11, 8}, {0, 8},
                      {9, 12}, {0, 8},
                      {9, 12}, {0, 8},
                      {8, 8}, {0, 8},
                      {8, 8}, {0, 8},
                      {6, 16}, {0, 8},

                      // --- A SECTION REPEAT ---
                      {4, 8}, {0, 8},
                      {4, 8}, {0, 8},
                      {11, 12}, {0, 8},
                      {11, 12}, {0, 8},
                      {1, 8}, {0, 8},
                      {1, 8}, {0, 8},
                      {11, 16}, {0, 8},

                      {9, 8}, {0, 8},
                      {9, 8}, {0, 8},
                      {8, 12}, {0, 8},
                      {8, 12}, {0, 8},
                      {6, 8}, {0, 8},
                      {6, 8}, {0, 8},
                      {4, 16}, {0, 8}
                  };

//Software Delay to run useless loops
//// Function Prototypes
//#define MAX_SEQ_LEN 32
//void swDelay(char numLoops);
//void initButtons(void) {
//    // S1 (P2.1)
//    P2DIR &= ~BIT1;
//    P2REN |= BIT1;
//    P2OUT |= BIT1;
//
//    // S2 (P1.1)
//    P1DIR &= ~BIT1;
//    P1REN |= BIT1;
//    P1OUT |= BIT1;
//}
//
//// Declare globals here
//enum GAME_STATE {WELCOME = 0, PLAY_SEQ = 1, CHECK_INP = 2, FAIL_ERROR = 3};
//int gameSeq[MAX_SEQ_LEN];
//int seqLen = 0;
//int playerPos = 0;
//int i;
//
////Software Delay to run useless loops
void swDelay(char numLoops) {
    volatile unsigned long i;
    char j;
    for (j = 0; j < numLoops; j++) {

        for (i = 1000; i > 0; i--);
    }
}

void config_buttons(void) {
    // S1 (P7.0)
    P7DIR &= ~BIT0;
    P7REN |= BIT0;
    P7OUT |= BIT0;

    // S2 (P3.6)
    P3DIR &= ~BIT6;
    P3REN |= BIT6;
    P3OUT |= BIT6;

    // S3 (P2.2)
    P2DIR &= ~BIT2;
    P2REN |= BIT2;
    P2OUT |= BIT2;

    // S4 (P7.4)
    P7DIR &= ~BIT4;
    P7REN |= BIT4;
    P7OUT |= BIT4;
}

uint8_t get_button_states(void) {
	uint8_t s1_state = !(P7IN & BIT0);
	uint8_t s2_state = !(P3IN & BIT6);
	uint8_t s3_state = !(P2IN & BIT2);
	uint8_t s4_state = !(P7IN & BIT4);
	return (s4_state << 3) | (s3_state << 2) | (s2_state << 1) | s1_state;
}
//
//
//    //Function that executes our game over segment. Flashing all LEDs, and displaying game over message
//    void runGameOver(void) {
//        Graphics_clearDisplay(&g_sContext);
//        Graphics_drawStringCentered(&g_sContext, "GAME OVER", AUTO_STRING_LENGTH, 48, 48, OPAQUE_TEXT);
//        Graphics_flushBuffer(&g_sContext);
//
//        int i;
//        for (i = 0; i < 3; i++) {
//
////          Turn all LEDs on
//            setLeds(0x0F);
//
//            Turn on Buzzer
//            BuzzerOn(128);
//
//            swDelay(5);
//
//
//            setLeds(0);
//            BuzzerOff();
//
//            swDelay(5);
//        }
//
//        swDelay(5);
//    }
//
//    //Function to check if the S1 and S2 buttons are pressed
//    char checkButtons(void) {
//            char s1_pressed = !(P2IN & BIT1);
//            char s2_pressed = !(P1IN & BIT1);
//            return (s1_pressed && s2_pressed);
//        }
//
//
//    //Countdown for starting the game
//    void doCountdown(void) {
//        unsigned char delay = 10;
//
//        Graphics_clearDisplay(&g_sContext);
//
//        Graphics_drawStringCentered(&g_sContext, "3", 1, 48, 48, OPAQUE_TEXT);
//        Graphics_flushBuffer(&g_sContext);
//        swDelay(delay);
//
//        Graphics_drawStringCentered(&g_sContext, "2", 1, 48, 48, OPAQUE_TEXT);
//        Graphics_flushBuffer(&g_sContext);
//        swDelay(delay);
//
//        Graphics_drawStringCentered(&g_sContext, "1", 1, 48, 48, OPAQUE_TEXT);
//        Graphics_flushBuffer(&g_sContext);
//        swDelay(delay);
//    }
//
//
//    // Main function to play sequence. Associates LED num with appropriate BIT values
//    void playSeq(int seq[], int len) {
//
//        unsigned char delay = 30;
//        int i;
//        int scaledDelay = (delay-(len*3));
//        for (i = 0; i < len; i++) {
//            int ledNum = seq[i];
//
//            BuzzerOn(128 - ledNum*15);
//
//            if (ledNum == 1) {
//                setLeds(BIT3);
//            }
//            else if (ledNum == 2) {
//                setLeds(BIT2);
//            }
//            else if (ledNum == 3) {
//                setLeds(BIT1);
//            }
//            else if (ledNum == 4) {
//                setLeds(BIT0);
//            }
//            swDelay(scaledDelay <= 5 ? 5 : scaledDelay);
//
//            BuzzerOff();
//            setLeds(0);
//
//            swDelay(scaledDelay <= 5 ? 5 : scaledDelay);
//        }
//    }
//
//    // Function to initialize the start of the game before the State Machine, to avoid redrawing in loop
//    void welcomeScreen(enum GAME_STATE *state) {
//        *state = WELCOME;
//        Graphics_clearDisplay(&g_sContext);
//        Graphics_drawStringCentered(&g_sContext, "SIMON SAYS", AUTO_STRING_LENGTH, 48, 30, TRANSPARENT_TEXT);
//        Graphics_drawStringCentered(&g_sContext, "Press * to Play", AUTO_STRING_LENGTH, 48, 50, TRANSPARENT_TEXT);
//        Graphics_flushBuffer(&g_sContext);
//    }
//
//// Main
//void main(void)
//
//{
//    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
//                                 // You can then configure it properly, if desired
//    // Useful code starts here
//    // Initialize important HW
//    initLeds();
//    initButtons();
//    configDisplay();
//    configKeypad();
//    //Randomize the game every time
//    srand(time(NULL));
//
//    enum GAME_STATE state;
//    welcomeScreen(&state);
//    char currKey = 0;
//    char dispKey;
//    char keyPressed;
//    while (1)
//    {
//
//        if (checkButtons()) {
//                    welcomeScreen(&state);
//                    seqLen = 0;
//                }
//        switch(state) {
//        case WELCOME:
//            seqLen = 0;
//            playerPos = 0;
//
//            currKey = getKey();
//            if (currKey == '*') {
//                doCountdown();
//                state = PLAY_SEQ;
//            }
//            break;
//        case PLAY_SEQ:
//            if (seqLen < MAX_SEQ_LEN) {
//                gameSeq[seqLen] = (rand() % 4) + 1;
//                seqLen++;
//            } else {
//                Graphics_clearDisplay(&g_sContext);
//                Graphics_drawStringCentered(&g_sContext, "CONGRATULATIONS!", AUTO_STRING_LENGTH, 48, 48, OPAQUE_TEXT);
//                Graphics_flushBuffer(&g_sContext);
//                swDelay(50);
//                welcomeScreen(&state);
//                break;
//            }
//            playerPos = 0;
//            playSeq(gameSeq, seqLen);
//            Graphics_clearDisplay(&g_sContext);
//            Graphics_flushBuffer(&g_sContext);
//            swDelay(1);
//            state = CHECK_INP;
//            break;
//        case CHECK_INP:
//            keyPressed = getKey();
//            char dispNum[2];
//            dispNum[0] = keyPressed;
//            dispNum[1] = '\0';
//            if (keyPressed != 0){
//                int numberEntered = keyPressed - 48;
//                Graphics_clearDisplay(&g_sContext);
//                Graphics_drawStringCentered(&g_sContext, dispNum, 1, (10 + numberEntered*15) , 48, OPAQUE_TEXT);
//                Graphics_flushBuffer(&g_sContext);
//                if (numberEntered == gameSeq[playerPos]){
//                    playerPos++;
//                    if(playerPos == seqLen){
//                        //go back to play
//                        state = PLAY_SEQ;
//                            }
//                        } else{
//                            // return to start
//                            playerPos = 0;
//                            swDelay(45);
//                            state = FAIL_ERROR;
//                        }
//                        swDelay(30);
//                    }
//                    break;
//        case FAIL_ERROR:
//            runGameOver();
//            welcomeScreen(&state);
//            break;
//        }
//        }
//    }


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define TIMER_RESOLUTION_MS 5
#define TICKS_PER_SECOND (32768 / 163)
#define COUNTDOWN_TICKS TICKS_PER_SECOND * 3
volatile uint32_t global_timer_ticks = 0;
volatile uint32_t next_state_tick_target = 0;

void configure_timer_a2() {
    TA2CTL = TASSEL_1 + MC_1 + ID_0 + TACLR;
    // 32768 Hz / 200 Hz = 163.84
    TA2CCR0 = 163;
    TA2CCTL0 |= CCIE;
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {
    global_timer_ticks++;
//    if (global_timer_ticks % 20 == 0) {
//        setLeds(BIT1);
//    }
//    else {
//        setLeds(0);
//    }
}

//uint16_t freq_to_ccr0(NotePitch freq_hz) {
//    if (freq_hz == 0) {
//        return 0;
//    }
//    return (uint16_t)(32768.0 / (float)freq_hz);
//}

//void buzzer_on(NotePitch pitch) {
//    uint16_t ccr0_val = freq_to_ccr0(pitch);
//    if (ccr0_val > 0) {
//        TB0CCR0 = ccr0_val;
//        TB0CCR5 = ccr0_val / 2;
//        TB0CTL |= MC_1;
//    }
//}

void buzzer_off() {
    TB0CTL &= ~MC_1;
    TB0CCR5 = 0;
}

void do_countdown() {
    uint32_t elapsed_ticks = global_timer_ticks - next_state_tick_target;
    uint16_t one_second_ticks = TICKS_PER_SECOND;

    if (elapsed_ticks < (1 * one_second_ticks)) {
//        lcd_message("3...");
        setLeds(BIT0);
    } else if (elapsed_ticks < (2 * one_second_ticks)) {
//        lcd_message("2..");
        setLeds(BIT1);
    } else if (elapsed_ticks < (3 * one_second_ticks)) {
//        lcd_message("1.");
        setLeds(BIT2);
    } else if (elapsed_ticks < (4 * one_second_ticks)) {
//        lcd_message("GO!");
        setLeds(BIT3);
    } else {
//        lcd_clear();
        setLeds(0);
//        currentState = PLAY_SONG;
        next_state_tick_target = global_timer_ticks;
//        current_note_index = 0;
    }
}

void main(void) {
    WDTCTL = WDTPW | WDTHOLD;
    initLeds();
    configure_timer_a2();
    __enable_interrupt();
    next_state_tick_target = global_timer_ticks;
    uint32_t elapsed_ticks = global_timer_ticks - next_state_tick_target;
//    uint16_t one_second_ticks = TICKS_PER_SECOND;
    while(1) {
        playNote(song[current_note].pitch);
        swDelay(song[current_note].duration);
        current_note++;
        if (current_note == 84)
            current_note = 0;
    }
}


int pitchToTicks(int pitch){
    int ticks = 32768 / pitches[pitch];
    return ticks;
}

int pitchToLED(int pitch){
    setLeds(BIT4 >> ((pitch % 4) + 1));
    return (pitch % 4) + 1;
}

int playNote(int pitch)
{
    int led = 0;

    if(pitch == 0){
        BuzzerOff();
        setLeds(led);
    } else{
        BuzzerOn(pitchToTicks(pitch));
        led = pitchToLED(pitch);
        setLeds(BIT4 >> led);
    }
    return led;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////


//volatile unsigned long timer = 0;
//volatile unsigned char leap = 0;
//
//#pragma vector = TIMER2_A0_VECTOR
//__interrupt void Timer_A2_ISR(void) {
//    if (leap < 1819) {
//        timer++;
//        leap++;
//    }
//    else {
//        leap = 0
//    }
//}
//
//
//void config() {
//    P5SEL |= BIT2 | BIT3 | BIT4 | BIT5;
//    UCSCTL6 &= ~(XT2DRIVE_3 | XT2OFF);
//    UCSCTL6 |= XT2DRIVE_1;
//    UCSCTL4 &= ~SELS_7;
//    UCSCTL4 |= SELS_5;
//}
//
//
//volatile unsigned long timer = 0;
//void setupTimerA2(){
//    TA2CTL = TASSEL_2 | MC_1 | ID_0; //divider = 1
//    //0.25ms = (max_count + 1) * 1/f
//    //max_count = 1999
//    TA2CCR0 = 1999; // max_count + 1 = 2000 => 0.25ms
//    TA2CCTL0 = CCIE; //Enable the interrupt
//}
//
//#pragma vector = TIMER2_A0_VECTOR
//__interrupt void Timer_A2_ISR(void) {
//    timer++
//    }
//
//
//void countToDisplay(int count) {
//    long unsigned int remaining = count * 2.5;
//    char time_str[7];
//    for (int i = 6; i >= 0; --i) {
//        if (i == 2) {
//            time_str[i] = '.';
//        } else {
//            time_str[i] = (remaining % 10) + '0';
//            remaining /= 10;
//        }
//    }
//}
//
//
//#include "msp430.h"
//void config() {
//    //configure P2 digit I/O and direction
//
//    //Configuring the INPUT
//    //First clear P2.2 and P2.3
//    P2SEL &= ~(BIT2 | BIT3);
//    //Make P2.2 and P2.3 as inputs
//    P2DIR &= ~(BIT2 | BIT3);
//
//    //Configuring the OUTPUT
//    //First clear P2.6 and P2.7
//    P2SEL &= ~(BIT6 | BIT7);
//    //P2.6 and P2.7 as outputs
//    P2DIR |= (BIT6 | BIT7);
//
//    //Set internal pull up or pull down resistors if needed
//
//    //Enable pull up resistors for P2.2 and P2.3
//    P2REN |= (BIT2 | BIT3);
//    //Set pull up resistors for P2.2 and P2.3
//    P2OUT |= (BIT2 | BIT3);
//}
//
//
////use P2IN as input
//unsigned char read_switches() {
//    unsigned char switches = 0;
//    //Checking if S1 is pushed and S2 is not
//    //Since pushing will be logic level 0
//    if (!(P2IN & BIT2) && (P2IN & BIT3)) {
//        switches = 1;
//    }
//    //Checking if S2 is pushed and S1 is not
//    else if ((P2IN & BIT2) && !(P2IN & BIT3)) {
//        switches = 2;
//    }
//    //Checking if both buttons are pushed
//    else if (!(P2IN & BIT2) && !(P2IN & BIT3)) {
//        switches = 3;
//    }
//    else {
//        //If none are pushed set the read switches value to 0
//        switches = 0;
//    }
//}
//
//void switches_to_leds() {
//    unsigned char switch_values;
//    //Using the function from the previous part to get the values 0, 1, 2 or 3
//    switch_values = read_switches();
//    //Initialize by clearing bit 6 and 7, that is turning off all the LEDs
//    P2OUT &= ~(BIT6 | BIT7);
//    //Checking to see if read_switches passes in 1. If so, turn on LED1, which is connected to P2.7
//    if (switch_values == 1) {
//        P2OUT |= BIT7;
//    }
//    //Checking to see if read_switches passes in 2. If so, turn on LED2, which is connected to P2.6
//    else if (switch_values == 2) {
//        P2OUT |= BIT6;
//    }
//    //Checking for switch_values 0 and 3 aren't needed since the initial statement already turns off all the LEDs
//}
//
//void runTimerA2(void) {
//    TA2CTL = TASSEL_1 + MC_1 + ID_0;
//    TA2CCR0 = 883;
//    TA2CCTL0 = CCIE;
//}
