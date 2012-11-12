#include "../sput/sput.h"
#include <stdio.h>
#include <math.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)

#define SAMPLE_RATE 48000.0
#define PHASOR_PERIOD 4294967296.0  // 32 bit phasor
#define NOTES_PER_OCTAVE 12.0
#define REF_FREQ 440.0
#define REF_NOTE 60.0

/* MIDI note state */
#define NB_EL(x) (sizeof(x)/sizeof((x)[0]))
typedef unsigned int phasor_t;
phasor_t note_inc[128] = {};
phasor_t note_state[128] = {};

float sum_state(void) {
    unsigned int i;
    float f_sum = 0;
    for (i=0; i<NB_EL(note_state); i++) {
        if (note_inc[i]) {
            float f_state = note_state[i];
            f_state -= PHASOR_PERIOD / 2;
            f_state *= (0.1) * (1.0 / PHASOR_PERIOD);
            // LOG("f_state = %f\n", f_state);
            f_sum += f_state;
            note_state[i] += note_inc[i];
        }
    }
    return f_sum;
}
void sput_get_samples(float *vec, int n) {
    int i;
    for (i=0; i<n; i++) {
        vec[i] = sum_state();
    }
}


int note_to_inc(int i_note) {
    double note = i_note;
    /* 60 -> 440Hz */
    double freq = REF_FREQ * pow(2, ((note - REF_NOTE) / NOTES_PER_OCTAVE));
    LOG("freq = %f -> %f\n", note, freq);
    double inc = (freq / SAMPLE_RATE) * PHASOR_PERIOD;
    int i_inc = (inc + 0.5);
    return i_inc;
}

/* 60 -> 440Hz */
void sput_note_on(struct sput *x, int note) {
    note_inc[note & 0x7F] = note_to_inc(note);
    LOG("on %d %d\n", note, (int)note_inc[note & 0x7F]);
}
void sput_note_off(struct sput *x, int note) {
    note_state[note & 0x7F] = 0;
    note_inc[note & 0x7F] = 0;
    LOG("off %d\n", note);
}
void sput_init(struct sput *x) {
}

/* The engine is just a bunch of one and two pole IIR filters and a
   patch matrix.  It uses MMX/SSE2 fixed point to have clipping for
   free, and to make it easy to implement square wave oscillators.
   Because of feedback this is hard to unroll and data-dependencies
   probably kill performance.  It uses 'd'-domain ops, instead of
   'z'-domain ops.  This is because it's ran at a high samplerate, so
   poles will be very near 0. */


/* TODO:
   1. Fix samplerate.
   2. Fix phasor periodicity.


 */
