/*
    Copyright 2013 Renwick James Hudspith

    This file (config_gluons.c) is part of GLU.

    GLU is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    GLU is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with GLU.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
   @file config_gluons.c
   @brief code to compute the configuration space gluonic correlation functions
 */

#include "Mainfile.h"

#include "cut_output.h"   // output file
#include "cut_routines.h" // momentum cutting?
#include "SM_wrap.h"      // do we want some smearing?
#include "geometry.h"     // general lexi site

/**
   @param LT
   @brief length of the temporal direction
 */
#define LT Latt.dims[ND-1]

// compute the log fields, overwrite the link matrices!
static int
Amu_fields( A , def )
     struct site *__restrict A ;
     const lie_field_def def ;
{
  int i ;
#pragma omp parallel for private(i)
  PFOR( i = 0 ; i < LVOLUME ; i++ ) { 
    struct spt_site temp ;
    int mu ;
    switch( def )
      {
      case LINEAR_DEF :
	for( mu = 0 ; mu < ND ; mu++ ) 
	  Hermitian_proj( temp.O[mu] , A[i].O[mu] ) ;
	break ;
      case LOG_DEF :
	for( mu = 0 ; mu < ND ; mu++ ) 
	  exact_log_slow( temp.O[mu] , A[i].O[mu] ) ;
	break ;
      }
    memcpy( &A[i] , &temp , sizeof( struct spt_site ) ) ;
  }
  return GLU_SUCCESS ;
}

// compute the gauge fixing accuracy
static void
check_landau_condition( const struct site *__restrict A )
{
  // compute the gauge fixing accuracy Tr( dA dA )
  double sum = 0.0 ;
  int i ;
#pragma omp parallel for private(i) reduction(+:sum)
  for( i = 0 ; i < LVOLUME ; i++ ) {
    GLU_complex dA[ NCNC ] = {} ;
    int mu ;
    for( mu = 0 ; mu < ND ; mu++ ) {
      const int back = gen_shift( i , -mu - 1 ) ; 
      int elem ;
      for( elem = 0 ; elem < NCNC ; elem++ ) {
	dA[ elem ] += A[i].O[mu][elem] - A[back].O[mu][elem] ;
      }
    }
    GLU_real tr ;
    trace_ab_herm( &tr , dA , dA ) ;
    sum = sum + (double)tr ;
  }
  printf( "[CUTS] GF accuracy :: %e \n" , 4.0 * sum / ( NC * LVOLUME ) ) ;
  return ;
}

// contract = Tr( A_\mu(x) A_\mu(y) ) ( for \mu < ND )
static double
contract_slices( A , x , t_ref )
     const struct site *__restrict A ;
     const int x ;
     const int t_ref ;
{
  GLU_real tr ;
  register double loc_tr = 0.0 ;
  int j ;
  for( j = 0 ; j < LCU ; j++ ) {
    const int y = j + t_ref ;
    #if ND == 4
    trace_ab_herm( &tr , A[x].O[0] , A[y].O[0] ) ;
    loc_tr += (double)tr ;
    trace_ab_herm( &tr , A[x].O[1] , A[y].O[1] ) ;
    loc_tr += (double)tr ;
    trace_ab_herm( &tr , A[x].O[2] , A[y].O[2] ) ;
    loc_tr += (double)tr ;
    //trace_ab_herm( &tr , A[x].O[3] , A[y].O[3] ) ;
    //loc_tr += (double)tr ;
    #else
    int mu ;
    for( mu = 0 ; mu < ND-1 ; mu++ ) {
      trace_ab_herm( &tr , A[x].O[mu] , A[y].O[mu] ) ;
      loc_tr += (double)tr ;
    }
    #endif
  }
  return loc_tr / (double)( LCU ) ;
}

static double
recurse_sum( const struct site *__restrict A ,
	     const int idx1 , const int idx2 ,
	     const int spacing ,
	     int vec[ ND ] , int mu , int *norm ) 
{
  if( mu > ND-2 ) { 
    const int xidx = idx1 + gen_site( vec ) ;
    *norm = *norm + 1 ;
    return contract_slices( A , xidx , idx2 ) ;
  }

  double sum = 0.0 ;
  int nu ;
  int loc_vec[ ND ] = {} ; 
  for( nu = 0 ; nu < ND ; nu++ ) {
    loc_vec[ nu ] = vec[ nu ] ;
  }
  for( nu = 0 ; nu < Latt.dims[mu] ; nu+=spacing ) {
    loc_vec[ mu ] = nu ;
    sum += recurse_sum( A , idx1 , idx2 , spacing , 
			loc_vec , mu + 1 , norm ) ;
  }
  return sum ;
}

/////////// spatial-spatial point sources over the whole lattice  ////////
static void
spatial_pointsource( A , gs , gsnorm )
     const struct site *__restrict A ;
     double *__restrict gs ;
     const double gsnorm ;
{
  // loop time 
  int t ;
#pragma omp parallel for private(t)
  for( t = 0 ; t < LT/2+1 ; t++ ) { // is exactly symmetric around LT/2
    // local trace
    register double loc_tr = 0.0 ;
    // temporary trace
    GLU_real tr ;
    // loop separations tau
    int tau , i , j ;
    for( tau = 0 ; tau < LT ; tau++ ) {
      const int idx1 = LCU * tau ;
      const int idx2 = LCU * ( ( tau + t ) % LT ) ;
      // loop spatial hypercube index i
      for( i = 0 ; i < LCU ; i++ ) {
	const int x = i + idx1 ;
	for( j = 0 ; j < LCU ; j++ ) {
	  const int y = j + idx2 ;
	  #if ND == 4
	  trace_ab_herm( &tr , A[x].O[0] , A[y].O[0] ) ;
	  loc_tr += (double)tr ;
	  trace_ab_herm( &tr , A[x].O[1] , A[y].O[1] ) ;
	  loc_tr += (double)tr ;
	  trace_ab_herm( &tr , A[x].O[2] , A[y].O[2] ) ;
	  loc_tr += (double)tr ;
	  //trace_ab_herm( &tr , A[x].O[3] , A[y].O[3] ) ;
	  //loc_tr += (double)tr ;
	  #else
	  int mu ;
	  for( mu = 0 ; mu < ND-1 ; mu++ ) {
	    trace_ab_herm( &tr , A[x].O[mu] , A[y].O[mu] ) ;
	    loc_tr += (double)tr ;
	  }
	  #endif
	}
      }
    }
    gs[t] = loc_tr * 2 / ( LCU * LCU * LT ) ;
  }
#pragma omp parallel for private(t)
  for( t = LT/2+1 ; t < LT ; t++ ) {
    // all to all is symmetric around LT/2
    gs[ t ] = gs[ (LT)-t ] ;
  }
  return ;
}

// compute the correlator from a specific point "x"
static void
spatial_correlator( A , gs , gsnorm , spacing )
     const struct site *__restrict A ;
     double *__restrict gs ;
     const double gsnorm ;
     const int spacing ;
{
  // this one is more effective
  if( spacing == 1 ) {
    return spatial_pointsource( A , gs , gsnorm ) ;
  }
  // this one is more general
  int t ;
#pragma omp parallel for private(t)
  for( t = 0 ; t < LT ; t++ ) { 
    // local trace
    double loc_tr = 0.0 ;
    int norm = 0 ;
    // loop separations tau
    int tau ;
    for( tau = 0 ; tau < LT ; tau++ ) {
      const int idx1 = LCU * tau ;
      const int idx2 = LCU * ( ( tau + t ) % LT ) ;
      int x[ ND ] = {} ;
      loc_tr += recurse_sum( A , idx1 , idx2 , spacing , 
			     x , 0 , &norm ) ;
    }
    gs[t] = loc_tr * 2.0 / (double)norm ;
  }
  return ;
}

// computes the configuration space propagators
int 
cuts_struct_configspace( struct site *__restrict A ,
			 const struct cut_info CUTINFO ,
			 const struct sm_info SMINFO )
{
  printf( "\n[CUTS] Computing the configuration-space gluon propagator\n" ) ;

  // smeared gluon field to extract the ground state?
  SM_wrap_struct( A , SMINFO ) ;

  // take the log
  if( Amu_fields( A , CUTINFO.definition ) != GLU_SUCCESS ) {
    printf( "[CUTS] Something wrong in Logging of the fields, check it out \n" ) ;
    return GLU_FAILURE ;
  }

  // check that we are in Landau gauge
  check_landau_condition( A ) ;

  // set up the outputs
  char *str = output_str_struct( CUTINFO ) ;  
  FILE *Ap = fopen( str , "wb" ) ; 

  // compute the temporal and spatial correlators
  double *gsp = malloc( Latt.dims[ ND-1 ] * sizeof( double ) ) ;

  // normalisations
  const double spat_norm = 1.0 / ( ( ND-1 ) * LCU ) ;

  // correlator between spacings to all, if CUTINFO.max_t == 1 this is all to all
  spatial_correlator( A , gsp , spat_norm , CUTINFO.max_t ) ;

  // write out the timeslice list ...
  int lt[ 1 ] = { LT } ;
  write_tslice_list( Ap , lt ) ;

  // and write the props ....
  write_g2_to_list( Ap , gsp , lt ) ;

  // tell us where we are writing out to
  printf( "[CUTS] data written to %s \n" , str ) ;

  // and free the gluon correlator
  free( gsp ) ;

  return GLU_SUCCESS ;
}

#undef LT // clean it up?