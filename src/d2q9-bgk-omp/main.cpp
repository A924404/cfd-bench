/*
 ** Code to implement a d2q9-bgk lattice boltzmann scheme.
 ** 'd2' inidates a 2-dimensional grid, and
 ** 'q9' indicates 9 velocities per grid cell.
 ** 'bgk' refers to the Bhatnagar-Gross-Krook collision step.
 **
 ** The 'speeds' in each cell are numbered as follows:
 **
 ** 6 2 5
 **  \|/
 ** 3-0-1
 **  /|\
 ** 7 4 8
 **
 ** A 2D grid:
 **
 **           cols
 **       --- --- ---
 **      | D | E | F |
 ** rows  --- --- ---
 **      | A | B | C |
 **       --- --- ---
 **
 ** 'unwrapped' in row major order to give a 1D array:
 **
 **  --- --- --- --- --- ---
 ** | A | B | C | D | E | F |
 **  --- --- --- --- --- ---
 **
 ** Grid indicies are:
 **
 **          ny
 **          ^       cols(ii)
 **          |  ----- ----- -----
 **          | | ... | ... | etc |
 **          |  ----- ----- -----
 ** rows(jj) | | 1,0 | 1,1 | 1,2 |
 **          |  ----- ----- -----
 **          | | 0,0 | 0,1 | 0,2 |
 **          |  ----- ----- -----
 **          ----------------------> nx
 **
 ** Note the names of the input parameter and obstacle files
 ** are passed on the command line, e.g.:
 **
 **   ./d2q9-bgk input.params obstacles.dat
 **
 ** Be sure to adjust the grid dimensions in the parameter file
 ** if you choose a different obstacle file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

#include <iostream>
#include <omp.h>

#define WARMUPS         1000
#define NSPEEDS         9
#define LOCALSIZEX      128
#define LOCALSIZEY      1

/* dump output files for verification */
#define FINALSTATEFILE  "final_state.dat"
#define AVVELSFILE      "av_vels.dat"

/* struct to hold the parameter values */
typedef struct
{
  int   nx;            /* no. of cells in x-direction */
  int   ny;            /* no. of cells in y-direction */
  int   maxIters;      /* no. of iterations */
  int   reynolds_dim;  /* dimension for Reynolds number */
  float density;       /* density per link */
  float accel;         /* density redistribution */
  float omega;         /* relaxation parameter */
} t_param;

/* struct to hold the 'speed' values */
typedef struct
{
  float speeds[NSPEEDS];
} t_speed;

/*
 ** function prototypes
 */

/* load params, allocate memory, load obstacles & initialise fluid particle densities */
int initialise(const char* paramfile, const char* obstaclefile,
    t_param* params, t_speed** cells_ptr, t_speed** tmp_cells_ptr,
    int** obstacles_ptr, float** av_vels_ptr);

/*
 ** The main calculation methods.
 ** timestep calls, in order, the functions:
 ** accelerate_flow(), propagate(), rebound() & collision()
 */
int write_values(const t_param params, t_speed* cells, int* obstacles, float* av_vels);

/* finalise, including freeing up allocated memory */
int finalise(t_speed* cells_ptr, t_speed* tmp_cells_ptr,
    int* obstacles_ptr, float* av_vels_ptr);

/* Sum all the densities in the grid.
 ** The total should remain constant from one timestep to the next. */
float total_density(const t_param params, t_speed* cells);

/* compute average velocity */
float av_velocity(const t_param params, t_speed* cells, int* obstacles);

/* calculate Reynolds number */
float calc_reynolds(const t_param params, t_speed* cells, int* obstacles);

/* utility functions */
void die(const char* message, const int line, const char* file);
void usage(const char* exe);


#pragma omp declare target
bool 
isGreater(const float x, const float y) 
{
  return x > y ? 1 : 0;
}
#pragma omp end declare target

int main(int argc, char* argv[])
{
  char*    paramfile = NULL;    /* input parameter file */
  char*    obstaclefile = NULL; /* input obstacle file */
  t_param  params;              /* struct to hold parameter values */
  t_speed* cells     = NULL;    /* grid containing fluid densities */
  t_speed* tmp_cells = NULL;    /* scratch space */
  int*     obstacles = NULL;/* grid indicating which cells are blocked */
  float*   av_vels   = NULL;    /* a record of the av. velocity computed for each timestep */
  struct timeval timstr;        /* structure to hold elapsed time */
  double tic, toc;              /* floating point numbers to calculate elapsed wallclock time */

  /* parse the command line */
  if (argc != 3)
  {
    usage(argv[0]);
  }
  else
  {
    paramfile = argv[1];
    obstaclefile = argv[2];
  }

  /* initialise our data structures and load values from file */
  initialise(paramfile, obstaclefile, &params, &cells, 
      &tmp_cells, &obstacles, &av_vels);

  // declare host arrays
  unsigned int Ny = params.ny;
  unsigned int Nx = params.nx;
  unsigned int MaxIters = params.maxIters;

  float *speeds0 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *speeds1 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *speeds2 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *speeds3 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *speeds4 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *speeds5 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *speeds6 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *speeds7 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *speeds8 = (float*) malloc (sizeof(float) * Ny*Nx);

  float *tmp_speeds0 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *tmp_speeds1 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *tmp_speeds2 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *tmp_speeds3 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *tmp_speeds4 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *tmp_speeds5 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *tmp_speeds6 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *tmp_speeds7 = (float*) malloc (sizeof(float) * Ny*Nx);
  float *tmp_speeds8 = (float*) malloc (sizeof(float) * Ny*Nx);

  float *tot_up = (float*) malloc (sizeof(float) * (Ny/LOCALSIZEY) * (Nx/LOCALSIZEX) * MaxIters);
  int *tot_cellsp = (int*) malloc (sizeof(int) * (Ny/LOCALSIZEY) * (Nx/LOCALSIZEX) * MaxIters);

  // Init arrays
  /* loop over _all_ cells */
  for (int jj = 0; jj < Ny; jj++)
  {
    for (int ii = 0; ii < Nx; ii++)
    {
      speeds0[ii + jj*Nx] = cells[ii + jj*Nx].speeds[0];
      speeds1[ii + jj*Nx] = cells[ii + jj*Nx].speeds[1];
      speeds2[ii + jj*Nx] = cells[ii + jj*Nx].speeds[2];
      speeds3[ii + jj*Nx] = cells[ii + jj*Nx].speeds[3];
      speeds4[ii + jj*Nx] = cells[ii + jj*Nx].speeds[4];
      speeds5[ii + jj*Nx] = cells[ii + jj*Nx].speeds[5];
      speeds6[ii + jj*Nx] = cells[ii + jj*Nx].speeds[6];
      speeds7[ii + jj*Nx] = cells[ii + jj*Nx].speeds[7];
      speeds8[ii + jj*Nx] = cells[ii + jj*Nx].speeds[8];
    }
  }

  //parameters for kernel
  float omega = params.omega;
  float densityaccel = params.density*params.accel;

  int teams = Nx / LOCALSIZEX * Ny / LOCALSIZEY;
  int threads = LOCALSIZEX * LOCALSIZEY;

#pragma omp target data map(tofrom: speeds0[0:Ny*Nx], \
                                    speeds1[0:Ny*Nx], \
                                    speeds2[0:Ny*Nx], \
                                    speeds3[0:Ny*Nx], \
                                    speeds4[0:Ny*Nx], \
                                    speeds5[0:Ny*Nx], \
                                    speeds6[0:Ny*Nx], \
                                    speeds7[0:Ny*Nx], \
                                    speeds8[0:Ny*Nx], \
                                    obstacles[0:Ny*Nx]), \
                        map(alloc: tmp_speeds0[0:Ny*Nx], \
                                   tmp_speeds1[0:Ny*Nx], \
                                   tmp_speeds2[0:Ny*Nx], \
                                   tmp_speeds3[0:Ny*Nx], \
                                   tmp_speeds4[0:Ny*Nx], \
                                   tmp_speeds5[0:Ny*Nx], \
                                   tmp_speeds6[0:Ny*Nx], \
                                   tmp_speeds7[0:Ny*Nx], \
                                   tmp_speeds8[0:Ny*Nx]), \
                         map(from: tot_up[0:(Ny/LOCALSIZEY) * (Nx/LOCALSIZEX) * MaxIters], \
                                   tot_cellsp[0:(Ny/LOCALSIZEY) * (Nx/LOCALSIZEX) * MaxIters])
  {

  //start timer
  gettimeofday(&timstr, NULL);
  tic = timstr.tv_sec * 1e6 + timstr.tv_usec;

  for (int tt = 0; tt < MaxIters; tt++) {
    if (tt == WARMUPS - 1) {
      //start timer after warmup
      gettimeofday(&timstr, NULL);
      tic = timstr.tv_sec * 1e6 + timstr.tv_usec;
    }
    #pragma omp target teams num_teams(teams) thread_limit(threads)
    {
      float local_sum[LOCALSIZEX*LOCALSIZEY];
      float local_sum2[LOCALSIZEX*LOCALSIZEY];
      #pragma omp parallel 
      {
        const int lid = omp_get_thread_num();  
        const int tid = omp_get_team_num();  
        const int dim = omp_get_num_threads();  
        const int gid = dim * tid + lid;

        /* get column and row indices */
        const int ii = gid % Nx;
        const int jj = gid / Nx;

        const float c_sq_inv = 3.f;
        const float c_sq = 1.f/c_sq_inv; /* square of speed of sound */
        const float temp1 = 4.5f;
        const float w1 = 1.f/9.f;
        const float w0 = 4.f * w1;  /* weighting factor */
        const float w2 = 1.f/36.f; /* weighting factor */
        const float w11 = densityaccel * w1;
        const float w21 = densityaccel * w2;

        /* determine indices of axis-direction neighbours
         ** respecting periodic boundary conditions (wrap around) */
        const int y_n = (jj + 1) % Ny;
        const int x_e = (ii + 1) % Nx;
        const int y_s = (jj == 0) ? (jj + Ny - 1) : (jj - 1);
        const int x_w = (ii == 0) ? (ii + Nx - 1) : (ii - 1);

        /* propagate densities from neighbouring cells, following
         ** appropriate directions of travel and writing into
         ** scratch space grid */

        float tmp_s0 = speeds0[ii + jj*Nx];
        float tmp_s1 = (jj == Ny-2 && (!obstacles[x_w + jj*Nx] && isGreater((speeds3[x_w + jj*Nx] - w11) , 0.f) && isGreater((speeds6[x_w + jj*Nx] - w21) , 0.f) && isGreater((speeds7[x_w + jj*Nx] - w21) , 0.f))) ? speeds1[x_w + jj*Nx]+w11 : speeds1[x_w + jj*Nx];
        float tmp_s2 = speeds2[ii + y_s*Nx];
        float tmp_s3 = (jj == Ny-2 && (!obstacles[x_e + jj*Nx] && isGreater((speeds3[x_e + jj*Nx] - w11) , 0.f) && isGreater((speeds6[x_e + jj*Nx] - w21) , 0.f) && isGreater((speeds7[x_e + jj*Nx] - w21) , 0.f))) ? speeds3[x_e + jj*Nx]-w11 : speeds3[x_e + jj*Nx];
        float tmp_s4 = speeds4[ii + y_n*Nx];
        float tmp_s5 = (y_s == Ny-2 && (!obstacles[x_w + y_s*Nx] && isGreater((speeds3[x_w + y_s*Nx] - w11) , 0.f) && isGreater((speeds6[x_w + y_s*Nx] - w21) , 0.f) && isGreater((speeds7[x_w + y_s*Nx] - w21) , 0.f))) ? speeds5[x_w + y_s*Nx]+w21 : speeds5[x_w + y_s*Nx];
        float tmp_s6 = (y_s == Ny-2 && (!obstacles[x_e + y_s*Nx] && isGreater((speeds3[x_e + y_s*Nx] - w11) , 0.f) && isGreater((speeds6[x_e + y_s*Nx] - w21) , 0.f) && isGreater((speeds7[x_e + y_s*Nx] - w21) , 0.f))) ? speeds6[x_e + y_s*Nx]-w21 : speeds6[x_e + y_s*Nx];
        float tmp_s7 = (y_n == Ny-2 && (!obstacles[x_e + y_n*Nx] && isGreater((speeds3[x_e + y_n*Nx] - w11) , 0.f) && isGreater((speeds6[x_e + y_n*Nx] - w21) , 0.f) && isGreater((speeds7[x_e + y_n*Nx] - w21) , 0.f))) ? speeds7[x_e + y_n*Nx]-w21 : speeds7[x_e + y_n*Nx];
        float tmp_s8 = (y_n == Ny-2 && (!obstacles[x_w + y_n*Nx] && isGreater((speeds3[x_w + y_n*Nx] - w11) , 0.f) && isGreater((speeds6[x_w + y_n*Nx] - w21) , 0.f) && isGreater((speeds7[x_w + y_n*Nx] - w21) , 0.f))) ? speeds8[x_w + y_n*Nx]+w21 : speeds8[x_w + y_n*Nx];

        /* compute local density total */
        float local_density = tmp_s0 + tmp_s1 + tmp_s2 + tmp_s3 + tmp_s4  + tmp_s5  + tmp_s6  + tmp_s7  + tmp_s8;
        const float local_density_recip = 1.f/(local_density);
        /* compute x velocity component */
        float u_x = (tmp_s1
            + tmp_s5
            + tmp_s8
            - tmp_s3
            - tmp_s6
            - tmp_s7)
          * local_density_recip;
        /* compute y velocity component */
        float u_y = (tmp_s2
            + tmp_s5
            + tmp_s6
            - tmp_s4
            - tmp_s8
            - tmp_s7)
          * local_density_recip;

        /* velocity squared */
        const float temp2 = - (u_x * u_x + u_y * u_y)/(2.f * c_sq);

        /* equilibrium densities */
        float d_equ[NSPEEDS];
        /* zero velocity density: weight w0 */
        d_equ[0] = w0 * local_density
          * (1.f + temp2);
        /* axis speeds: weight w1 */
        d_equ[1] = w1 * local_density * (1.f + u_x * c_sq_inv
            + (u_x * u_x) * temp1
            + temp2);
        d_equ[2] = w1 * local_density * (1.f + u_y * c_sq_inv
            + (u_y * u_y) * temp1
            + temp2);
        d_equ[3] = w1 * local_density * (1.f - u_x * c_sq_inv
            + (u_x * u_x) * temp1
            + temp2);
        d_equ[4] = w1 * local_density * (1.f - u_y * c_sq_inv
            + (u_y * u_y) * temp1
            + temp2);
        /* diagonal speeds: weight w2 */
        d_equ[5] = w2 * local_density * (1.f + (u_x + u_y) * c_sq_inv
            + ((u_x + u_y) * (u_x + u_y)) * temp1
            + temp2);
        d_equ[6] = w2 * local_density * (1.f + (-u_x + u_y) * c_sq_inv
            + ((-u_x + u_y) * (-u_x + u_y)) * temp1
            + temp2);
        d_equ[7] = w2 * local_density * (1.f + (-u_x - u_y) * c_sq_inv
            + ((-u_x - u_y) * (-u_x - u_y)) * temp1
            + temp2);
        d_equ[8] = w2 * local_density * (1.f + (u_x - u_y) * c_sq_inv
            + ((u_x - u_y) * (u_x - u_y)) * temp1
            + temp2);

        float tmp;
        int expression = obstacles[ii + jj*Nx];
        tmp_s0 = expression ? tmp_s0 : (tmp_s0 + omega * (d_equ[0] - tmp_s0));
        tmp = tmp_s1;
        tmp_s1 = expression ? tmp_s3 : (tmp_s1 + omega * (d_equ[1] - tmp_s1));
        tmp_s3 = expression ? tmp : (tmp_s3 + omega * (d_equ[3] - tmp_s3));
        tmp = tmp_s2;
        tmp_s2 = expression ? tmp_s4 : (tmp_s2 + omega * (d_equ[2] - tmp_s2));
        tmp_s4 = expression ? tmp : (tmp_s4 + omega * (d_equ[4] - tmp_s4));
        tmp = tmp_s5;
        tmp_s5 = expression ? tmp_s7 : (tmp_s5 + omega * (d_equ[5] - tmp_s5));
        tmp_s7 = expression ? tmp : (tmp_s7 + omega * (d_equ[7] - tmp_s7));
        tmp = tmp_s6;
        tmp_s6 = expression ? tmp_s8 : (tmp_s6 + omega * (d_equ[6] - tmp_s6));
        tmp_s8 = expression ? tmp : (tmp_s8 + omega * (d_equ[8] - tmp_s8));

        /* local density total */
        local_density = 1.f/((tmp_s0 + tmp_s1 + tmp_s2 + tmp_s3 + tmp_s4 + tmp_s5 + tmp_s6 + tmp_s7 + tmp_s8));

        /* x-component of velocity */
        u_x = (tmp_s1
            + tmp_s5
            + tmp_s8
            - tmp_s3
            - tmp_s6
            - tmp_s7)
          * local_density;
        /* compute y velocity component */
        u_y = (tmp_s2
            + tmp_s5
            + tmp_s6
            - tmp_s4
            - tmp_s7
            - tmp_s8)
          * local_density;

        tmp_speeds0[ii + jj*Nx] = tmp_s0;
        tmp_speeds1[ii + jj*Nx] = tmp_s1;
        tmp_speeds2[ii + jj*Nx] = tmp_s2;
        tmp_speeds3[ii + jj*Nx] = tmp_s3;
        tmp_speeds4[ii + jj*Nx] = tmp_s4;
        tmp_speeds5[ii + jj*Nx] = tmp_s5;
        tmp_speeds6[ii + jj*Nx] = tmp_s6;
        tmp_speeds7[ii + jj*Nx] = tmp_s7;
        tmp_speeds8[ii + jj*Nx] = tmp_s8;


        int local_idi = lid % LOCALSIZEX; 
        int local_idj = lid / LOCALSIZEX;
        int local_sizei = LOCALSIZEX;
        int local_sizej = LOCALSIZEY;
        /* accumulate the norm of x- and y- velocity components */
        local_sum[local_idi + local_idj*local_sizei] = (obstacles[ii + jj*Nx]) ? 0 : hypotf(u_x,u_y);
        /* increase counter of inspected cells */
        local_sum2[local_idi + local_idj*local_sizei] = (obstacles[ii + jj*Nx]) ? 0 : 1 ;

#pragma omp barrier

        int group_id = tid % (Nx/LOCALSIZEX);
        int group_id2 = tid / (Nx/LOCALSIZEX);
        int group_size = (Nx/LOCALSIZEX);
        int group_size2 = (Ny/LOCALSIZEY);
        if(local_idi == 0 && local_idj == 0){
          float sum = 0.0f;
          int sum2 = 0;
          for(int i = 0; i<local_sizei*local_sizej; i++){
            sum += local_sum[i];
            sum2 += local_sum2[i];
          }
          tot_up[group_id+group_id2*group_size+tt*group_size*group_size2] = sum;
          tot_cellsp[group_id+group_id2*group_size+tt*group_size*group_size2] = sum2;
        }
      }
    }

    float* speed_tmp = speeds0;
    speeds0 = tmp_speeds0;
    tmp_speeds0 = speed_tmp;

    speed_tmp = speeds1;
    speeds1 = tmp_speeds1;
    tmp_speeds1 = speed_tmp;

    speed_tmp = speeds2;
    speeds2 = tmp_speeds2;
    tmp_speeds2 = speed_tmp;

    speed_tmp = speeds3;
    speeds3 = tmp_speeds3;
    tmp_speeds3 = speed_tmp;

    speed_tmp = speeds4;
    speeds4 = tmp_speeds4;
    tmp_speeds4 = speed_tmp;

    speed_tmp = speeds5;
    speeds5 = tmp_speeds5;
    tmp_speeds5 = speed_tmp;

    speed_tmp = speeds6;
    speeds6 = tmp_speeds6;
    tmp_speeds6 = speed_tmp;

    speed_tmp = speeds7;
    speeds7 = tmp_speeds7;
    tmp_speeds7 = speed_tmp;

    speed_tmp = speeds8;
    speeds8 = tmp_speeds8;
    tmp_speeds8 = speed_tmp;
  }

  gettimeofday(&timstr, NULL);
  toc = timstr.tv_sec * 1e6 + timstr.tv_usec;
  printf("After warmup for %d iterations, ", WARMUPS);
  printf("average kernel execution time over %d iterations:\t\t\t%.6lf (us)\n",
         MaxIters - WARMUPS, (toc - tic) / (MaxIters - WARMUPS));
  printf("Total kernel execution time over: %.6lf (s)\n", (toc - tic) * 1e-6);

  } // omp target 

  float tot_u = 0;
  int tot_cells = 0;
  for (int tt = 0; tt < MaxIters; tt++){
    tot_u = 0;
    tot_cells = 0;
    for(int i = 0; i < Nx/LOCALSIZEX*Ny/LOCALSIZEY; i++){
      tot_u += tot_up[i+tt*Nx/LOCALSIZEX*Ny/LOCALSIZEY];
      tot_cells += tot_cellsp[i+tt*Nx/LOCALSIZEX*Ny/LOCALSIZEY];
      //printf("%d %f %d\n", i, tot_u, tot_cells);
    }
    av_vels[tt] = tot_u/tot_cells;
  }

  // put answers back into cells
  for (int jj = 0; jj < Ny; jj++)
  {
    for (int ii = 0; ii < Nx; ii++)
    {
      cells[ii + jj*Nx].speeds[0] = speeds0[ii + jj*Nx];
      cells[ii + jj*Nx].speeds[1] = speeds1[ii + jj*Nx];
      cells[ii + jj*Nx].speeds[2] = speeds2[ii + jj*Nx];
      cells[ii + jj*Nx].speeds[3] = speeds3[ii + jj*Nx];
      cells[ii + jj*Nx].speeds[4] = speeds4[ii + jj*Nx];
      cells[ii + jj*Nx].speeds[5] = speeds5[ii + jj*Nx];
      cells[ii + jj*Nx].speeds[6] = speeds6[ii + jj*Nx];
      cells[ii + jj*Nx].speeds[7] = speeds7[ii + jj*Nx];
      cells[ii + jj*Nx].speeds[8] = speeds8[ii + jj*Nx];
    }
  }

  /* write final values and free memory */
  printf("==done==\n");
  printf("Reynolds number:\t\t%.12E\n", calc_reynolds(params, cells, obstacles));
  write_values(params, cells, obstacles, av_vels);
  finalise(cells, tmp_cells, obstacles, av_vels);

  free(speeds0);
  free(speeds1);
  free(speeds2);
  free(speeds3);
  free(speeds4);
  free(speeds5);
  free(speeds6);
  free(speeds7);
  free(speeds8);
  free(tot_up);
  free(tot_cellsp);

  return EXIT_SUCCESS;
}

float av_velocity(const t_param params, t_speed* cells, int* obstacles)
{
  int    tot_cells = 0;  /* no. of cells used in calculation */
  float tot_u;          /* accumulated magnitudes of velocity for each cell */

  /* initialise */
  tot_u = 0.f;

  /* loop over all non-blocked cells */
  for (int jj = 0; jj < params.ny; jj++)
  {
    for (int ii = 0; ii < params.nx; ii++)
    {
      /* ignore occupied cells */
      if (!obstacles[ii + jj*params.nx])
      {
        /* local density total */
        float local_density = 0.f;

        for (int kk = 0; kk < NSPEEDS; kk++)
        {
          local_density += cells[ii + jj*params.nx].speeds[kk];
        }

        /* x-component of velocity */
        float u_x = (cells[ii + jj*params.nx].speeds[1]
            + cells[ii + jj*params.nx].speeds[5]
            + cells[ii + jj*params.nx].speeds[8]
            - (cells[ii + jj*params.nx].speeds[3]
              + cells[ii + jj*params.nx].speeds[6]
              + cells[ii + jj*params.nx].speeds[7]))
          / local_density;
        /* compute y velocity component */
        float u_y = (cells[ii + jj*params.nx].speeds[2]
            + cells[ii + jj*params.nx].speeds[5]
            + cells[ii + jj*params.nx].speeds[6]
            - (cells[ii + jj*params.nx].speeds[4]
              + cells[ii + jj*params.nx].speeds[7]
              + cells[ii + jj*params.nx].speeds[8]))
          / local_density;
        /* accumulate the norm of x- and y- velocity components */
        tot_u += sqrtf((u_x * u_x) + (u_y * u_y));
        /* increase counter of inspected cells */
        ++tot_cells;
      }
    }
  }

  return tot_u / (float)tot_cells;
}

int initialise(const char* paramfile, const char* obstaclefile,
    t_param* params, t_speed** cells_ptr, t_speed** tmp_cells_ptr,
    int** obstacles_ptr, float** av_vels_ptr){
  char   message[1024];  /* message buffer */
  FILE*  fp;             /* file pointer */
  int    xx, yy;         /* generic array indices */
  int    blocked;        /* indicates whether a cell is blocked by an obstacle */
  int    retval;         /* to hold return value for checking */

  /* open the parameter file */
  fp = fopen(paramfile, "r");

  if (fp == NULL)
  {
    sprintf(message, "could not open input parameter file: %s", paramfile);
    die(message, __LINE__, __FILE__);
  }
  /* read in the parameter values */
  retval = fscanf(fp, "%d\n", &(params->nx));

  if (retval != 1) die("could not read param file: nx", __LINE__, __FILE__);

  retval = fscanf(fp, "%d\n", &(params->ny));

  if (retval != 1) die("could not read param file: ny", __LINE__, __FILE__);

  retval = fscanf(fp, "%d\n", &(params->maxIters));

  if (retval != 1) die("could not read param file: maxIters", __LINE__, __FILE__);

  retval = fscanf(fp, "%d\n", &(params->reynolds_dim));

  if (retval != 1) die("could not read param file: reynolds_dim", __LINE__, __FILE__);

  retval = fscanf(fp, "%f\n", &(params->density));

  if (retval != 1) die("could not read param file: density", __LINE__, __FILE__);

  retval = fscanf(fp, "%f\n", &(params->accel));

  if (retval != 1) die("could not read param file: accel", __LINE__, __FILE__);

  retval = fscanf(fp, "%f\n", &(params->omega));

  if (retval != 1) die("could not read param file: omega", __LINE__, __FILE__);

  /* and close up the file */
  fclose(fp);

  /*
   ** allocate memory.
   **
   ** remember C is pass-by-value, so we need to
   ** pass pointers into the initialise function.
   **
   ** nb we are allocating a 1D array, so that the
   ** memory will be contiguous.  We still want to
   ** index this memory as if it were a (row major
   ** ordered) 2D array, however.  We will perform
   ** some arithmetic using the row and column
   ** coordinates, inside the square brackets, when
   ** we want to access elements of this array.
   **
   ** note also that we are using a structure to
   ** hold an array of 'speeds'.  We will allocate
   ** a 1D array of these structs.
   */

  /* main grid */
  *cells_ptr = (t_speed*)malloc(sizeof(t_speed) * (params->ny * params->nx));

  if (*cells_ptr == NULL) die("cannot allocate memory for cells", __LINE__, __FILE__);

  /* 'helper' grid, used as scratch space */
  *tmp_cells_ptr = (t_speed*)malloc(sizeof(t_speed) * (params->ny * params->nx));

  if (*tmp_cells_ptr == NULL) die("cannot allocate memory for tmp_cells", __LINE__, __FILE__);

  /* the map of obstacles */
  *obstacles_ptr = (int*) malloc (sizeof(int) * params->ny * params->nx);

  if (*obstacles_ptr == NULL) die("cannot allocate column memory for obstacles", __LINE__, __FILE__);

  /* initialise densities */
  float w0 = params->density * 4.f / 9.f;
  float w1 = params->density      / 9.f;
  float w2 = params->density      / 36.f;

  for (int jj = 0; jj < params->ny; jj++)
  {
    for (int ii = 0; ii < params->nx; ii++)
    {
      /* centre */
      (*cells_ptr)[ii + jj*params->nx].speeds[0] = w0;
      /* axis directions */
      (*cells_ptr)[ii + jj*params->nx].speeds[1] = w1;
      (*cells_ptr)[ii + jj*params->nx].speeds[2] = w1;
      (*cells_ptr)[ii + jj*params->nx].speeds[3] = w1;
      (*cells_ptr)[ii + jj*params->nx].speeds[4] = w1;
      /* diagonals */
      (*cells_ptr)[ii + jj*params->nx].speeds[5] = w2;
      (*cells_ptr)[ii + jj*params->nx].speeds[6] = w2;
      (*cells_ptr)[ii + jj*params->nx].speeds[7] = w2;
      (*cells_ptr)[ii + jj*params->nx].speeds[8] = w2;
    }
  }

  /* first set all cells in obstacle array to zero */
  for (int jj = 0; jj < params->ny; jj++)
  {
    for (int ii = 0; ii < params->nx; ii++)
    {
      (*obstacles_ptr)[ii + jj*params->nx] = 0;
    }
  }

  /* open the obstacle data file */
  fp = fopen(obstaclefile, "r");

  if (fp == NULL)
  {
    sprintf(message, "could not open input obstacles file: %s", obstaclefile);
    die(message, __LINE__, __FILE__);
  }

  /* read-in the blocked cells list */
  while ((retval = fscanf(fp, "%d %d %d\n", &xx, &yy, &blocked)) != EOF)
  {
    /* some checks */
    if (retval != 3) die("expected 3 values per line in obstacle file", __LINE__, __FILE__);

    if (xx < 0 || xx > params->nx - 1) die("obstacle x-coord out of range", __LINE__, __FILE__);

    if (yy < 0 || yy > params->ny - 1) die("obstacle y-coord out of range", __LINE__, __FILE__);

    if (blocked != 1) die("obstacle blocked value should be 1", __LINE__, __FILE__);

    /* assign to array */
    (*obstacles_ptr)[xx + yy*params->nx] = blocked;
  }

  /* and close the file */
  fclose(fp);

  /*
   ** allocate space to hold a record of the avarage velocities computed
   ** at each timestep
   */
  *av_vels_ptr = (float*)malloc(sizeof(float) * params->maxIters);

  return EXIT_SUCCESS;
}

int finalise(t_speed* cells_ptr, t_speed* tmp_cells_ptr,
    int* obstacles_ptr, float* av_vels_ptr)
{
  /*
   ** free up allocated memory
   */
  free(cells_ptr);
  free(tmp_cells_ptr);
  free(obstacles_ptr);
  free(av_vels_ptr);

  return EXIT_SUCCESS;
}


float calc_reynolds(const t_param params, t_speed* cells, int* obstacles)
{
  const float viscosity = 1.f / 6.f * (2.f / params.omega - 1.f);

  return av_velocity(params, cells, obstacles) * params.reynolds_dim / viscosity;
}

float total_density(const t_param params, t_speed* cells)
{
  float total = 0.f;  /* accumulator */

  for (int jj = 0; jj < params.ny; jj++)
  {
    for (int ii = 0; ii < params.nx; ii++)
    {
      for (int kk = 0; kk < NSPEEDS; kk++)
      {
        total += cells[ii + jj*params.nx].speeds[kk];
      }
    }
  }

  return total;
}

int write_values(const t_param params, t_speed* cells, int* obstacles, float* av_vels)
{
  FILE* fp;                     /* file pointer */
  const float c_sq = 1.f / 3.f; /* sq. of speed of sound */
  float local_density;         /* per grid cell sum of densities */
  float pressure;              /* fluid pressure in grid cell */
  float u_x;                   /* x-component of velocity in grid cell */
  float u_y;                   /* y-component of velocity in grid cell */
  float u;                     /* norm--root of summed squares--of u_x and u_y */

  fp = fopen(FINALSTATEFILE, "w");

  if (fp == NULL)
  {
    die("could not open file output file", __LINE__, __FILE__);
  }

  for (int jj = 0; jj < params.ny; jj++)
  {
    for (int ii = 0; ii < params.nx; ii++)
    {
      /* an occupied cell */
      if (obstacles[ii + jj*params.nx])
      {
        u_x = u_y = u = 0.f;
        pressure = params.density * c_sq;
      }
      /* no obstacle */
      else
      {
        local_density = 0.f;

        for (int kk = 0; kk < NSPEEDS; kk++)
        {
          local_density += cells[ii + jj*params.nx].speeds[kk];
        }

        /* compute x velocity component */
        u_x = (cells[ii + jj*params.nx].speeds[1]
            + cells[ii + jj*params.nx].speeds[5]
            + cells[ii + jj*params.nx].speeds[8]
            - (cells[ii + jj*params.nx].speeds[3]
              + cells[ii + jj*params.nx].speeds[6]
              + cells[ii + jj*params.nx].speeds[7]))
          / local_density;
        /* compute y velocity component */
        u_y = (cells[ii + jj*params.nx].speeds[2]
            + cells[ii + jj*params.nx].speeds[5]
            + cells[ii + jj*params.nx].speeds[6]
            - (cells[ii + jj*params.nx].speeds[4]
              + cells[ii + jj*params.nx].speeds[7]
              + cells[ii + jj*params.nx].speeds[8]))
          / local_density;
        /* compute norm of velocity */
        u = sqrtf((u_x * u_x) + (u_y * u_y));
        /* compute pressure */
        pressure = local_density * c_sq;
      }

      /* write to file */
      fprintf(fp, "%d %d %.12E %.12E %.12E %.12E %d\n", ii, jj, u_x, u_y, u, pressure, obstacles[ii * params.nx + jj]);
    }
  }

  fclose(fp);

  fp = fopen(AVVELSFILE, "w");

  if (fp == NULL)
  {
    die("could not open file output file", __LINE__, __FILE__);
  }

  for (int ii = 0; ii < params.maxIters; ii++)
  {
    fprintf(fp, "%d:\t%.12E\n", ii, av_vels[ii]);
  }

  fclose(fp);

  return EXIT_SUCCESS;
}

void die(const char* message, const int line, const char* file)
{
  fprintf(stderr, "Error at line %d of file %s:\n", line, file);
  fprintf(stderr, "%s\n", message);
  fflush(stderr);
  exit(EXIT_FAILURE);
}

void usage(const char* exe)
{
  fprintf(stderr, "Usage: %s <paramfile> <obstaclefile>\n", exe);
  exit(EXIT_FAILURE);
}
