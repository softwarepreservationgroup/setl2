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
 *  \package{Character Table}
 *
 *  This package provides some information by character, similar to that
 *  normally provided in \verb"ctype.h".  The macros here are a little
 *  more powerful, and a little more finely tuned for our purposes.
\*/

#ifndef CHARTAB_LOADED

/* character types */

#define ISID      0x4000               /* valid ID character                */
#define ISDIGIT   0x8000               /* digit character                   */
#define ISWHITE   0x2000               /* whitespace                        */

/* character table proper */

#ifdef SHARED

unsigned char_tab[] = {                /* character table                   */
      /* ascii   0     */  ISWHITE | 0,
      /* ascii   1     */  ISWHITE | 0,
      /* ascii   2     */  ISWHITE | 0,
      /* ascii   3     */  ISWHITE | 0,
#if UNIX
      /* ascii   4     */  0,          /* Unix end of file                  */
#else
      /* ascii   4     */  ISWHITE | 0,
#endif
      /* ascii   5     */  ISWHITE | 0,
      /* ascii   6     */  ISWHITE | 0,
      /* ascii   7     */  ISWHITE | 0,
      /* ascii   8     */  0,
      /* ascii   9     */  0,
      /* ascii  10     */  0,
      /* ascii  11     */  ISWHITE | 0,
      /* ascii  12     */  ISWHITE | 0,
      /* ascii  13     */  0,
      /* ascii  14     */  ISWHITE | 0,
      /* ascii  15     */  ISWHITE | 0,
      /* ascii  16     */  ISWHITE | 0,
      /* ascii  17     */  ISWHITE | 0,
      /* ascii  18     */  ISWHITE | 0,
      /* ascii  19     */  ISWHITE | 0,
      /* ascii  20     */  ISWHITE | 0,
      /* ascii  21     */  ISWHITE | 0,
      /* ascii  22     */  ISWHITE | 0,
      /* ascii  23     */  ISWHITE | 0,
      /* ascii  24     */  ISWHITE | 0,
      /* ascii  25     */  ISWHITE | 0,
#if DOS | OS2 | VMS
      /* ascii  26     */  0,          /* MSDOS & VMS end of file           */
#else
      /* ascii  26     */  0,
#endif
      /* ascii  27     */  ISWHITE | 0,
      /* ascii  28     */  ISWHITE | 0,
      /* ascii  29     */  ISWHITE | 0,
      /* ascii  30     */  ISWHITE | 0,
      /* ascii  31     */  ISWHITE | 0,
      /* ascii  32     */  ISWHITE | 0,
      /* ascii  33 (!) */  0,
      /* ascii  34 (") */  0,
      /* ascii  35 (#) */  0,
      /* ascii  36 ($) */  0,
      /* ascii  37 (%) */  0,
      /* ascii  38 (&) */  0,
      /* ascii  39 (') */  0,
      /* ascii  40 (() */  0,
      /* ascii  41 ()) */  0,
      /* ascii  42 (*) */  0,
      /* ascii  43 (+) */  0,
      /* ascii  44 (,) */  0,
      /* ascii  45 (-) */  0,
      /* ascii  46 (.) */  0,
      /* ascii  47 (/) */  0,
      /* ascii  48 (0) */  ISID | ISDIGIT | 0,
      /* ascii  49 (1) */  ISID | ISDIGIT | 1,
      /* ascii  50 (2) */  ISID | ISDIGIT | 2,
      /* ascii  51 (3) */  ISID | ISDIGIT | 3,
      /* ascii  52 (4) */  ISID | ISDIGIT | 4,
      /* ascii  53 (5) */  ISID | ISDIGIT | 5,
      /* ascii  54 (6) */  ISID | ISDIGIT | 6,
      /* ascii  55 (7) */  ISID | ISDIGIT | 7,
      /* ascii  56 (8) */  ISID | ISDIGIT | 8,
      /* ascii  57 (9) */  ISID | ISDIGIT | 9,
      /* ascii  58 (:) */  0,
      /* ascii  59 (;) */  0,
      /* ascii  60 (<) */  0,
      /* ascii  61 (=) */  0,
      /* ascii  62 (>) */  0,
      /* ascii  63 (?) */  0,
      /* ascii  64 (@) */  0,
      /* ascii  65 (A) */  ISID | ISDIGIT | 10,
      /* ascii  66 (B) */  ISID | ISDIGIT | 11,
      /* ascii  67 (C) */  ISID | ISDIGIT | 12,
      /* ascii  68 (D) */  ISID | ISDIGIT | 13,
      /* ascii  69 (E) */  ISID | ISDIGIT | 14,
      /* ascii  70 (F) */  ISID | ISDIGIT | 15,
      /* ascii  71 (G) */  ISID | ISDIGIT | 16,
      /* ascii  72 (H) */  ISID | ISDIGIT | 17,
      /* ascii  73 (I) */  ISID | ISDIGIT | 18,
      /* ascii  74 (J) */  ISID | ISDIGIT | 19,
      /* ascii  75 (K) */  ISID | ISDIGIT | 20,
      /* ascii  76 (L) */  ISID | ISDIGIT | 21,
      /* ascii  77 (M) */  ISID | ISDIGIT | 22,
      /* ascii  78 (N) */  ISID | ISDIGIT | 23,
      /* ascii  79 (O) */  ISID | ISDIGIT | 24,
      /* ascii  80 (P) */  ISID | ISDIGIT | 25,
      /* ascii  81 (Q) */  ISID | ISDIGIT | 26,
      /* ascii  82 (R) */  ISID | ISDIGIT | 27,
      /* ascii  83 (S) */  ISID | ISDIGIT | 28,
      /* ascii  84 (T) */  ISID | ISDIGIT | 29,
      /* ascii  85 (U) */  ISID | ISDIGIT | 30,
      /* ascii  86 (V) */  ISID | ISDIGIT | 31,
      /* ascii  87 (W) */  ISID | ISDIGIT | 32,
      /* ascii  88 (X) */  ISID | ISDIGIT | 33,
      /* ascii  89 (Y) */  ISID | ISDIGIT | 34,
      /* ascii  90 (Z) */  ISID | ISDIGIT | 35,
      /* ascii  91 ([) */  0,
      /* ascii  92 (\) */  0,
      /* ascii  93 (]) */  0,
      /* ascii  94 (^) */  0,
      /* ascii  95 (_) */  ISID,
      /* ascii  96 (`) */  0,
      /* ascii  97 (a) */  ISID | ISDIGIT | 10,
      /* ascii  98 (b) */  ISID | ISDIGIT | 11,
      /* ascii  99 (c) */  ISID | ISDIGIT | 12,
      /* ascii 100 (d) */  ISID | ISDIGIT | 13,
      /* ascii 101 (e) */  ISID | ISDIGIT | 14,
      /* ascii 102 (f) */  ISID | ISDIGIT | 15,
      /* ascii 103 (g) */  ISID | ISDIGIT | 16,
      /* ascii 104 (h) */  ISID | ISDIGIT | 17,
      /* ascii 105 (i) */  ISID | ISDIGIT | 18,
      /* ascii 106 (j) */  ISID | ISDIGIT | 19,
      /* ascii 107 (k) */  ISID | ISDIGIT | 20,
      /* ascii 108 (l) */  ISID | ISDIGIT | 21,
      /* ascii 109 (m) */  ISID | ISDIGIT | 22,
      /* ascii 110 (n) */  ISID | ISDIGIT | 23,
      /* ascii 111 (o) */  ISID | ISDIGIT | 24,
      /* ascii 112 (p) */  ISID | ISDIGIT | 25,
      /* ascii 113 (q) */  ISID | ISDIGIT | 26,
      /* ascii 114 (r) */  ISID | ISDIGIT | 27,
      /* ascii 115 (s) */  ISID | ISDIGIT | 28,
      /* ascii 116 (t) */  ISID | ISDIGIT | 29,
      /* ascii 117 (u) */  ISID | ISDIGIT | 30,
      /* ascii 118 (v) */  ISID | ISDIGIT | 31,
      /* ascii 119 (w) */  ISID | ISDIGIT | 32,
      /* ascii 120 (x) */  ISID | ISDIGIT | 33,
      /* ascii 121 (y) */  ISID | ISDIGIT | 34,
      /* ascii 122 (z) */  ISID | ISDIGIT | 35,
      /* ascii 123 ({) */  0,
      /* ascii 124 (|) */  0,
      /* ascii 125 (}) */  0,
      /* ascii 126 (~) */  0,
      /* ascii 127     */  0,
      /* ascii 128     */  0,
      /* ascii 129     */  0,
      /* ascii 130     */  0,
      /* ascii 131     */  0,
      /* ascii 132     */  0,
      /* ascii 133     */  0,
      /* ascii 134     */  0,
      /* ascii 135     */  0,
      /* ascii 136     */  0,
      /* ascii 137     */  0,
      /* ascii 138     */  0,
      /* ascii 139     */  0,
      /* ascii 140     */  0,
      /* ascii 141     */  0,
      /* ascii 142     */  0,
      /* ascii 143     */  0,
      /* ascii 144     */  0,
      /* ascii 145     */  0,
      /* ascii 146     */  0,
      /* ascii 147     */  0,
      /* ascii 148     */  0,
      /* ascii 149     */  0,
      /* ascii 150     */  0,
      /* ascii 151     */  0,
      /* ascii 152     */  0,
      /* ascii 153     */  0,
      /* ascii 154     */  0,
      /* ascii 155     */  0,
      /* ascii 156     */  0,
      /* ascii 157     */  0,
      /* ascii 158     */  0,
      /* ascii 159     */  0,
      /* ascii 160     */  0,
      /* ascii 161     */  0,
      /* ascii 162     */  0,
      /* ascii 163     */  0,
      /* ascii 164     */  0,
      /* ascii 165     */  0,
      /* ascii 166     */  0,
      /* ascii 167     */  0,
      /* ascii 168     */  0,
      /* ascii 169     */  0,
      /* ascii 170     */  0,
      /* ascii 171     */  0,
      /* ascii 172     */  0,
      /* ascii 173     */  0,
      /* ascii 174     */  0,
      /* ascii 175     */  0,
      /* ascii 176     */  0,
      /* ascii 177     */  0,
      /* ascii 178     */  0,
      /* ascii 179     */  0,
      /* ascii 180     */  0,
      /* ascii 181     */  0,
      /* ascii 182     */  0,
      /* ascii 183     */  0,
      /* ascii 184     */  0,
      /* ascii 185     */  0,
      /* ascii 186     */  0,
      /* ascii 187     */  0,
      /* ascii 188     */  0,
      /* ascii 189     */  0,
      /* ascii 190     */  0,
      /* ascii 191     */  0,
      /* ascii 192     */  0,
      /* ascii 193     */  0,
      /* ascii 194     */  0,
      /* ascii 195     */  0,
      /* ascii 196     */  0,
      /* ascii 197     */  0,
      /* ascii 198     */  0,
      /* ascii 199     */  0,
      /* ascii 200     */  0,
      /* ascii 201     */  0,
      /* ascii 202     */  0,
      /* ascii 203     */  0,
      /* ascii 204     */  0,
      /* ascii 205     */  0,
      /* ascii 206     */  0,
      /* ascii 207     */  0,
      /* ascii 208     */  0,
      /* ascii 209     */  0,
      /* ascii 210     */  0,
      /* ascii 211     */  0,
      /* ascii 212     */  0,
      /* ascii 213     */  0,
      /* ascii 214     */  0,
      /* ascii 215     */  0,
      /* ascii 216     */  0,
      /* ascii 217     */  0,
      /* ascii 218     */  0,
      /* ascii 219     */  0,
      /* ascii 220     */  0,
      /* ascii 221     */  0,
      /* ascii 222     */  0,
      /* ascii 223     */  0,
      /* ascii 224     */  0,
      /* ascii 225     */  0,
      /* ascii 226     */  0,
      /* ascii 227     */  0,
      /* ascii 228     */  0,
      /* ascii 229     */  0,
      /* ascii 230     */  0,
      /* ascii 231     */  0,
      /* ascii 232     */  0,
      /* ascii 233     */  0,
      /* ascii 234     */  0,
      /* ascii 235     */  0,
      /* ascii 236     */  0,
      /* ascii 237     */  0,
      /* ascii 238     */  0,
      /* ascii 239     */  0,
      /* ascii 240     */  0,
      /* ascii 241     */  0,
      /* ascii 242     */  0,
      /* ascii 243     */  0,
      /* ascii 244     */  0,
      /* ascii 245     */  0,
      /* ascii 246     */  0,
      /* ascii 247     */  0,
      /* ascii 248     */  0,
      /* ascii 249     */  0,
      /* ascii 250     */  0,
      /* ascii 251     */  0,
      /* ascii 252     */  0,
      /* ascii 253     */  0,
      /* ascii 254     */  0,
      /* ascii 255     */  0};

#else

extern unsigned char_tab[];            /* character table                   */

#endif

#define is_id_char(c) (char_tab[c] & ISID)
                                       /* is valid id character             */
#define is_white_space(c) (char_tab[c] & ISWHITE)
                                       /* is white space                    */
#define numeric_val(c) (char_tab[c] & 0x3f)
                                       /* numeric value                     */
#define is_digit(c,b) ((char_tab[c] & ISDIGIT) && (numeric_val(c) < b))
                                       /* is a number in the given base     */
#define to_upper(c) ((c >= 'a' && c <= 'z') ? c + 'A' - 'a' : c)
                                       /* convert to upper case             */

#define CHARTAB_LOADED 1
#endif
