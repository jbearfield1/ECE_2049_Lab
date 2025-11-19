/*
 * score.h
 *
 *  Created on: Nov 12, 2025
 *      Author: Rishi Halker
 */

#ifndef SCORE_H
#define SCORE_H
#include <stdint.h>
#include <stdbool.h>

#define SCORE_FULL_POINTS 100

typedef struct {
    uint32_t points;
    uint16_t combo;
    uint16_t maxCombo;
} ScoreTracker;

void Score_Init(ScoreTracker *s);

// Call this repeatedly each tick while the note/LED is active.
// Returns true if a full-score hit occurred.
bool Score_PollAndUpdate(ScoreTracker *s,
                         uint8_t  ledId,           // 0..3 (which LED is on for this note)
                         uint32_t nowTick,         // TA2 ms tick
                         uint32_t noteStartTick,   // when LED turned on
                         uint32_t noteEndTick);    // when LED turned off

#endif
