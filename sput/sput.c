#include "../sput/sput.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)

/* CONFIG */
#define SAMPLE_RATE 48000.0
#define PHASOR_PERIOD 4294967296.0  // 32 bit phasor
#define NOTES_PER_OCTAVE 12.0
#define REF_FREQ 440.0
#define REF_NOTE 60.0

/* TOOLS */
#define NB_EL(x) (sizeof(x)/sizeof((x)[0]))
#define FOR_IN(i,a) for(i=0; i<NB_EL(a); i++)


/* MIDI note to voice map, necessary for turning off voices. */
static int note2voice(struct sput *x, int note) {
    return x->note2voice[note % 128];
}
static void set_note2voice(struct sput *x, int note, int voice) {
    x->note2voice[note % 128] = voice % SPUT_NB_VOICES;
}

/* FIXME: no floats! */
int note_to_inc(int i_note) {
    double note = i_note;
    /* 60 -> 440Hz */
    double freq = REF_FREQ * pow(2, ((note - REF_NOTE) / NOTES_PER_OCTAVE));
    LOG("freq = %f -> %f\n", note, freq);
    double inc = (freq / SAMPLE_RATE) * PHASOR_PERIOD;
    int i_inc = (inc + 0.5);
    return i_inc;
}

int voice_alloc(struct sput *x) {
    unsigned int v;
    FOR_IN(v, x->voice) {
        if (x->voice[v].note_inc == 0) return v;
    }
    /* This is not good, but better than doing nothing.  FIXME: Add
       some kind of tagging to do LRU alloc. */
    return 0;
}

void sput_note_on(struct sput *x, int note) {
    int v = voice_alloc(x);
    x->note2voice[note % 128] = v;
    x->voice[v].note_inc = note_to_inc(note % 128);
}
void sput_note_off(struct sput *x, int note) {
    int v = x->note2voice[note % 128];
    x->note2voice[note % 128] = 0;
    x->voice[v].note_inc = 0;
}

/* MIDI note state */
// FIXME: no float!
float sum_tick(struct sput *x) {
    unsigned int v;
    float f_sum = 0;
    FOR_IN(v, x->voice) {
        if (x->voice[v].note_inc) {
            float f_state = x->voice[v].note_state;
            f_state -= PHASOR_PERIOD / 2;
            f_state *= (0.1) * (1.0 / PHASOR_PERIOD);
            // LOG("f_state = %f\n", f_state);
            f_sum += f_state;
            x->voice[v].note_state += x->voice[v].note_inc;
        }
    }
    return f_sum;
}
void sput_run(struct sput *x, float *vec, int n) {
    int i;
    for (i=0; i<n; i++) {
        vec[i] = sum_tick(x);
    }
}

void sput_init(struct sput *x) {
    bzero(x, sizeof(*x));
}

/* The engine is just a bunch of one and two pole IIR filters and a
   patch matrix.  It uses MMX/SSE2 fixed point to have clipping for
   free, and to make it easy to implement square wave oscillators.
   Because of feedback this is hard to unroll and data-dependencies
   probably kill performance.  It uses 'd'-domain ops, instead of
   'z'-domain ops.  This is because it's ran at a high samplerate, so
   poles will be very near 0.

   Basic principle for oversampled synthesis: nonlinearities create
   high frequency components, so you want to filter before applying a
   new nonlinearity.


*/


/* TODO:
   - Output subsampler
   - Voice allocator
   - Wavetable interpolator
   - Don't use float except for initializing tables (all fixed point, needs to run on Arduino)
*/
