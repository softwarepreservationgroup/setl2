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
\*/

/*\
 *  \function{Table of Built-In Symbols}
\*/

#ifdef COMPILER
{  ft_omega,   "OM",                &sym_omega,          0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_omega,   &spec_omega,      0, NULL,                0,    0     },
#endif

#ifdef COMPILER
{  ft_atom,    "FALSE",             &sym_false,          0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_atom,    &spec_false,      0, NULL,                0,    0     },
#endif

#ifdef COMPILER
{  ft_atom,    "TRUE",              &sym_true,           0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_atom,    &spec_true,       0, NULL,                0,    0     },
#endif

#ifdef COMPILER
{  ft_long,    "0",                 &sym_zero,           0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_short,   &spec_zero,       0, NULL,                0,    0     },
#endif

#ifdef COMPILER
{  ft_long,    "1",                 &sym_one,            0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_short,   &spec_one,        1, NULL,                0,    0     },
#endif

#ifdef COMPILER
{  ft_long,    "2",                 &sym_two,            0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_short,   &spec_two,        2, NULL,                0,    0     },
#endif

#ifdef COMPILER
{  ft_omega,   "COMMAND_LINE",      NULL,                0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_omega,   &spec_cline,      0, NULL,                0,    0     },
#endif

#ifdef COMPILER
{  ft_omega,   "_nullset",          &sym_nullset,        0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_omega,   &spec_nullset,    0, NULL,                0,    0     },
#endif

#ifdef COMPILER
{  ft_omega,   "_nulltup",          &sym_nulltup,        0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_omega,   &spec_nulltup,    0, NULL,                0,    0     },
#endif

#ifdef COMPILER
{  ft_omega,   "_memory",           &sym_memory,         0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_omega,   &spec_memory,     0, NULL,                0,    0     },
#endif

#ifdef COMPILER
{  ft_omega,   "ABEND_TRAP",        &sym_abendtrap,      0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_omega,   &spec_abendtrap,  0, NULL,                0,    0     },
#endif

/*
 *  Miscellaneous procedures
 */

#ifdef COMPILER
{  ft_proc,    "NEWAT",             NULL,                0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_newat,         0,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "DATE",              NULL,                0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_date,          0,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "TIME",              NULL,                0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_time,          0,    0     },
#endif

/*
 *  Type checking procedures
 */

#ifdef COMPILER
{  ft_proc,    "TYPE",              NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_type,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IS_ATOM",           NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_is_atom,       1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IS_BOOLEAN",        NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_is_boolean,    1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IS_INTEGER",        NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_is_integer,    1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IS_REAL",           NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_is_real,       1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IS_STRING",         NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_is_string,     1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IS_SET",            NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_is_set,        1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IS_MAP",            NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_is_map,        1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IS_TUPLE",          NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_is_tuple,      1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IS_PROCEDURE",      NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_is_procedure,  1,    0     },
#endif

/*
 *  Math procedures
 */

#ifdef COMPILER
{  ft_proc,    "ABS",               NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_abs,           1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "EVEN",              NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_even,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "ODD",               NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_odd,           1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "FLOAT",             NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_float,         1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "ATAN2",             NULL,                2,    0,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_atan2,         2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "FIX",               NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_fix,           1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "FLOOR",             NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_floor,         1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "CEIL",              NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_ceil,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "EXP",               NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_exp,           1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "LOG",               NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_log,           1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "COS",               NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_cos,           1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "SIN",               NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_sin,           1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "TAN",               NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_tan,           1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "ACOS",              NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_acos,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "ASIN",              NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_asin,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "ATAN",              NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_atan,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "TANH",              NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_tanh,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "SQRT",              NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_sqrt,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "SIGN",              NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_sign,          1,    0     },
#endif

/*
 *  String scanning primitives
 */

#ifdef COMPILER
{  ft_proc,    "CHAR",              NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_char,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "STR",               NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_str,           1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "ANY",               NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_any,           2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "BREAK",             NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_break,         2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "LEN",               NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_len,           2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "MATCH",             NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_match,         2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "NOTANY",            NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_notany,        2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "SPAN",              NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_span,          2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "LPAD",              NULL,                2,    0,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_lpad,          2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "RANY",              NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_rany,          2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "RBREAK",            NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_rbreak,        2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "RLEN",              NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_rlen,          2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "RMATCH",            NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_rmatch,        2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "RNOTANY",           NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_rnotany,       2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "RSPAN",             NULL,                2,    0,    "31"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_rspan,         2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "RPAD",              NULL,                2,    0,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_rpad,          2,    0     },
#endif

/*
 *  Input / Output procedures
 */

#ifdef COMPILER
{  ft_proc,    "OPEN",              NULL,                1,    1,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_open,          1,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "CLOSE",             NULL,                1,    0,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_close,         1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "GET",               NULL,                1,    1,    "22"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_get,           1,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "GETA",              NULL,                2,    1,    "122" },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_geta,          2,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "READ",              NULL,                1,    1,    "22"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_read,          1,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "READA",             NULL,                2,    1,    "122" },
#endif
#ifdef INTERP
{  ft_proc,    &spec_reada,      0, setl2_reada,         2,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "READS",             NULL,                2,    1,    "322" },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_reads,         2,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "UNSTR",             NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_unstr,         1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "BINSTR",            NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_binstr,        1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "UNBINSTR",          NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_unbinstr,      1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "PRINT",             NULL,                0,    1,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_print,         0,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "NPRINT",            NULL,                0,    1,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_nprint,        0,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "PRINTA",            NULL,                1,    1,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    &spec_printa,        0, setl2_printa,        1,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "NPRINTA",           NULL,                1,    1,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    &spec_nprinta,       0, setl2_nprinta,       1,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "GETB",              NULL,                2,    1,    "122" },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_getb,          2,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "PUTB",              NULL,                1,    1,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_putb,          1,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "GETS",              NULL,                4,    0,    "1112"},
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_gets,          4,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "PUTS",              NULL,                3,    0,    "111" },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_puts,          1,    1     },
#endif

#ifdef COMPILER
{  ft_proc,    "FSIZE",             NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    &spec_fsize,      0, setl2_fsize,         1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "EOF",               NULL,                0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_eof,           0,    0     },
#endif

/*
 *  System procedures
 */

#ifdef COMPILER
{  ft_proc,    "FEXISTS",           NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_fexists,       1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "SYSTEM",            NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_system,        1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "ABORT",             NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_abort,         1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "CALLOUT",           NULL,                3,    0,    "111" },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_Ccallout,      3,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "OPCODE_COUNT",      NULL,                0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_opcode_count,  0,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "CALLOUT2",          NULL,                3,    0,    "111" },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_Ccallout2,     3,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "GETENV",            NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_getenv,        1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "POPEN",             NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_popen,         1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "GETCHAR",           NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_getchar,       1,    0     },
#endif
 
#ifdef COMPILER
{  ft_proc,    "FFLUSH",            NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_fflush,        1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "USER_TIME",         NULL,                0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_user_time,     0,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "SETL2_TRACE",             NULL,          1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_trace,         1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "SETL2_REF_COUNT",             NULL,      1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_ref_count,     1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "LIBRARY_FILE",      NULL,                1,    0,    "1" },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_library_file,  1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "LIBRARY_PACKAGE",   NULL,                1,    0,    "1" },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_library_package,  1,    0     },
#endif

/*
 *  Process procedures
 */


#ifdef COMPILER
{  ft_proc,    "PROC_SUSPEND",           NULL,           1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_suspend,       1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "PROC_RESUME",            NULL,           1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_resume,        1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "PROC_KILL",              NULL,           1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_kill,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "PROC_NEWMBOX",           NULL,           0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_newmbox,       0,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "PROC_AWAIT",             NULL,           1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_await,         1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "PROC_ACHECK",            NULL,           1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_acheck,        1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "PROC_PASS",              NULL,           0,    0,    ""    },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_pass,          0,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "JAVASCRIPT",       NULL,                 1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_javascript,    1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "YIELD",            NULL,                 1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_wait,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "$PASS_SYMTAB",       NULL,               1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_pass_symtab,   1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "EVAL",            NULL,                  1,    0,    "1"   },
#endif

#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_eval,          1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "MALLOC",            NULL,                1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_malloc,        1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "FREE",            NULL,                  1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_dispose,       1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "DLL_OPEN",            NULL,              1,    0,    "1"
},
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_open_lib,      1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "DLL_CLOSE",            NULL,             1,    0,    "1"   },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_close_lib,     1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "DLL_NUMSYMBOLS",            NULL,        1,    0,    "1"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_num_symbols,   1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "DLL_GETSYMBOLNAME",        NULL,         2,    0,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_get_symbol_name, 2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "DLL_GETSYMBOL",            NULL,         2,    0,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_get_symbol,    2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "DLL_FINDSYMBOL",            NULL,        2,    0,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_find_symbol,   2,    0     },
#endif


#ifdef COMPILER
{  ft_proc,    "CALLFUNCTION",            NULL,          3,    0,    "111"
},
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_call_function, 3,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "BPEEK",                   NULL,          2,    0,    "11"
},
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_bpeek,         2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "SPEEK",                   NULL,          2,    0,    "11"
},
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_speek,         2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IPEEK",                   NULL,          2,    0,    "11"
},
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_ipeek,         2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "BPOKE",            NULL,                 3,    0,    "111"
},
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_bpoke,         3,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "SPOKE",            NULL,                 3,    0,    "111"
},
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_spoke,         3,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "IPOKE",            NULL,                 3,    0,    "111"
},
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_ipoke,         3,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "HOST_GET",          NULL,                1,    0,    "1"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_host_get,      1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "HOST_PUT",          NULL,                2,    0,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_host_put,      2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "HOST_CALL",         NULL,                2,    0,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_host_put,      2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "GETURL",         NULL,                   1,    0,    "1"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_geturl,        1,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "POSTURL",         NULL,                   2,    0,    "11"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_posturl,        2,    0     },
#endif

#ifdef COMPILER
{  ft_proc,    "CREATEACTIVEXOBJECT",         NULL,        1,    0,    "1"  },
#endif
#ifdef INTERP
{  ft_proc,    NULL,             0, setl2_create_activexobject,        1,    0     },
#endif


#ifdef COMPILER
{  -1,         NULL,                NULL,                0,    0,    ""    },
#endif
#ifdef INTERP
{  -1,         NULL,             0, NULL,                0,    0     },
#endif
