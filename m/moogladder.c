#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include "soundpipe.h"

int sp_moogladder_create(sp_moogladder **t){
    *t = malloc(sizeof(sp_moogladder));
    return SP_OK;
}
int sp_moogladder_destroy(sp_moogladder **t){
    free(*t);
    return SP_OK;
}
int sp_moogladder_init(sp_data *sp, sp_moogladder *p){
    p->istor = 0.0;
    p->res = 0.4;
    p->freq = 1000;
    int i;

    if (p->istor == 0.0) {
      for (i = 0; i < 6; i++)
        p->delay[i] = 0.0;
      for (i = 0; i < 3; i++)
        p->tanhstg[i] = 0.0;
      p->oldfreq = 0.0;
      p->oldres = -1.0;     /* ensure calculation on first cycle */
    }
    return SP_OK;
}

int sp_moogladder_compute(sp_data *sp, sp_moogladder *p, SPFLOAT *in, SPFLOAT *out){
    SPFLOAT   freq = p->freq;
    SPFLOAT   res = p->res;
    SPFLOAT  res4;
    SPFLOAT  *delay = p->delay;
    SPFLOAT  *tanhstg = p->tanhstg;
    SPFLOAT  stg[4], input;
    SPFLOAT  acr, tune;
#define THERMAL (0.000025) /* (1.0 / 40000.0) transistor thermal voltage  */
    int     j, k;

    if (res < 0) res = 0;

    if (p->oldfreq != freq || p->oldres != res) {
        SPFLOAT  f, fc, fc2, fc3, fcr;
        p->oldfreq = freq;
        /* sr is half the actual filter sampling rate  */
        fc =  (SPFLOAT)(freq/sp->sr);
        f  =  0.5*fc;
        fc2 = fc*fc;
        fc3 = fc2*fc;
        /* frequency & amplitude correction  */
        fcr = 1.8730*fc3 + 0.4955*fc2 - 0.6490*fc + 0.9988;
        acr = -3.9364*fc2 + 1.8409*fc + 0.9968;
        tune = (1.0 - exp(-((2 * M_PI)*f*fcr))) / THERMAL;   /* filter tuning  */
        p->oldres = res;
        p->oldacr = acr;
        p->oldtune = tune;
    } else {
        res = p->oldres;
        acr = p->oldacr;
        tune = p->oldtune;
    }
    res4 = 4.0*(SPFLOAT)res*acr;

    /* oversampling  */
    for (j = 0; j < 2; j++) {
        /* filter stages  */
        input = *in - res4 /*4.0*res*acr*/ *delay[5];
        delay[0] = stg[0] = delay[0] + tune*(tanh(input*THERMAL) - tanhstg[0]);
#if 0
        input = stg[0];
        stg[1] = delay[1] + tune*((tanhstg[0] = tanh(input*THERMAL)) - tanhstg[1]);
        input = delay[1] = stg[1];
        stg[2] = delay[2] + tune*((tanhstg[1] = tanh(input*THERMAL)) - tanhstg[2]);
        input = delay[2] = stg[2];
        stg[3] = delay[3] + tune*((tanhstg[2] =
                                   tanh(input*THERMAL)) - tanh(delay[3]*THERMAL));
        delay[3] = stg[3];
#else
        for (k = 1; k < 4; k++) {
          input = stg[k-1];
          stg[k] = delay[k]
            + tune*((tanhstg[k-1] = tanh(input*THERMAL))
                    - (k != 3 ? tanhstg[k] : tanh(delay[k]*THERMAL)));
          delay[k] = stg[k];
        }
#endif
        /* 1/2-sample delay for phase compensation  */
        delay[5] = (stg[3] + delay[4])*0.5;
        delay[4] = stg[3];
    }
    *out = (SPFLOAT) delay[5];
    return SP_OK;
}
