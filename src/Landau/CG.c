/*
    Copyright 2013 Renwick James Hudspith

    This file (CG.c) is part of GLU.

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
   @file CG.c
   @brief common code for the the CG routines
 */

#include "Mainfile.h"    // general definitions

#include "GLU_splines.h" // GLUbic spline interpolation code
#include "gramschmidt.h" // for reunitarisation
#include "gtrans.h"      // gauge transformations

// this is the point where the gram-schmidt loses out
// to the n-ape det-rescaled projection
#if NC > 18
  #include "taylor_logs.h"
#endif

// some small memory for the stabler average
static double *traces , *divs ;

#ifdef exp_a2_approx
// expansion :: 1 + i du Au - 0.5 * ( du Au ) ^ 2, must have A2_APPROX_EXPAND defined
static void
a2_approx_expand( GLU_complex dA[ NCNC ] )
{
#if NC==3
  const double QQ0 = 1.0 - 0.5 * ( cimag( dA[0] ) * cimag( dA[0] ) +		\
				   creal( dA[1] ) * creal( dA[1] ) + cimag( dA[1] ) * cimag( dA[1] ) +
				   creal( dA[2] ) * creal( dA[2] ) + cimag( dA[2] ) * cimag( dA[2] ) ) ;
  const double QQ4 = 1.0 - 0.5 * ( cimag( dA[4] ) * cimag( dA[4] ) +
				   creal( dA[1] ) * creal( dA[1] ) + cimag( dA[1] ) * cimag( dA[1] ) +
				   creal( dA[5] ) * creal( dA[5] ) + cimag( dA[5] ) * cimag( dA[5] ) ) ;
  const double complex QQ1 = 0.5 * ( I * dA[1] * ( cimag( dA[0] ) + cimag( dA[4] ) ) - dA[2] * conj( dA[5] ) ) ;
  const double complex QQ3 = -0.5 * ( conj( dA[1] ) * I * ( cimag( dA[0] ) + cimag( dA[4] ) ) + dA[5] * conj( dA[2] ) ) ;
  const double complex QQ6 = -0.5 * ( conj( dA[2] ) * I * ( cimag( dA[0] ) + cimag( dA[8] ) ) - conj( dA[5] ) * conj( dA[1] ) ) ;
  const double complex QQ7 = -0.5 * ( conj( dA[5] ) * I * ( cimag( dA[4] ) + cimag( dA[8] ) ) + conj( dA[2] ) * dA[1] ) ;
  dA[0] += QQ0 ;
  dA[1] += QQ1 ;
  dA[3] += QQ3 ;
  dA[4] += QQ4 ;
  dA[6] += QQ6 ;
  dA[7] += QQ7 ;
#elif NC==2
  // compact little result, checked. (dA[1])^2==0, sign is due to antihermiticity 
  *( dA + 0 ) += 1.0 - 0.5 * ( cimag( dA[0] ) * cimag( dA[0] ) +
			       creal( dA[1] ) * creal( dA[1] ) +
			       cimag( dA[1] ) * cimag( dA[1] ) ) ;
#else
  GLU_complex dAdA[ NCNC ] ;
  register GLU_complex cache ;
  multab( dAdA , dA , dA ) ;
  int mu ;
  for( mu = 0 ; mu < NCNC ; mu++ ) {
    cache = dA[mu] - 0.5*dAdA[mu] ;
    dA[mu] = ( mu%( NC+1 ) == 0 ) ? 1.0 + cache : cache ;
  }
#endif
  return ;
}
#endif

// numerically more friendly average?
static double
average_traces( const int LENGTH )
{
  int i ;
  register double ave = 0.0 ;
  for( i = 0 ; i < LENGTH ; i++ ) {
    ave += ( traces[i] - ave ) * divs[i] ;
  }
  return ave ;
}

// gauge transformation and a log
#if (defined deriv_full) || (defined deriv_fulln)
static void
gtrans_log( GLU_complex A[ HERMSIZE ] ,
	    const GLU_complex a[ NCNC ] ,
	    const GLU_complex b[ NCNC ] ,
	    const GLU_complex c[ NCNC ] )
{
  GLU_complex temp[ NCNC ] ;
  equiv( temp , b ) ;
  gtransform_local( a , temp , c ) ;
  exact_log_slow_short( A , temp ) ;
  return ;
}
#endif

// trace of an SU(NC) gauge transformation
static inline double
Re_trace_abc_dag_suNC( const GLU_complex a[ NCNC ] , 
		       const GLU_complex b[ NCNC ] , 
		       const GLU_complex c[ NCNC ] )
{
  register double sum ;
#if NC == 3
  //const GLU_complex a0 = ( a[0] * b[0] + a[1] * b[3] + a[2] * b[6] ) ;
  const GLU_real Ra0 = creal( a[0] ) * creal( b[0] ) - cimag( a[0] ) * cimag( b[0] ) +\
                       creal( a[1] ) * creal( b[3] ) - cimag( a[1] ) * cimag( b[3] ) +\
                       creal( a[2] ) * creal( b[6] ) - cimag( a[2] ) * cimag( b[6] ) ;
  const GLU_real Ia0 = creal( a[0] ) * cimag( b[0] ) + cimag( a[0] ) * creal( b[0] ) +\
                       creal( a[1] ) * cimag( b[3] ) + cimag( a[1] ) * creal( b[3] ) +\
                       creal( a[2] ) * cimag( b[6] ) + cimag( a[2] ) * creal( b[6] ) ;
  //const GLU_complex a1 = ( a[0] * b[1] + a[1] * b[4] + a[2] * b[7] ) ;
  const GLU_real Ra1 = creal( a[0] ) * creal( b[1] ) - cimag( a[0] ) * cimag( b[1] ) +\
                       creal( a[1] ) * creal( b[4] ) - cimag( a[1] ) * cimag( b[4] ) +\
                       creal( a[2] ) * creal( b[7] ) - cimag( a[2] ) * cimag( b[7] ) ;
  const GLU_real Ia1 = creal( a[0] ) * cimag( b[1] ) + cimag( a[0] ) * creal( b[1] ) +\
                       creal( a[1] ) * cimag( b[4] ) + cimag( a[1] ) * creal( b[4] ) +\
                       creal( a[2] ) * cimag( b[7] ) + cimag( a[2] ) * creal( b[7] ) ;
  //const GLU_complex a2 = ( a[0] * b[2] + a[1] * b[5] + a[2] * b[8] ) ;
  const GLU_real Ra2 = creal( a[0] ) * creal( b[2] ) - cimag( a[0] ) * cimag( b[2] ) +\
                       creal( a[1] ) * creal( b[5] ) - cimag( a[1] ) * cimag( b[5] ) +\
                       creal( a[2] ) * creal( b[8] ) - cimag( a[2] ) * cimag( b[8] ) ;
  const GLU_real Ia2 = creal( a[0] ) * cimag( b[2] ) + cimag( a[0] ) * creal( b[2] ) +\
                       creal( a[1] ) * cimag( b[5] ) + cimag( a[1] ) * creal( b[5] ) +\
                       creal( a[2] ) * cimag( b[8] ) + cimag( a[2] ) * creal( b[8] ) ;
  //const GLU_complex a3 = ( a[3] * b[0] + a[4] * b[3] + a[5] * b[6] ) ;
  const GLU_real Ra3 = creal( a[3] ) * creal( b[0] ) - cimag( a[3] ) * cimag( b[0] ) +\
                       creal( a[4] ) * creal( b[3] ) - cimag( a[4] ) * cimag( b[3] ) +\
                       creal( a[5] ) * creal( b[6] ) - cimag( a[5] ) * cimag( b[6] ) ;
  const GLU_real Ia3 = creal( a[3] ) * cimag( b[0] ) + cimag( a[3] ) * creal( b[0] ) +\
                       creal( a[4] ) * cimag( b[3] ) + cimag( a[4] ) * creal( b[3] ) +\
                       creal( a[5] ) * cimag( b[6] ) + cimag( a[5] ) * creal( b[6] ) ;
  //const GLU_complex a4 = ( a[3] * b[1] + a[4] * b[4] + a[5] * b[7] ) ;
  const GLU_real Ra4 = creal( a[3] ) * creal( b[1] ) - cimag( a[3] ) * cimag( b[1] ) +\
                       creal( a[4] ) * creal( b[4] ) - cimag( a[4] ) * cimag( b[4] ) +\
                       creal( a[5] ) * creal( b[7] ) - cimag( a[5] ) * cimag( b[7] ) ;
  const GLU_real Ia4 = creal( a[3] ) * cimag( b[1] ) + cimag( a[3] ) * creal( b[1] ) +\
                       creal( a[4] ) * cimag( b[4] ) + cimag( a[4] ) * creal( b[4] ) +\
                       creal( a[5] ) * cimag( b[7] ) + cimag( a[5] ) * creal( b[7] ) ;
  //const GLU_complex a5 = ( a[3] * b[2] + a[4] * b[5] + a[5] * b[8] ) ;
  const GLU_real Ra5 = creal( a[3] ) * creal( b[2] ) - cimag( a[3] ) * cimag( b[2] ) +\
                       creal( a[4] ) * creal( b[5] ) - cimag( a[4] ) * cimag( b[5] ) +\
                       creal( a[5] ) * creal( b[8] ) - cimag( a[5] ) * cimag( b[8] ) ;
  const GLU_real Ia5 = creal( a[3] ) * cimag( b[2] ) + cimag( a[3] ) * creal( b[2] ) +\
                       creal( a[4] ) * cimag( b[5] ) + cimag( a[4] ) * creal( b[5] ) +\
                       creal( a[5] ) * cimag( b[8] ) + cimag( a[5] ) * creal( b[8] ) ;
  //const GLU_complex a6 = conj( a1 * a5 - a2 * a4 ) ;
  const GLU_real Ra6 = +( Ra1 * Ra5 - Ia1 * Ia5 - Ra2 * Ra4 + Ia2 * Ia4 ) ;
  const GLU_real Ia6 = -( Ra1 * Ia5 + Ia1 * Ra5 - Ra2 * Ia4 - Ia2 * Ra4 ) ;
  //const GLU_complex a7 = conj( a2 * a3 - a0 * a5 ) ;
  const GLU_real Ra7 = +( Ra2 * Ra3 - Ia2 * Ia3 - Ra0 * Ra5 + Ia0 * Ia5 ) ;
  const GLU_real Ia7 = -( Ra2 * Ia3 + Ia2 * Ra3 - Ra0 * Ia5 - Ia0 * Ra5 ) ;
  //const GLU_complex a8 = conj( a0 * a4 - a1 * a3 ) ;
  const GLU_real Ra8 = +( Ra0 * Ra4 - Ia0 * Ia4 - Ra1 * Ra3 + Ia1 * Ia3 ) ;
  const GLU_real Ia8 = -( Ra0 * Ia4 + Ia0 * Ra4 - Ra1 * Ia3 - Ia1 * Ra3 ) ;
  // and compute the trace
  sum = Ra0 * creal( c[0] ) + Ia0 * cimag( c[0] ) ; 
  sum = Ra1 * creal( c[1] ) + Ia1 * cimag( c[1] ) + sum ; 
  sum = Ra2 * creal( c[2] ) + Ia2 * cimag( c[2] ) + sum ; 
  sum = Ra3 * creal( c[3] ) + Ia3 * cimag( c[3] ) + sum ; 
  sum = Ra4 * creal( c[4] ) + Ia4 * cimag( c[4] ) + sum ; 
  sum = Ra5 * creal( c[5] ) + Ia5 * cimag( c[5] ) + sum ; 
  sum = Ra6 * creal( c[6] ) + Ia6 * cimag( c[6] ) + sum ; 
  sum = Ra7 * creal( c[7] ) + Ia7 * cimag( c[7] ) + sum ; 
  sum = Ra8 * creal( c[8] ) + Ia8 * cimag( c[8] ) + sum ; 
#elif NC == 2
  const GLU_complex a0 = a[0] * b[0] + a[1] * b[2] ;
  const GLU_complex a1 = a[0] * b[1] + a[1] * b[3] ;
  sum  = creal( a0 ) * creal( c[0] ) + cimag( a0 ) * cimag( c[0] ) ;
  sum += creal( a1 ) * creal( c[1] ) + cimag( a1 ) * cimag( c[1] ) ;
  sum -= creal( a1 ) * creal( c[2] ) - cimag( a1 ) * cimag( c[2] ) ;
  sum += creal( a0 ) * creal( c[3] ) - cimag( a0 ) * cimag( c[3] ) ;
#else
  GLU_real trABCdag ;
  trace_abc_dag_Re( &trABCdag , a , b , c ) ;
  sum = (double)trABCdag ;
#endif
  return sum ;
}

// could have several different searches here
double
approx_minimum( const int nmeas , 
		const double alphas[ nmeas ] ,
		const double functional[ nmeas ] )
{
  // compute the spline derivatives
  double derivative[ nmeas ] ;
  spline_derivative( derivative , alphas , functional , nmeas ) ;

  // compute the sum of the derivatives, if they are all small my thinking is
  // that we are at the limit of the precision of the functional
  double sumder = 0.0 ;
  int sumneg = 0 , i ;
  for( i = 0 ; i < nmeas ; i++ ) {

    // sum of the derivatives, if we are too flat
    // we exit returning the user-specified tuning alpha
    sumder += fabs( derivative[i] ) ;

    // we find the minimum using this dirty method
    // this is a bad idea - what if we had more than one? TODO!
    if( derivative[i] < 0. ) sumneg ++ ;

    #ifdef V_FOR_VERBOSE
    printf( "[GF] der[%d] %e \n" , i , derivative[i] ) ;
    #endif
  }

  #ifdef V_FOR_VERBOSE
  printf( "[GF] sumneg :: %d \n" , sumneg ) ;
  printf( "[GF] sumder :: %e \n" , sumder ) ;
  #endif

  // if we are at the limit of precision we leave
  if( sumder < PREC_TOL ) {
    return Latt.gf_alpha ;
  }

  // at the moment this routine assumes a quadratic shape
  // if there are no negative terms in the derivative we return 0
  if( sumneg == 0 ) {
    return 0.0 ;
  // if it is all negative the best alpha is greater than our largest probe
  // we return the largest probe
  } else if( sumneg == nmeas ) {
    return alphas[nmeas-1] ;
  // otherwise we have bound the minimum and we solve for it 
  } else {
    const double result = cubic_min( alphas , functional , 
				     derivative , sumneg ) ;

    if( isnan( result ) ) { 
      return 0.0 ;
    } else {
      return result ;
    }
  }
  return 0.0 ;
}

// little wrapper for the traces array
void allocate_traces( const int LENGTH )
{
  traces = malloc( LENGTH * sizeof( double ) ) ;
  divs = malloc( LENGTH * sizeof( double ) ) ;
  int i ;
  // set up the divs so it doesn't have to recompute all the time
  for( i = 0 ; i < LENGTH ; i++ ) {
    divs[i] = 1.0 / (double)( i + 1. ) ;
  }
  return ;
}

// Coulomb derivative term
double
coul_gtrans_fields( struct sp_site_herm *__restrict rotato ,
		    const struct site *__restrict lat ,
		    const GLU_complex *__restrict *__restrict slice_gauge ,
		    const int t )
{
  const double fact = 1.0 / (double)( ( ND - 1 ) * NC ) ;
  int i ;
#pragma omp parallel for private(i)
  PFOR( i = 0 ; i < LCU ; i ++ ) {
    GLU_complex temp[ NCNC ] ;
    #ifdef deriv_lin
    double loc_sum = 0.0 ;
    #endif
    const int j = i + LCU * t ;
    int mu ;
    for( mu = 0 ; mu < ND-1 ; mu++ ) {
      const int it = lat[i].neighbor[mu] ;
      equiv( temp , lat[j].O[mu] ) ;
      gtransform_local( slice_gauge[i] , temp , slice_gauge[it] ) ;
      #if (defined deriv_lin) || (defined deriv_linn) 
      Hermitian_proj_short( rotato[i].O[mu] , temp ) ;
      #else
      exact_log_slow_short( rotato[i].O[mu] , temp ) ;
      #endif
      // compute val in here?
      #ifdef deriv_lin
      loc_sum += creal( trace( temp ) ) ;
      #endif
    }
    #ifdef deriv_lin
    traces[i] = (double)loc_sum * fact ;
    #endif
  }

  // this computes the \alpha == 0.0 contribution for the log def
#if defined deriv_full
#pragma omp parallel for private(i)
  PFOR( i = 0 ; i < LCU ; i ++ ) {
    GLU_real tr ;
    register double loc_sum = 0.0 ;
    int mu ;
    for( mu = 0 ; mu < ND-1 ; mu++ ) {
      trace_ab_herm_short( &tr , rotato[i].O[mu] , rotato[i].O[mu] ) ;
      loc_sum += (double)tr ;
    }
    traces[i] = 0.5 * loc_sum * fact ;
  }
#endif

#if defined deriv_full
  return average_traces( LCU ) ;
#elif defined deriv_lin
  return 1.0 - average_traces( LCU ) ;
#endif
}

// for the polyak-ribiere
inline double
PRfmax( const double a , const double b )
{
  return ( b < a ? a : b ) ;
}

// gauge transformed functional evaluations
double
evaluate_alpha( const GLU_complex *__restrict *__restrict gauge , 
		const struct site *__restrict lat ,
		const int DIR ,
		const int LENGTH ,
		const int t ) 
{
  const double fact = 1.0 / (double)( NC * DIR ) ;

  // gauge transform to check the functional, this is the expensive bit, it is worth
  // really considering if there is something cheaper out there for us
  int i ;
  #pragma omp parallel for private(i)
  for( i = 0 ; i < LENGTH ; i++ ) {
    // some stacked allocations
    #if (defined deriv_full) || (defined deriv_fulln) || (defined deriv_fullnn)
    GLU_complex A[ HERMSIZE ] ;
    GLU_real trAA ;
    #endif

    // and compute the relevant slice
    const int j = LENGTH * t + i ;

    // gauge transform on site fields
    register double loc_sum = 0.0 ;
    int mu ;
    for( mu = 0 ; mu < DIR ; mu++ ) {

      // give it the right functional
      #if ( defined deriv_lin )
      // trace identity used here
      loc_sum += Re_trace_abc_dag_suNC( gauge[i] , lat[j].O[mu] , 
					gauge[lat[i].neighbor[mu]] ) ;
      #elif (defined deriv_linn )
      ///////////////////////////////////////////
      // TODO :: Think about the functionals being minimised
      #elif (defined deriv_fulln )
      ///////////////////////////////////////////
      // TODO :: Think about the functionals being minimised
      #else
      // gauge transform of U_\mu(x+\mu/2)
      gtrans_log( A , gauge[i] , lat[j].O[mu] , 
		  gauge[lat[i].neighbor[mu]] ) ;
      trace_ab_herm_short( &trAA , A , A ) ;
      loc_sum += 0.5 * (double)trAA ;
      #endif
    }
    // sum the trace or whatever
    traces[i] = (double)loc_sum * fact ;
  }

  // functional definitions are slightly different
#if ( defined deriv_lin ) || (defined deriv_linn )
  return 1.0 - average_traces( LENGTH ) ;
#else
  return average_traces( LENGTH ) ;
#endif
}

// is the same for Landau and Coulomb just with different LENGTHS
void
FOURIER_ACCELERATE( GLU_complex *__restrict *__restrict in ,
		    GLU_complex *__restrict *__restrict out ,
		    const void *__restrict forward ,
		    const void *__restrict backward ,
		    const GLU_real *__restrict psq ,
		    const int LENGTH ) 
{
#ifdef HAVE_FFTW3_H
  const fftw_plan *forw = ( const fftw_plan* )forward ;
  const fftw_plan *back = ( const fftw_plan* )backward ;
  ///// Fourier Acceleration //////
  #ifdef OMP_FFTW
  int mu , i ;
  for( mu = 0 ; mu < TRUE_HERM ; mu++ ) {
    PSPAWN fftw_execute( forw[mu] ) ; 
    #pragma omp parallel for private(i)
    PFOR( i = 0 ; i < LENGTH ; i++ ) {
      out[ mu ][ i ] *= psq[i] ;
    }
    PSPAWN fftw_execute( back[mu] ) ; 
  }
  PSYNC ;
  #else
  // single core FFT's
  int mu ;
  #pragma omp parallel for private(mu) schedule(dynamic)
  for( mu = 0 ; mu < TRUE_HERM ; mu++ ) {
    PSPAWN fftw_execute( forw[mu] ) ; 
    int i ;
    PFOR( i = 0 ; i < LENGTH ; i++ ) {
      out[ mu ][ i ] *= psq[i] ;
    }
    PSPAWN fftw_execute( back[mu] ) ; 
  }
  PSYNC ;
  #endif
#endif
  return ;
}

// and for freeing the traces array
void free_traces( void ) { free( traces ) ; free( divs ) ; }

// compute the functional quickly
double
gauge_functional_fast( const struct site *__restrict lat )
{
  const double fact = 1.0 / (double)( NC * ND ) ;
  int i ;
#pragma omp parallel for private(i)
  for( i = 0 ; i < LVOLUME ; i++ ) {
    #if (defined deriv_full) || (defined deriv_fulln) || (defined deriv_fullnn)
    GLU_complex A[ HERMSIZE ] ;
    GLU_real trAA ;
    #endif
    register double loc_sum = 0.0 ;
    int mu ;
    for( mu = 0 ; mu < ND ; mu++ ) {
      #if ( defined deriv_lin )
        #if NC == 3
        loc_sum += creal( lat[i].O[mu][0] ) ;
	loc_sum += creal( lat[i].O[mu][4] ) ;
	loc_sum += creal( lat[i].O[mu][8] ) ;
        #elif NC == 2
        loc_sum += creal( lat[i].O[mu][0] ) ;
	loc_sum += creal( lat[i].O[mu][3] ) ;
        #else
	loc_sum += creal( trace( lat[i].O[mu] ) ) ;
        #endif
      #elif (defined deriv_linn )
	GLU_complex temp[ NCNC ] ;
	multab_suNC( temp , lat[i].O[mu] , lat[lat[i].neighbor[mu]].O[mu] ) ;
	loc_sum += creal( trace( temp ) ) ;
      #else
	// still need to think about these
      exact_log_slow_short( A , lat[i].O[mu] ) ;
      trace_ab_herm_short( &trAA , A , A ) ;
      loc_sum += 0.5 * (double)trAA ;
      #endif
    }
    traces[i] = loc_sum * fact ;
  }

#if ( defined deriv_lin ) || (defined deriv_linn )
  return 1.0 - average_traces( LVOLUME ) ;
#else
  return average_traces( LVOLUME ) ;
#endif
}

// this is the same between Landau and Coulomb so I put it here 
inline void
set_gauge_matrix( GLU_complex *__restrict gx ,
		  const GLU_complex *__restrict *__restrict in ,
		  const double alpha ,
		  const int i )
{
#ifdef exp_exact
  GLU_complex short_gauge[ HERMSIZE ] ;
  #if NC==3
  short_gauge[ 0 ] = alpha * cimag( in[ 0 ][ i ] ) ;
  short_gauge[ 1 ] = -I * alpha * in[ 1 ][ i ] ;
  short_gauge[ 2 ] = -I * alpha * in[ 2 ][ i ] ;
  short_gauge[ 3 ] = -alpha * creal( in[ 0 ][ i ] ) ;
  short_gauge[ 4 ] = -I * alpha * in[ 3 ][ i ] ;
  #elif NC==2
  short_gauge[ 0 ] = alpha * cimag( in[ 0 ][ i ] ) ;
  short_gauge[ 1 ] = -I * alpha * in[ 1 ][ i ] ;
  #else 
  // this makes it hermitian, which is what the exponential expects
  int mu ;
  for( mu = 0 ; mu < HERMSIZE ; mu++ ) {
    short_gauge[mu] = -I * alpha * in[ mu ][ i ] ;
  }
  #endif
  exponentiate_short( gx , short_gauge ) ; 
#else
  #if NC==3
  gx[0] = 1.0 + I * alpha * cimag( in[0][i] ) ; 
  gx[1] = alpha * in[1][i] ;  
  gx[3] = -conj( gx[1] ) ; 
  gx[4] = 1.0 + -I * alpha * creal( in[0][i] ) ; 
  gx[6] = -alpha * conj( in[2][i] ) ;  
  gx[7] = -alpha * conj( in[3][i] ) ;
  #elif NC==2
  gx[0] = 1.0 + I * alpha * cimag( in[0][i] ) ; 
  gx[1] = alpha * in[1][i] ; 
  gx[2] = -conj( gx[1] ) ; 
  #else
  GLU_complex short_gauge[ HERMSIZE ] ;
  // this makes it antihermitian
  int mu ;
  for( mu = 0 ; mu < HERMSIZE ; mu++ ) {
    short_gauge[mu] = alpha * in[ mu ][ i ] ;
  }
  rebuild_antihermitian( gx , short_gauge ) ;
  add_constant( gx , 1.0 ) ; 
  #endif
  #if NC > 18
  nape_reunit( gx ) ; 
  #else
  reunit2( gx ) ; 
  #endif
#endif
}

// derivative
double
sum_deriv( const GLU_complex *__restrict *__restrict in , 
	   const int LENGTH )
{
  int i ;
  #pragma omp parallel for private(i)
  PFOR( i = 0 ; i < LENGTH ; i++ ) {
    register double loc_sum = 0.0 ;
    #if NC == 3
    loc_sum += creal(in[0][i]) * creal(in[0][i]) + cimag(in[0][i])*cimag(in[0][i]) 
      + creal(in[0][i]) * cimag( in[0][i] ) ;
    loc_sum += creal(in[1][i]) * creal(in[1][i]) + cimag(in[1][i])*cimag(in[1][i]) ; 
    loc_sum += creal(in[2][i]) * creal(in[2][i]) + cimag(in[2][i])*cimag(in[2][i]) ; 
    loc_sum += creal(in[3][i]) * creal(in[3][i]) + cimag(in[3][i])*cimag(in[3][i]) ; 
    #elif NC == 2
    loc_sum += creal(in[0][i]) * creal(in[0][i]) + cimag(in[0][i])*cimag(in[0][i]) ;
    loc_sum += creal(in[1][i]) * creal(in[1][i]) + cimag(in[1][i])*cimag(in[1][i]) ;
    #else
    GLU_complex temp[ HERMSIZE ] ;
    int mu ;
    for( mu = 0 ; mu < HERMSIZE ; mu++ ) {
      temp[mu] = in[mu][i] ;
    }
    GLU_real tr ;
    trace_ab_herm_short( &tr , temp , temp ) ;
    loc_sum = 0.5 * (double)tr ;
    #endif
    traces[i] = loc_sum ;
  }
  return 2.0 * average_traces( LENGTH ) ;
}

// Polak Ribiere Numerator
double
sum_PR_numerator( const GLU_complex *__restrict *__restrict in , 
		  const GLU_complex *__restrict *__restrict in_old ,
		  const int LENGTH ) 
{
  int i ;
#pragma omp parallel for private(i)
  PFOR( i = 0 ; i < LENGTH ; i++ ) {
    register double loc_sum = 0.0 ;
#if NC == 3
    register GLU_complex temp = in[0][i] - in_old[0][i] ;
    loc_sum += 2.0 * ( creal( in[0][i] ) * creal( temp ) + cimag( in[0][i] ) * cimag( temp ) ) ;
    loc_sum += creal( in[0][i] ) * cimag( temp ) + cimag( in[0][i] ) * creal( temp ) ;
    temp = in[1][i] - in_old[1][i] ;
    loc_sum += 2.0 * ( creal( in[1][i] ) * creal( temp ) + cimag( in[1][i] ) * cimag( temp ) ) ;
    temp = in[2][i] - in_old[2][i] ;
    loc_sum += 2.0 * ( creal( in[2][i] ) * creal( temp ) + cimag( in[2][i] ) * cimag( temp ) ) ;
    temp = in[3][i] - in_old[3][i] ;
    loc_sum += 2.0 * ( creal( in[3][i] ) * creal( temp ) + cimag( in[3][i] ) * cimag( temp ) ) ;
#elif NC == 2
    register GLU_complex temp = in[0][i] - in_old[0][i] ;
    loc_sum += 2.0 * ( creal( in[0][i] ) * creal( temp ) + cimag( in[0][i] ) * cimag( temp ) ) ;
    temp = in[1][i] - in_old[1][i] ;
    loc_sum += 2.0 * ( creal( in[1][i] ) * creal( temp ) + cimag( in[1][i] ) * cimag( temp ) ) ;
#else
    GLU_complex temp[ HERMSIZE ] , temp2[ NCNC ] ;
    int mu ;
    for( mu = 0 ; mu < HERMSIZE ; mu++ ) {
      temp[mu] = in[mu][i] - in_old[mu][i] ;
      temp2[ mu ] = in[ mu ][ i ] ;
    }
    GLU_real tr ;
    trace_ab_herm_short( &tr , temp2 , temp ) ;
    loc_sum = (double)tr ;
#endif
    traces[i] = loc_sum ;
  }
  return average_traces( LENGTH ) ;
}
