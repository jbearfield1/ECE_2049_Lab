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

// Declare globals here
enum GAME_STATE {WELCOME = 0, PLAY_SEQ = 1, CHECK_INP = 2, FAIL_ERROR = 3};
int gameSeq[MAX_SEQ_LEN];
int seqLen = 0;
int playerPos = 0;
int i;

//Software Delay to run useless loops
void swDelay(char numLoops) {
    volatile unsigned long i;
    char j;
    for (j = 0; j < numLoops; j++) {

        for (i = 1000; i > 0; i--);
    }
}


    //Function that executes our game over segment. Flashing all LEDs, and displaying game over message
    void runGameOver(void) {
        Graphics_clearDisplay(&g_sContext);
        Graphics_drawStringCentered(&g_sContext, "GAME OVER", AUTO_STRING_LENGTH, 48, 48, OPAQUE_TEXT);
        Graphics_flushBuffer(&g_sContext);

        int i;
        for (i = 0; i < 3; i++) {

//          Turn all LEDs on
            setLeds(0x0F);

            Turn on Buzzer
            BuzzerOn(128);

            swDelay(5);


            setLeds(0);
            BuzzerOff();

            swDelay(5);
        }

        swDelay(5);
    }

    //Function to check if the S1 and S2 buttons are pressed
    char checkButtons(void) {
            char s1_pressed = !(P2IN & BIT1);
            char s2_pressed = !(P1IN & BIT1);
            return (s1_pressed && s2_pressed);
        }


    //Countdown for starting the game
    void doCountdown(void) {
        unsigned char delay = 10;

        Graphics_clearDisplay(&g_sContext);

        Graphics_drawStringCentered(&g_sContext, "3", 1, 48, 48, OPAQUE_TEXT);
        Graphics_flushBuffer(&g_sContext);
        swDelay(delay);

        Graphics_drawStringCentered(&g_sContext, "2", 1, 48, 48, OPAQUE_TEXT);
        Graphics_flushBuffer(&g_sContext);
        swDelay(delay);

        Graphics_drawStringCentered(&g_sContext, "1", 1, 48, 48, OPAQUE_TEXT);
        Graphics_flushBuffer(&g_sContext);
        swDelay(delay);
    }


    // Main function to play sequence. Associates LED num with appropriate BIT values
    void playSeq(int seq[], int len) {

        unsigned char delay = 30;
        int i;
        int scaledDelay = (delay-(len*3));
        for (i = 0; i < len; i++) {
            int ledNum = seq[i];

            BuzzerOn(128 - ledNum*15);

            if (ledNum == 1) {
                setLeds(BIT3);
            }
            else if (ledNum == 2) {
                setLeds(BIT2);
            }
            else if (ledNum == 3) {
                setLeds(BIT1);
            }
            else if (ledNum == 4) {
                setLeds(BIT0);
            }
            swDelay(scaledDelay <= 5 ? 5 : scaledDelay);

            BuzzerOff();
            setLeds(0);

            swDelay(scaledDelay <= 5 ? 5 : scaledDelay);
        }
    }

    // Function to initialize the start of the game before the State Machine, to avoid redrawing in loop
    void welcomeScreen(enum GAME_STATE *state) {
        *state = WELCOME;
        Graphics_clearDisplay(&g_sContext);
        Graphics_drawStringCentered(&g_sContext, "SIMON SAYS", AUTO_STRING_LENGTH, 48, 30, TRANSPARENT_TEXT);
        Graphics_drawStringCentered(&g_sContext, "Press * to Play", AUTO_STRING_LENGTH, 48, 50, TRANSPARENT_TEXT);
        Graphics_flushBuffer(&g_sContext);
    }

// Main
void main(void)

{
    WDTCTL = WDTPW | WDTHOLD;    // Stop watchdog timer. Always need to stop this!!
                                 // You can then configure it properly, if desired
    // Useful code starts here
    // Initialize important HW
    initLeds();
    initButtons();
    configDisplay();
    configKeypad();
    //Randomize the game every time
    srand(time(NULL));

    enum GAME_STATE state;
    welcomeScreen(&state);
    char currKey = 0;
    char dispKey;
    char keyPressed;
    while (1)
    {

        if (checkButtons()) {
                    welcomeScreen(&state);
                    seqLen = 0;
                }
        switch(state) {
        case WELCOME:
            seqLen = 0;
            playerPos = 0;

            currKey = getKey();
            if (currKey == '*') {
                doCountdown();
                state = PLAY_SEQ;
            }
            break;
        case PLAY_SEQ:
            if (seqLen < MAX_SEQ_LEN) {
                gameSeq[seqLen] = (rand() % 4) + 1;
                seqLen++;
            } else {
                Graphics_clearDisplay(&g_sContext);
                Graphics_drawStringCentered(&g_sContext, "CONGRATULATIONS!", AUTO_STRING_LENGTH, 48, 48, OPAQUE_TEXT);
                Graphics_flushBuffer(&g_sContext);
                swDelay(50);
                welcomeScreen(&state);
                break;
            }
            playerPos = 0;
            playSeq(gameSeq, seqLen);
            Graphics_clearDisplay(&g_sContext);
            Graphics_flushBuffer(&g_sContext);
            swDelay(1);
            state = CHECK_INP;
            break;
        case CHECK_INP:
            keyPressed = getKey();
            char dispNum[2];
            dispNum[0] = keyPressed;
            dispNum[1] = '\0';
            if (keyPressed != 0){
                int numberEntered = keyPressed - 48;
                Graphics_clearDisplay(&g_sContext);
                Graphics_drawStringCentered(&g_sContext, dispNum, 1, (10 + numberEntered*15) , 48, OPAQUE_TEXT);
                Graphics_flushBuffer(&g_sContext);
                if (numberEntered == gameSeq[playerPos]){
                    playerPos++;
                    if(playerPos == seqLen){
                        //go back to play
                        state = PLAY_SEQ;
                            }
                        } else{
                            // return to start
                            playerPos = 0;
                            swDelay(45);
                            state = FAIL_ERROR;
                        }
                        swDelay(30);
                    }
                    break;
        case FAIL_ERROR:
            runGameOver();
            welcomeScreen(&state);
            break;
        }
        }
    }
