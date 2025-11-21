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
void swDelay(char numLoops);
void config_buttons(void);
uint8_t get_button_states(void);
int pitchToTicks(int pitch);
int pitchToLED(int pitch);
int playNote(int pitch);


// Declare globals here
enum GAME_STATE {WELCOME = 0, PLAY_SOUND = 1, WIN = 2, LOSE = 3};

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
//        if (global_timer_ticks % 20 == 0) {
//            setLeds(BIT1);
//        }
//        else {
//            setLeds(0);
//        }
}

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

//// Main
void main(void)

{
    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
                                 // You can then configure it properly, if desired
    // Useful code starts here
    // Initialize important HW
    initLeds();
    config_buttons();
    configDisplay();
    configKeypad();

    configure_timer_a2();
    __enable_interrupt();


    enum GAME_STATE state = PLAY_SOUND;

    int current_note = 0;
    uint32_t next_note_tick = 0;
    int current_LED = -1;

    while (1)
    {

//        if (checkButtons()) {
//                    welcomeScreen(&state);
//                    seqLen = 0;
//                }
        switch(state) {
        case WELCOME:

            if (true)
            {
                state = PLAY_SOUND;
                current_note = 0;
                next_note_tick = 0;
            }

            break;
        case PLAY_SOUND:



            if (next_note_tick < global_timer_ticks)
            {
                current_LED = playNote(song[current_note].pitch);
                next_note_tick = global_timer_ticks + (song[current_note].duration * 10);
                current_note++;
            }


            if (current_note == 84)
            {
                state = WIN;
//                state = LOSE;
            }

            break;
        case WIN:
            setLeds(1);
            //congratulations()
            //welcomeScreen(&state);
            break;
        case LOSE:
            //playerHumiliation();
            //welcomeScreen(&state);
            break;
        }
        }
    }
