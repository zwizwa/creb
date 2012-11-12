#include "../sput/sput.h"
#include <stdio.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)
void sput_note_on(struct sput *x, int note) {
    LOG("on %d\n", note);
}
void sput_note_off(struct sput *x, int note) {
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


