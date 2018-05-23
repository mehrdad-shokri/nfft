        nfft_f2
/*
 * Copyright (c) 2002, 2017 Jens Keiner, Stefan Kunis, Daniel Potts
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "config.h"

#ifdef HAVE_COMPLEX_H
#include <complex.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include "nfft3.h"
#include "infft.h"
#include "imex.h"

#define SETS_START 10 /* initial number of sets */
#define CMD_LEN_MAX 20 /* maximum length of command argument */

/* global flags */
#define FPT_MEX_FIRST_CALL (1U << 0)
static unsigned short gflags = FPT_MEX_FIRST_CALL;

static fpt_set** sets = NULL; /* sets */
static unsigned int sets_num_allocated = 0;
static int n_max = -1; /* maximum degree precomputed */
static char cmd[CMD_LEN_MAX];

static inline void check_nargs(const int nrhs, const int n, const char* errmsg)
{
  DM(if (nrhs != n)
    mexErrMsgTxt(errmsg);)
}

static inline int get_set(const mxArray *pm)
{
  int i = nfft_mex_get_int(pm,"Input argument set must be a scalar.");
  DM(if (i < 0 || i >= sets_num_allocated || sets[i] == NULL)
    mexErrMsgTxt("Set was not initialized or has already been finalized");)
  return i;
}

static inline int mkset()
{
  int i = 0;
  while (i < sets_num_allocated && sets[i] != NULL) i++;
  if (i == sets_num_allocated)
  {
    int l;

    if (sets_num_allocated >= INT_MAX - SETS_START - 1)
      mexErrMsgTxt("nfsoft: Too many sets already allocated.");

    fpt_set** sets_old = sets;
    sets = nfft_malloc((sets_num_allocated+SETS_START)*sizeof(fpt_set*));
    for (l = 0; l < sets_num_allocated; l++)
      sets[l] = sets_old[l];
    for (l = sets_num_allocated; l < sets_num_allocated+SETS_START; l++)
      sets[l] = NULL;
    if (sets_num_allocated > 0)
      nfft_free(sets_old);
    sets_num_allocated += SETS_START;
  }
  return i;
}

/* cleanup on mex function unload */
static void cleanup(void)
{
  int i;

  if (!(gflags & FPT_MEX_FIRST_CALL))
  {
    for (i = 0; i < sets_num_allocated; i++)
      if (sets[i])
      {
        fpt_finalize(*(sets[i]));
        sets[i] = NULL;
      }

    if (sets_num_allocated > 0)
    {
      nfft_free(sets);
      sets = NULL;
      sets_num_allocated = 0;
    }
    gflags |= FPT_MEX_FIRST_CALL;
    n_max = -1;
  }
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  if (gflags & FPT_MEX_FIRST_CALL)
  {
    /* Force Matlab to load libfftw3. There is at least one version of Matlab
     * which otherwise crashes upon invocation of this mex function. */
    mexEvalString("fft([1,2,3,4]);");

    nfft_mex_install_mem_hooks();

    mexAtExit(cleanup);
    gflags &= ~FPT_MEX_FIRST_CALL;
  }

  /* command string */
  DM(if (nrhs == 0)
    mexErrMsgTxt("At least one input required.");)

  DM(if (!mxIsChar(prhs[0]))
    mexErrMsgTxt("First argument must be a string.");)

  if (mxGetString(prhs[0], cmd, CMD_LEN_MAX))
    mexErrMsgTxt("Could not get command string.");

  if(strcmp(cmd,"init") == 0)
  {
    check_nargs(nrhs,3,"Wrong number of arguments for init.");
    int t = nfft_mex_get_int(prhs[1],"t must be scalar"); // exponent of the length of the transformation 2^t
    DM( if (t <= 0)
      mexErrMsgTxt("Input argument t must be positive.");)
    int flags_int = nfft_mex_get_int(prhs[2],"Input argument flags must be a scalar."); // flags
    DM( if (flags_int < 0)
      mexErrMsgTxt("Input argument flags must be non-negative.");)
    unsigned flags = (unsigned) flags_int;

    int i = mkset();
    fpt_set set = fpt_init(1, t, flags);
    sets[i] = &set;
    plhs[0] = mxCreateDoubleScalar((double)i);

  }else if(strcmp(cmd,"precompute") == 0)
  {
    check_nargs(nrhs,6,"Wrong number of arguments for precompute.");
    int i = get_set(prhs[1]);
    double *alpha = mxGetPr(prhs[2]);
    DM(if (!(mxIsDouble(prhs[2]) || (mxGetNumberOfDimensions(prhs[2]) > 2) || (mxGetN(prhs[2]) != 1)))
        mexErrMsgTxt("Input argument alpha must be a M x 1 array");)
    double *beta = mxGetPr(prhs[3]);
    DM(if (!(mxIsDouble(prhs[3]) || (mxGetNumberOfDimensions(prhs[3]) > 2) || (mxGetN(prhs[3]) != 1)))
        mexErrMsgTxt("Input argument beta must be a M x 1 array");)
    double *gam = mxGetPr(prhs[4]);
    DM(if (!(mxIsDouble(prhs[4]) || (mxGetNumberOfDimensions(prhs[4]) > 2) || (mxGetN(prhs[4]) != 1)))
        mexErrMsgTxt("Input argument gamma must be a M x 1 array");)
    DM(if ((mxGetM(prhs[2]) != mxGetM(prhs[3])) || (mxGetM(prhs[2]) != mxGetM(prhs[4])))
        mexErrMsgTxt("Input argument alpha, beta and gamma must have the same length");)
    int k_start = nfft_mex_get_int(prhs[5],"k_start must be scalar");

    fpt_precompute(*(sets[i]), 0, alpha, beta, gam, k_start, 1000.0);

  } else if(strcmp(cmd,"trafo") == 0)
  {
    check_nargs(nrhs,5,"Wrong number of arguments for trafo.");
    int i = get_set(prhs[1]);

    DM(if (!(mxIsDouble(prhs[2]) || mxIsComplex(prhs[2])) || (mxGetNumberOfDimensions(prhs[2]) > 2) || (mxGetN(prhs[2]) != 1))
      mexErrMsgTxt("Input argument a must be an M x 1 array");)
    double *a_real = mxGetPr(prhs[2]), *a_imag=0;
    if(mxIsComplex(prhs[2])) 
      a_imag = mxGetPi(prhs[2]);
    int N = mxGetM(prhs[2]);
    double _Complex *a = nfft_malloc(N*sizeof(double _Complex));

    for (int j = 0; j < N; j++)
    {
      if(a_imag)
        a[j] = a_real[j] + I*a_imag[j];
      else 
        a[j] = a_real[j];
    }

    int k_end = nfft_mex_get_int(prhs[3],"k_end must be scalar"); // k_end
    int flags_int = nfft_mex_get_int(prhs[4],"Input argument flags must be a scalar."); // flags
    DM( if (flags_int < 0)
      mexErrMsgTxt("Input argument flags must be non-negative.");)
    unsigned flags = (unsigned) flags_int;

    double _Complex *b = nfft_malloc(N*sizeof(double _Complex));
    fpt_trafo(*(sets[i]), 0, a, b, k_end, flags);

    plhs[0] = mxCreateDoubleMatrix(N, 1, mxCOMPLEX); // return value
    {
      double *br = mxGetPr(plhs[0]), *bi = mxGetPi(plhs[0]);
      for (int j = 0; j < N; j++)
      {
        br[j] = creal(b[j]);
        bi[j] = cimag(b[j]);
      }
    }
    nfft_free(a);
    nfft_free(b);
  
  } else if(strcmp(cmd,"transposed") == 0)
  {
    check_nargs(nrhs,5,"Wrong number of arguments for transposed.");
    int i = get_set(prhs[1]);

    DM(if (!(mxIsDouble(prhs[2]) || mxIsComplex(prhs[2])) || (mxGetNumberOfDimensions(prhs[2]) > 2) || (mxGetN(prhs[2]) != 1))
      mexErrMsgTxt("Input argument a must be an M x 1 array");)
    double *b_real = mxGetPr(prhs[2]), *b_imag=0;
    if(mxIsComplex(prhs[2])) 
      b_imag = mxGetPi(prhs[2]);
    int N = mxGetM(prhs[2]);
    double _Complex *b = nfft_malloc(N*sizeof(double _Complex));

    for (int j = 0; j < N; j++)
    {
      if(b_imag)
        b[j] = b_real[j] + I*b_imag[j];
      else 
        b[j] = b_real[j];
    }

    int k_end = nfft_mex_get_int(prhs[3],"k_end must be scalar"); // k_end
    int flags_int = nfft_mex_get_int(prhs[4],"Input argument flags must be a scalar."); // flags
    DM( if (flags_int < 0)
      mexErrMsgTxt("Input argument flags must be non-negative.");)
    unsigned flags = (unsigned) flags_int;

    double _Complex *a = nfft_malloc(N*sizeof(double _Complex));
    fpt_transposed(*(sets[i]), 0, a, b, k_end, flags);

    plhs[0] = mxCreateDoubleMatrix(N, 1, mxCOMPLEX); // return value
    {
      double *ar = mxGetPr(plhs[0]), *ai = mxGetPi(plhs[0]);
      for (int j = 0; j < N; j++)
      {
        ar[j] = creal(a[j]);
        ai[j] = cimag(a[j]);
      }
    }
    nfft_free(a);
    nfft_free(b);

  } else if(strcmp(cmd,"finalize") == 0)
  {
    check_nargs(nrhs,2,"Wrong number of arguments for finalize.");
    int i = get_set(prhs[1]);
    fpt_finalize(*(sets[i]));
    sets[i] = NULL;

  }else
    mexErrMsgTxt("fpt: Unknown command.\n");
}
