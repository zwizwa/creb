#ifndef _SPUT_H_
#define _SPUT_H_

struct sput {
};

void sput_note_on(struct sput *, int note);
void sput_note_off(struct sput *, int note);
void sput_init(struct sput *);

#endif
