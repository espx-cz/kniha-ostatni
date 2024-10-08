/*
 *  Copyright (c) 2003-2004, Mark Borgerding. All rights reserved.
 *  This file is part of KISS FFT - https://github.com/mborgerding/kissfft
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See COPYING file for more information.
 */

#ifndef KISS_FFTND_H
#define KISS_FFTND_H

#include "../kissfft/kiss_fft.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct kiss_fftnd_state * kiss_fftnd_cfg;
    
kiss_fftnd_cfg  kiss_fftnd_alloc(const int *dims,int ndims,int inverse_fft,void*mem,size_t*lenmem);
void kiss_fftnd(kiss_fftnd_cfg  cfg,const kiss_fft_cpx *fin,kiss_fft_cpx *fout);

#ifdef __cplusplus
}
#endif
#endif
