/*
 * score.c
 *
 *  Created on: Nov 12, 2025
 *      Author: Rishi Halker
 */

#include "score.h"
#include "buttons.h"

static inline uint8_t ExpectedBtnMask(uint8_t ledId) {
    static const uint8_t map[4] = { 0x01, 0x02, 0x04, 0x08 }; // S1,S2,S3,S4
    return (ledId < 4) ? map[ledId] : 0;
}

void Score_Init(ScoreTracker *s) {
    s->points = 0;
    s->combo  = 0;
    s->maxCombo = 0;
}

// internal: remember last raw mask to detect new presses (edges)
static uint8_t s_prevMask = 0;

bool Score_PollAndUpdate(ScoreTracker *s,
                         uint8_t  ledId,
                         uint32_t nowTick,
                         uint32_t noteStartTick,
                         uint32_t noteEndTick)
{
    // Only score if  inside the LED-on window
    if ((int32_t)(nowTick - noteStartTick) < 0) return false;
    if ((int32_t)(noteEndTick - nowTick)  < 0) return false;

    uint8_t mask = Buttons_ReadMask();
    uint8_t newPresses = (uint8_t)(mask & ~s_prevMask); // edge detect (0->1)
    s_prevMask = mask;

    uint8_t expect = ExpectedBtnMask(ledId);

    if (newPresses & expect) {
        // Full score: correct button pressed while LED is on
        s->points += SCORE_FULL_POINTS;
        if (++s->combo > s->maxCombo) s->maxCombo = s->combo;
        return true;
    }

    // Any wrong button during the window breaks combo (but adds no points)
    if (newPresses & ~expect) {
        s->combo = 0;
    }
    return false;
}


