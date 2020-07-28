/*\
 *  This macro makes a rough cut at an equality test.  If the two
 *  specifiers are short and equal we can determine equality without a
 *  procedure call.  If they are long but of different forms we also
 *  avoid a procedure call.  Otherwise we resort to a more robust
 *  function.
\*/

#ifdef COERCE_EQUALITY
#ifdef DIGITAL
#define spec_equal(t,l,r) { \
   if ((l)->sp_form == ft_omega && (r)->sp_form == ft_omega) t = 1; \
   else if ( (l)->sp_form == (r)->sp_form && \
           (  ( (l)->sp_form == ft_short && \
                 (l)->sp_val.sp_short_value == (r)->sp_val.sp_short_value ) || \
            ((l)->sp_val.sp_biggest == (r)->sp_val.sp_biggest))) t = 1; \
   else if ((l)->sp_form == ft_real || (r)->sp_form == ft_real) \
   		t = spec_equal_test(SETL_SYSTEM l,r); \
   else if ((l)->sp_form < ft_real || (r)->sp_form < ft_real) t = 0; \
   else if (((l)->sp_form < ft_set || (r)->sp_form < ft_set) && \
      (l)->sp_form != (r)->sp_form) t = 0; \
   else t = spec_equal_test(SETL_SYSTEM l,r); \
}
#else
#define spec_equal(t,l,r) { \
   if ((l)->sp_form == ft_omega && (r)->sp_form == ft_omega) t = 1; \
   else if ((l)->sp_form == (r)->sp_form && \
            (l)->sp_val.sp_biggest == (r)->sp_val.sp_biggest) t = 1; \
   else if ((l)->sp_form == ft_real || (r)->sp_form == ft_real) \
   		t = spec_equal_test(SETL_SYSTEM l,r); \
   else if ((l)->sp_form < ft_real || (r)->sp_form < ft_real) t = 0; \
   else if (((l)->sp_form < ft_set || (r)->sp_form < ft_set) && \
      (l)->sp_form != (r)->sp_form) t = 0; \
   else t = spec_equal_test(SETL_SYSTEM l,r); \
}
#endif

#else

#ifdef DIGITAL
#define spec_equal(t,l,r) { \
   if ((l)->sp_form == ft_omega && (r)->sp_form == ft_omega) t = 1; \
   else if ( (l)->sp_form == (r)->sp_form && \
           (  ( (l)->sp_form == ft_short && \
                 (l)->sp_val.sp_short_value == (r)->sp_val.sp_short_value ) || \
            ((l)->sp_val.sp_biggest == (r)->sp_val.sp_biggest))) t = 1; \
   else if ((l)->sp_form < ft_real || (r)->sp_form < ft_real) t = 0; \
   else if (((l)->sp_form < ft_set || (r)->sp_form < ft_set) && \
      (l)->sp_form != (r)->sp_form) t = 0; \
   else t = spec_equal_test(SETL_SYSTEM l,r); \
}
#else
#define spec_equal(t,l,r) { \
   if ((l)->sp_form == ft_omega && (r)->sp_form == ft_omega) t = 1; \
   else if ((l)->sp_form == (r)->sp_form && \
            (l)->sp_val.sp_biggest == (r)->sp_val.sp_biggest) t = 1; \
   else if ((l)->sp_form < ft_real || (r)->sp_form < ft_real) t = 0; \
   else if (((l)->sp_form < ft_set || (r)->sp_form < ft_set) && \
      (l)->sp_form != (r)->sp_form) t = 0; \
   else t = spec_equal_test(SETL_SYSTEM l,r); \
}
#endif

#endif

/*\
 *  \function{spec\_hash\_code()}
 *
 *  This macro tries to find hash codes.  If we are lucky we can return
 *  the hash code directly.  If we are not lucky we call a function to
 *  calculate the hash code.
\*/

#define spec_hash_code(t,s) {\
   if ((s)->sp_form == ft_omega) t = 0L; \
   else if ((s)->sp_form <= ft_short) t = (s)->sp_val.sp_short_value; \
   else if ((s)->sp_form <= ft_iter) t = (int32)((s)->sp_val.sp_biggest); \
   else if ((s)->sp_form >= ft_tuple) t = \
      (*((int32 *)((s)->sp_val.sp_biggest) + 1)); \
   else t = spec_hash_code_calc(s); \
}

