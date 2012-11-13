#ifndef _SPUT_H_
#define _SPUT_H_

#define SPUT_NB_VOICES 64
typedef unsigned int phasor_t;
struct voice {
    phasor_t note_inc;  /* 0 == off */
    phasor_t note_state;
};
struct sput {
    int note2voice[128];
    struct voice voice[SPUT_NB_VOICES];
};

void sput_note_on(struct sput *, int note);
void sput_note_off(struct sput *, int note);
void sput_init(struct sput *);
void sput_run(struct sput *, float *vec, int n);

#endif
