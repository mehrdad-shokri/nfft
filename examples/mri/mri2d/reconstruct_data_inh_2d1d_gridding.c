#include "options.h"
#include "util.h"
#include "nfft3.h"
#include "window_defines.h"
#include "math.h"



#define PHI_periodic(x) ((x>0.5)?(PHI(x-1.0,0)):((x<-0.5)?PHI(x+1.0,0):PHI(x,0)))

void nfft (char* filename,int N,int M,int iteration , int weight)
{
  int j,l;
  double weights;
  double time,min_time,max_time,min_inh,max_inh;
  double t,real,imag;
  double *w;
  nfft_plan my_plan, *ths;
  FILE* fp,*fw,*fout_real,*fout_imag,*finh,*ftime;
  int my_N[3],my_n[3];
  int flags = PRE_PHI_HUT| PRE_PSI |MALLOC_X| MALLOC_F_HAT|
                      MALLOC_F| FFTW_INIT| FFT_OUT_OF_PLACE|
                      FFTW_MEASURE| FFTW_DESTROY_INPUT;


  double Ts;
  double W;
  int N3;
  
  w = (double*) malloc(N*N*sizeof(double));

  ftime=fopen("readout_time.dat","r");
  finh=fopen("inh.dat","r");

  fprintf(stderr,"1\n");
  
  min_time=999999.0; max_time=-9999999.0;//Integer.maxValue!!!!
  for(j=0;j<M;j++)
  {
    fscanf(ftime,"%le ",&time);
    if(time<min_time)
      min_time = time;
    if(time>max_time)
      max_time = time;
  }

  fprintf(stderr,"2\n");
  
  fclose(ftime);
  
  Ts=min_time+max_time/2.0;


  min_inh=999999.0; max_inh=-9999999.0;//Integer.maxValue!!!!
  for(j=0;j<N*N;j++)
  {
    fscanf(finh,"%le ",&w[j]);
    if(w[j]<min_inh)
      min_inh = w[j];
    if(w[j]>max_inh)
      max_inh = w[j];
  }
  fclose(finh);

  W=2.0*MAX(fabs(min_inh),fabs(max_inh))*(1.2); //1.0+m/n!?!?!?!?!?
  N3=ceil(W*(max_time-min_time));

  fprintf(stderr,"3:  %i %e %e %e %e %e %e\n",N3,W,min_inh,max_inh,min_time,max_time,Ts);
  

  ths = (nfft_plan*) malloc(sizeof(nfft_plan));

  nfft_init_1d(ths,N3,1);
  N3=ths->n[0];

  my_N[0]=N;my_n[0]=next_power_of_2(N)+16;
  my_N[1]=N; my_n[1]=next_power_of_2(N)+16;
  my_N[2]=N3; my_n[2]=ths->n[0];
  
  /* initialise nfft */ 
  nfft_init_guru(&my_plan, 3, my_N, M, my_n, 6,flags,
                      FFTW_MEASURE| FFTW_DESTROY_INPUT);

  fp=fopen(filename,"r");
  ftime=fopen("readout_time.dat","r");

  for(j=0;j<my_plan.M_total;j++)
  {
    fscanf(fp,"%le %le %le %le",&my_plan.x[3*j+0],&my_plan.x[3*j+1],&real,&imag);
    my_plan.f[j]=real+I*imag;
    fscanf(ftime,"%le ",&my_plan.x[3*j+2]);

    my_plan.x[3*j+2] = (my_plan.x[3*j+2]-Ts)*W/N3;
    my_plan.f[j]*=1.0/PHI_HUT(N3*my_plan.x[3*j+2],0);
  }
  fclose(fp);
  fclose(ftime);

  
  if (weight)
  {
    fw=fopen("weights.dat","r");
    for(j=0;j<my_plan.M_total;j++)
    {
      fscanf(fw,"%le ",&weights);
      my_plan.f[j]*= weights;
    }
    fclose(fw);
  }

    
  if(my_plan.nfft_flags & PRE_PSI) {
    nfft_precompute_psi(&my_plan);
    if(my_plan.nfft_flags & PRE_FULL_PSI)
      nfft_precompute_full_psi(&my_plan);
  } 

  t=second();
  /* do the transform */ 
  nfft_adjoint(&my_plan);
  

    
  fout_real=fopen("output_real.dat","w");
  fout_imag=fopen("output_imag.dat","w");
  
  for (j=0;j<N*N;j++) {
    my_plan.f_hat[j*N3]*=PHI_periodic(w[j]/W+0.5);
    for(l=1;l<N3;l++) {
      my_plan.f_hat[j*N3]+=my_plan.f_hat[j*N3+l]*PHI_periodic(w[j]/W-((double)l)/((double)N3)+0.5);
    }

    /* Verschiebung wieder herausrechnen */
    my_plan.f_hat[j*N3]*=cexp(2.0*I*PI*Ts*w[j]);
    
    fprintf(fout_real,"%le ",creal(my_plan.f_hat[j*N3]));
    fprintf(fout_imag,"%le ",cimag(my_plan.f_hat[j*N3]));
  }
  
  t=second()-t;
  fprintf(stderr,"time: %e seconds\n",t);

  fclose(fout_real);
  fclose(fout_imag);
  nfft_finalize(&my_plan);
  nfft_finalize(ths);
  free(ths);
  free(w);
}


int main(int argc, char **argv)
{
  if (argc <= 3) {

    printf("usage: ./reconstruct FILENAME N M m PRE_FULL_PSI\n");
    return 1;
  }
  
  /* Allocate memory to hold every layer in memory after the
  2D-infft */

  nfft(argv[1],atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]));

  return 1;
}
