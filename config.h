/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define if building universal (internal helper macro) */
/* #undef AC_APPLE_UNIVERSAL_BUILD */

/* Ascii output to stdout. */
/* #undef ASCII_CHECK */

/* Use the highly improved clover term. */
/* #undef CLOVER_IMPROVE */

/* DBW2 improved staple. */
/* #undef DBW2 */

/* Trace maximisation routine for APE projection. */
#define GIVENS_APE /**/

/* Bluegene specific hacks. */
/* #undef GLU_BGQ */

/* SD gauge fixing routines */
/* #undef GLU_GFIX_SD */

/* GSL default (Mersenne Twister) selection */
/* #undef GSL_RNG */

/* Define to 1 if you have the <complex.h> header file. */
#define HAVE_COMPLEX_H 1

/* FFTW library. */
#define HAVE_FFTW3_H /**/

/* Do we have the GSL libs? */
/* #undef HAVE_GSL */

/* Define to 1 if you have the <gsl/gsl_eigen.h> header file. */
/* #undef HAVE_GSL_GSL_EIGEN_H */

/* Define to 1 if you have the <immintrin.h> header file. */
#define HAVE_IMMINTRIN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <lapacke.h> header file. */
/* #undef HAVE_LAPACKE_H */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <math.h> header file. */
#define HAVE_MATH_H 1

/* Define to 1 if you have the <omp.h> header file. */
#define HAVE_OMP_H 1

/* Look for where we prefix to */
#define HAVE_PREFIX "/home/jamie/PROG/LIBS/GLU_SU3"

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <time.h> header file. */
#define HAVE_TIME_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Iwasaki improved staple. */
/* #undef IWASAKI */

/* Leading term */
/* #undef IWA_WEIGHT1 */

/* Sub-leading rectangle term */
/* #undef IWA_WEIGHT2 */

/* Keep It Simple Stupid RNG (JKISS32) */
/* #undef KISS_RNG */

/* Generated Gribov copies. */
/* #undef LUXURY_GAUGE */

/* Multiply With Carry (1038) selection */
/* #undef MWC_1038_RNG */

/* Multiply With Carry (4096) selection */
/* #undef MWC_4096_RNG */

/* No loop unrolling. */
/* #undef NBLOCK */

/* Compiled for SU(NC) */
#define NC 3 

/* Compiled for ND */
/* #undef ND */

/* targeted, has been seen to be quicker but do not distribute the binaries!
   */
#define NOT_CONDOR_MODE /**/

/* NSTOCH updates */
/* #undef NSTOCH */

/* OBC hacks enabled */
/* #undef OBC_HACK */

/* OpenMP-parallel FFT routines. */
/* #undef OMP_FFTW */

/* overrelaxed routines are slow */
/* #undef OVERRELAXED_GF */

/* Name of package */
#define PACKAGE "glu"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "renwick.james.hudspith@gmail.com"

/* Define to the full name of this package. */
#define PACKAGE_NAME "GLU"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "GLU 1.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "glu"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.1"

/* Plaquette field strength tensor. */
/* #undef PLAQUETTE_FMUNU */

/* Nonexceptional projection index. */
#define PROJ_GRACEY 0

/* Storage of single precision fields and utilisation of fftwf routines. */
/* #undef SINGLE_PREC */

/* Define to 1 if all of the C90 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Symanzik improved staple. */
/* #undef SYMANZIK */

/* Topological charge measurement start. */
/* #undef TOP_VALUE */

/* Version number of package */
#define VERSION "1.1"

/* Well (512) rng selection */
/* #undef WELL_512_RNG */

/* Wflow observable measurements start from here */
/* #undef WFLOW_MEAS_START */

/* Only timing the gradient flow */
/* #undef WFLOW_TIME_ONLY */

/* Wilson flow maximum time stopping condition. */
/* #undef WFLOW_TIME_STOP */

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
#if defined AC_APPLE_UNIVERSAL_BUILD
# if defined __BIG_ENDIAN__
#  define WORDS_BIGENDIAN 1
# endif
#else
# ifndef WORDS_BIGENDIAN
/* #  undef WORDS_BIGENDIAN */
# endif
#endif

/* XOR (1024) selection */
/* #undef XOR_1024_RNG */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Defined the overimproved smearing parameter epsilon. */
/* #undef epsilon */

/* Exact exponentiation for the gauge transformation matrices */
/* #undef exp_exact */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* K5 term in the overimproved smearing definition. */
/* #undef k5 */

/* Define to the equivalent of the C99 'restrict' keyword, or to
   nothing if this is not supported.  Do not define if restrict is
   supported only directly.  */
#define restrict __restrict__
/* Work around a bug in older versions of Sun C++, which did not
   #define __restrict__ or support _Restrict or __restrict__
   even though the corresponding Sun C compiler ended up with
   "#define restrict _Restrict" or "#define restrict __restrict__"
   in the previous line.  This workaround can be removed once
   we assume Oracle Developer Studio 12.5 (2016) or later.  */
#if defined __SUNPRO_CC && !defined __RESTRICT && !defined __restrict__
# define _Restrict
# define __restrict__
#endif

/* Verbose output printed to stdout. */
/* #undef verbose */
