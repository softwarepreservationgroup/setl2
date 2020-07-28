/*\
 *
 *  % MIT License
 *  %
 *  % Copyright (c) 1990 W. Kirk Snyder
 *  %
 *  % Permission is hereby granted, free of charge, to any person obtaining a copy
 *  % of this software and associated documentation files (the "Software"), to deal
 *  % in the Software without restriction, including without limitation the rights
 *  % to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *  % copies of the Software, and to permit persons to whom the Software is
 *  % furnished to do so, subject to the following conditions:
 *  % 
 *  % The above copyright notice and this permission notice shall be included in all
 *  % copies or substantial portions of the Software.
 *  %
 *  % THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  % IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  % FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  % AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  % LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  % OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *  % SOFTWARE.
 *  %
 *
 *  \package{Method Codes}
 *
 *  This package contains definitions for built-in method codes.
\*/

#ifndef MCODE_LOADED

/* operand type definitions */

/* ## begin mcode_types */
#define m_initobj              0       /* initialize instance               */
#define m_add                  1       /* +                                 */
#define m_add_r                2       /* + on right                        */
#define m_sub                  3       /* -                                 */
#define m_sub_r                4       /* - on right                        */
#define m_mult                 5       /* *                                 */
#define m_mult_r               6       /* * on right                        */
#define m_div                  7       /* /                                 */
#define m_div_r                8       /* / on right                        */
#define m_exp                  9       /* **                                */
#define m_exp_r               10       /* ** on right                       */
#define m_mod                 11       /* mod                               */
#define m_mod_r               12       /* mod on right                      */
#define m_min                 13       /* min                               */
#define m_min_r               14       /* min on right                      */
#define m_max                 15       /* max                               */
#define m_max_r               16       /* max on right                      */
#define m_with                17       /* with                              */
#define m_with_r              18       /* with on right                     */
#define m_less                19       /* less                              */
#define m_less_r              20       /* less on right                     */
#define m_lessf               21       /* lessf                             */
#define m_lessf_r             22       /* lessf on right                    */
#define m_npow                23       /* npow                              */
#define m_npow_r              24       /* npow on right                     */
#define m_uminus              25       /* unary minus                       */
#define m_domain              26       /* domain                            */
#define m_range               27       /* range                             */
#define m_pow                 28       /* pow                               */
#define m_arb                 29       /* arb                               */
#define m_nelt                30       /* #                                 */
#define m_from                31       /* from                              */
#define m_fromb               32       /* fromb                             */
#define m_frome               33       /* frome                             */
#define m_of                  34       /* map, tuple, or string             */
#define m_ofa                 35       /* multi-valued map                  */
#define m_slice               36       /* slice                             */
#define m_end                 37       /* string end                        */
#define m_sof                 38       /* map, tuple, or string assign      */
#define m_sofa                39       /* mmap sinister assignment          */
#define m_sslice              40       /* slice assignment                  */
#define m_send                41       /* string end assignment             */
#define m_lt                  42       /* <                                 */
#define m_lt_r                43       /* < on right                        */
#define m_in                  44       /* in                                */
#define m_in_r                45       /* in on right                       */
#define m_create              46       /* create method                     */
#define m_iterstart           47       /* start iterator method             */
#define m_iternext            48       /* iterator next method              */
#define m_siterstart          49       /* start set iterator method         */
#define m_siternext           50       /* set iterator next method          */
#define m_str                 51       /* printable string method           */
#define m_user                52       /* user method                       */
/* ## end mcode_types */

#ifdef SHARED

char *mcode_desc[] = {                
   "",                                 /*  0  */
   "+",                                /*  1  */
   "",                                 /*  2  */
   "-",                                /*  3  */
   "",                                 /*  4  */
   "*",                                /*  5  */
   "",                                 /*  6  */
   "/",                                /*  7  */
   "",                                 /*  8  */
   "**",                               /*  9  */
   "",                                 /* 10  */
   "MOD",                              /* 11  */
   "",                                 /* 12  */
   "MIN",                              /* 13  */
   "",                                 /* 14  */
   "MAX",                              /* 15  */
   "",                                 /* 16  */
   "WITH",                             /* 17  */
   "",                                 /* 18  */
   "LESS",                             /* 19  */
   "",                                 /* 20  */
   "LESSF",                            /* 21  */
   "",                                 /* 22  */
   "NPOW",                             /* 23  */
   "",                                 /* 24  */
   "Unary minus",                      /* 25  */
   "DOMAIN",                           /* 26  */
   "RANGE",                            /* 27  */
   "POW",                              /* 28  */
   "ARB",                              /* 29  */
   "NELT",                             /* 30  */
   "FROM",                             /* 31  */
   "FROMB",                            /* 32  */
   "FROME",                            /* 33  */
   "F(X)",                             /* 34  */
   "F{X}",                             /* 35  */
   "F(I..J)",                          /* 36  */
   "F(I..)",                           /* 37  */
   "",                                 /* 38  */
   "",                                 /* 39  */
   "",                                 /* 45  */
   "",                                 /* 46  */
   "",                                 /* 40  */
   "",                                 /* 41  */
   "<",                                /* 42  */
   "",                                 /* 43  */
   "IN",                               /* 44  */
   "",                                 /* 47  */
   "",                                 /* 48  */
   "",                                 /* 49  */
   "",                                 /* 50  */
   "",                                 /* 51  */
   "",                                 /* 52  */
    NULL};

#else

extern char *mcode_desc[];              /* print string for forms            */

#endif

#define MCODE_LOADED 1
#endif

