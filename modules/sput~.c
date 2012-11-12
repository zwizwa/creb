/*
 *   sput~.c  - Pd wrapper for sput.c synth
 *   Copyright (c) 2000-2003 by Tom Schouten
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "m_pd.h"
#include "../sput/sput.h"
#include <math.h>

struct sput_pd {
    t_object x_obj;
    t_float x_f;
    struct sput sput;
};

static void sput_pd_free(struct sput_pd *x) {
}

static void sput_pd_note_on(struct sput_pd *x, t_floatarg note) {
    sput_note_on(&x->sput, note);
}
static void sput_pd_note_off(struct sput_pd *x, t_floatarg note) {
    sput_note_off(&x->sput, note);
}
static void sput_pd_notein(struct sput_pd *x, t_floatarg note, t_floatarg vel) {
    if (vel == 0) {
        sput_note_off(&x->sput, note);
    }
    else {
        sput_note_on(&x->sput, note);
    }
}
static t_int *sput_pd_perform(t_int *w) {
    struct sput *x = (void*)w[1];
    t_int n = (t_int)(w[2]);
    t_float *out = (float *)(w[3]);
    int i;
    for (i=0; i<n; i++) out[i] = 0;
    return w+4;
}
static void sput_pd_dsp(struct sput_pd *x, t_signal **sp) {
    dsp_add(sput_pd_perform, 3, &x->sput, sp[0]->s_n, sp[0]->s_vec);
} 
t_class *sput_pd_class;

static void *sput_pd_new(void) {
    struct sput_pd *x = (void*)pd_new(sput_pd_class);
    sput_init(&x->sput);
    outlet_new(&x->x_obj, gensym("signal"));
    return (void *)x;
}


void sput_tilde_setup(void) {
    sput_pd_class = class_new(gensym("sput~"), (t_newmethod)sput_pd_new,
    	(t_method)sput_pd_free, sizeof(struct sput_pd), 0, 0);
    class_addmethod(sput_pd_class, (t_method)sput_pd_dsp, gensym("dsp"), 0);
    class_addmethod(sput_pd_class, (t_method)sput_pd_note_on, gensym("note_on"), A_FLOAT, A_NULL);
    class_addmethod(sput_pd_class, (t_method)sput_pd_note_on, gensym("note_off"), A_FLOAT, A_NULL); 
    class_addmethod(sput_pd_class, (t_method)sput_pd_notein, gensym("notein"), A_FLOAT, A_FLOAT, A_NULL); 

}
