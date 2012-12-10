#include "../sput/sput.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)

/* CONFIG */
#ifndef SPUT_SAMPLE_RATE
#define SPUT_SAMPLE_RATE 48000.0
#endif

/* Implementation constants. */
#define PHASOR_PERIOD 4294967296.0 // 32 bit phasor
#define NOTES_PER_OCTAVE 12.0
#define REF_FREQ 440.0
#define REF_NOTE 69.0

/* TOOLS */
#define NB_EL(x) (sizeof(x)/sizeof((x)[0]))
#define FOR_IN(i,a) for(i=0; i<NB_EL(a); i++)


/* MIDI note to voice map, necessary for turning off voices. */
static int note2voice(struct sput *x, int note) {
    return x->note2voice[note % 128];
}
static void set_note2voice(struct sput *x, int note, int voice) {
    x->note2voice[note % 128] = voice % NB_EL(x->voice);
}

/* Map midi note to octave, note */
#define FREQ_TO_INC(freq)  (((freq) / SPUT_SAMPLE_RATE) * PHASOR_PERIOD)

#if 1
/* Table based. */

/* Create the phasor increments for an equally tempered chromatic
   scale using a floating point sequence.  Other octaves are derived
   from these by shifting. */

#define SEMI          0.9438743126816935 // 2 ^ {1/12}
#define MIDI_NOTE_127 12543.853951415975 // 440 ^ {2^{127-69/12}}, frequency of MIDI note 127

#define N0 (SEMI*N1)
#define N1 (SEMI*N2)
#define N2 (SEMI*N3)
#define N3 (SEMI*N4)
#define N4 (SEMI*N5)
#define N5 (SEMI*N6)
#define N6 (SEMI*N7)
#define N7 (SEMI*N8)
#define N8 (SEMI*N9)
#define N9 (SEMI*N10)
#define N10 (SEMI*N11)
#define N11 FREQ_TO_INC(MIDI_NOTE_127)

static const phasor_t note_tab[12] = {
    N0, N1, N2,  N3,  // 116 - 119
    N4, N5, N6,  N7,  // 120 - 123
    N8, N9, N10, N11, // 124 - 127
};

/* Create a midi note -> octave, note map */
#define NOTE(o,n) \
    ((((o) & 15) << 4) | ((n) & 15))
#define OCTAVE(o) \
    NOTE(o,0), NOTE(o,1), NOTE(o,2),  NOTE(o,3), \
    NOTE(o,4), NOTE(o,5), NOTE(o,6),  NOTE(o,7), \
    NOTE(o,8), NOTE(o,9), NOTE(o,10), NOTE(o,11)

const u8 midi_tab[128] = {
    NOTE(10,4), NOTE(10,5), NOTE(10,6),  NOTE(10,7),
    NOTE(10,8), NOTE(10,9), NOTE(10,10), NOTE(10,11),
    OCTAVE(9),
    OCTAVE(8), OCTAVE(7), OCTAVE(6),
    OCTAVE(5), OCTAVE(4), OCTAVE(3),
    OCTAVE(2), OCTAVE(1), OCTAVE(0),
};

/* Combine both tables. */
phasor_t note_to_inc(int note) {
    int octave_note = midi_tab[note & 127];
    int octave = octave_note >> 4;
    int n = octave_note & 15;
    phasor_t p = note_tab[n] >> octave;
    LOG("%d -> (%d,%d,%d,%d)\n", note, octave, n, note_tab[n], p);
    return p;
}


#else

#define NOTE_TO_FREQ(note) (REF_FREQ * POW2((((note) - REF_NOTE) / NOTES_PER_OCTAVE)))
#define NOTE_TO_INC(note)  (FREQ_TO_INC(NOTE_TO_FREQ(note)))
#define POW2(x) pow(2,x)

phasor_t note_to_inc(int i_note) {
    double note = i_note;
    /* 60 -> 440Hz */
    double freq = NOTE_TO_FREQ(note);
    double inc = FREQ_TO_INC(freq);
    phasor_t i_inc = (inc + 0.5);
    LOG("freq = %f -> %f, %d\n", note, freq, i_inc);
    return i_inc;
}
#endif

int voice_alloc(struct sput *x) {
    unsigned int v;
    FOR_IN(v, x->voice) {
        if (x->voice[v].note_inc == 0) return v;
    }
    /* This is not good, but better than doing nothing.  FIXME: Use
       current envelope value to perform selection.  Data org: keep
       envolopes together. */
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
float sum_tick_saw(struct sput *x) {
    unsigned int v;
    int sum = 0;
    FOR_IN(v, x->voice) {
        if (x->voice[v].note_inc) {
            /* Shift is arbitrary, but we interpret phasor as signed. */
            int p = x->voice[v].note_state;
            sum += (p >> 4);
            x->voice[v].note_state += x->voice[v].note_inc;
        }
    }
    return (1.0 / PHASOR_PERIOD) * ((float)sum);
}
float sum_tick_square(struct sput *x) {
    unsigned int v;
    unsigned int accu = 0;
    FOR_IN(v, x->voice) {
        if (x->voice[v].note_inc) {
            /* Shift is arbitrary, but we interpret phasor as signed. */
            unsigned int bit = x->voice[v].note_state & 0x80000000;
            // accu ^= bit;
            accu |= bit;
            x->voice[v].note_state += x->voice[v].note_inc;
        }
    }
    return (1.0 / PHASOR_PERIOD) * ((float)accu);
}
void sput_run(struct sput *x, float *vec, int n) {
    int i;
    for (i=0; i<n; i++) {
        vec[i] = sum_tick_saw(x);
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

   Voice architecture:
   - classic: each voice: VCO -> VCF -> VCA; each section with envelope
   - FM/PM: modulation algos
   - dynwav / additive synthesis (model based)
*/


/* TODO:
   - Output subsampler
   - Voice allocator
   - Wavetable interpolator
   - Don't use float except for initializing tables (all fixed point, needs to run on Arduino)
*/
