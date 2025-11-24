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

enum GAME_STATE {WELCOME = 0, PLAY_SOUND = 1, WIN = 2, LOSE = 3};

// Function Prototypes
void swDelay(char numLoops);
void config_buttons(void);
uint8_t get_button_states(void);
int pitchToTicks(int pitch);
int pitchToLED(int pitch);
int playNote(int pitch);
void welcomeScreen(enum GAME_STATE *state);
char checkButton(char currKey);
void do_countdown(enum GAME_STATE* state);


// Declare globals here
#define TIMER_RESOLUTION_MS 5
#define TICKS_PER_SECOND (32768 / 163)
#define COUNTDOWN_TICKS TICKS_PER_SECOND * 3
volatile uint32_t global_timer_ticks = 0;
volatile uint32_t next_state_tick_target = 0;
bool countdown = 0;

void configure_timer_a2() {
    TA2CTL = TASSEL_1 + MC_1 + ID_0 + TACLR;
    // 32768 Hz / 200 Hz = 163.84
    TA2CCR0 = 163;
    TA2CCTL0 |= CCIE;
}

#pragma vector=TIMER2_A0_VECTOR
__interrupt void Timer_A2_ISR(void) {
    global_timer_ticks++;
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

struct Note taunt_song[] = {
    {7, 8}, {0, 4},
    {6, 8}, {0, 4},
    {5, 8}, {0, 4},
    {4, 50}, {0, 8}
};

struct Note victory_song[] = {
    // --- PART 1: Ascending Triplet 1 (Da-da-da) ---
    {4, 6},  {0, 2}, // C
    {8, 6},  {0, 2}, // E
    {11, 6}, {0, 2}, // G

    // --- PART 2: Ascending Triplet 2 (Da-da-da) ---
    {4, 6},  {0, 2}, // C
    {8, 6},  {0, 2}, // E
    {11, 6}, {0, 2}, // G

    // --- PART 3: The "Victory" Melody ---
    {1, 12}, {0, 2}, // High A
    {11, 12},{0, 2}, // G
    {8, 12}, {0, 2}, // E
    {4, 12}, {0, 2}, // C

    // --- PART 4: Grand Finish ---
    {11, 8}, {0, 2}, // G (Setup)
    {1, 40}, {0, 20} // High A (Long finish)
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
                 //        if (global_timer_ticks % 20 == 0) {
                 //            setLeds(BIT1);
                 //        }
                 //        else {
                 //            setLeds(0);
                 //        }(pitch));
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

    int current_note = 0;
    uint32_t next_note_tick = 0;
    int current_LED = -1;

    enum GAME_STATE state;
    welcomeScreen(&state);
    char currKey = 0;

	int missedNotes = 0;
	bool notePlayed = false;

    while (1)
    {
        currKey = getKey();
        if (checkButton(currKey)) {
            welcomeScreen(&state);
            setLeds(0);
        }
        switch(state) {
        case WELCOME:
            BuzzerOff();
            current_note = 0;
            next_note_tick = 0;
            missedNotes = 0;

            currKey = getKey();
            if (currKey == '*') {
                Graphics_clearDisplay(&g_sContext);
                next_state_tick_target = global_timer_ticks;
                countdown = 1;
            }

            if (countdown) {
                do_countdown(&state);
            }

            break;
        case PLAY_SOUND:
            if (next_note_tick < global_timer_ticks)
            {
                // increments number of missed notes if user missed their window to press button
                if (!notePlayed) {
                    missedNotes++;
                }

                current_LED = playNote(song[current_note].pitch);
                next_note_tick = global_timer_ticks + (song[current_note].duration * 8);
                current_note++;

                // user has not played the new note yet
                if (current_LED == 0)
                    notePlayed = true;
                else
                    notePlayed = false;
            }

            // logs that the user pressed the required note
            if (get_button_states() == (1 << (current_LED - 1)) && !notePlayed) {
				notePlayed = true;
			// wrong button press missed locks out another button press and adds missed note
			} else if (get_button_states() && get_button_states() != (1 << (current_LED - 1)) && !notePlayed) {
			    notePlayed = true;
                missedNotes++;
			}

            // fails the user if they miss too many notes
            if (missedNotes >= 5) {
                current_note = 0;
                next_note_tick = 0;
                BuzzerOff();
                Graphics_clearDisplay(&g_sContext);
                state = LOSE;
            }


            // user wins if they make it through the entire song
            //84 total notes
            if (current_note == 84)
            {
                current_note = 0;
                next_note_tick = 0;
                BuzzerOff();
                Graphics_clearDisplay(&g_sContext);
                state = WIN;
            }

            break;
        case WIN:
            if (next_note_tick < global_timer_ticks) {
                if (current_note == 24) {
                    current_note = 0;
                }
                playNote(victory_song[current_note].pitch);
                next_note_tick = global_timer_ticks + (victory_song[current_note].duration);
                current_note++;
            }
            //congratulations()
//            if (current_note <= 24) {
            Graphics_drawStringCentered(&g_sContext, "You Won! :)", AUTO_STRING_LENGTH, 48, 40, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "Press # to Reset", AUTO_STRING_LENGTH, 48, 60, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);
//            }
//            else {
//                Graphics_clearDisplay(&g_sContext);
//                welcomeScreen(&state);
//            }
            break;
        case LOSE:
            if (next_note_tick < global_timer_ticks) {
                if (current_note == 8) {
                    current_note = 0;
                }
                playNote(taunt_song[current_note].pitch);
                next_note_tick = global_timer_ticks + (taunt_song[current_note].duration * 5);
                current_note++;
            }
            Graphics_drawStringCentered(&g_sContext, "You Lost :(", AUTO_STRING_LENGTH, 48, 40, TRANSPARENT_TEXT);
            Graphics_drawStringCentered(&g_sContext, "Press # to Reset", AUTO_STRING_LENGTH, 48, 60, TRANSPARENT_TEXT);
            Graphics_flushBuffer(&g_sContext);
            break;
        }
        }
    }

void do_countdown(enum GAME_STATE* state) {
    uint16_t one_second_ticks = TICKS_PER_SECOND;
    if (global_timer_ticks < (next_state_tick_target + (1 * one_second_ticks))) {
        Graphics_drawStringCentered(&g_sContext, "3", 1, 48, 48, OPAQUE_TEXT);
        Graphics_flushBuffer(&g_sContext);
        setLeds(BIT3);
    } else if (global_timer_ticks < (next_state_tick_target + (2 * one_second_ticks))) {
        Graphics_drawStringCentered(&g_sContext, "2", 1, 48, 48, OPAQUE_TEXT);
        Graphics_flushBuffer(&g_sContext);
        setLeds(BIT2);
    } else if (global_timer_ticks < (next_state_tick_target + (3 * one_second_ticks))) {
        Graphics_drawStringCentered(&g_sContext, "1", 1, 48, 48, OPAQUE_TEXT);
        Graphics_flushBuffer(&g_sContext);
        setLeds(BIT1);
    } else {
        Graphics_drawStringCentered(&g_sContext, "GO!", 3, 48, 48, OPAQUE_TEXT);
        Graphics_flushBuffer(&g_sContext);
        setLeds(0);
        countdown = 0;
        *state = PLAY_SOUND;
    }
}

// Function to initialize the start of the game before the State Machine, to avoid redrawing in loop
    void welcomeScreen(enum GAME_STATE *state) {
        *state = WELCOME;
        Graphics_clearDisplay(&g_sContext);
        Graphics_drawStringCentered(&g_sContext, "GUITAR HERO", AUTO_STRING_LENGTH, 48, 30, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "Press * to Play", AUTO_STRING_LENGTH, 48, 50, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "Press # to Reset", AUTO_STRING_LENGTH, 48, 70, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
    }

    //Function to check if the # buttons are pressed
    char checkButton(char currKey) {
        if (currKey == '#') {
            return 1;
        }
        else {
            return 0;
        }
    }
