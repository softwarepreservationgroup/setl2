/*
 *
 *                 ActiveX objects Native Package
 *                 ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^        
 */


/* SETL2 system header files */

#ifdef WIN32

/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "builtins.h"                  /* built-in symbols                  */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_strngs.h"                  /* strings                           */
#include "tuples.h"                    /* tuples                            */
#include "procs.h"                     /* procedures                        */
#include "x_reals.h"
#include "axobj.h"
/* constants */

#define YES    1                       /* true constant                     */
#define NO     0                       /* false constant                    */


void access_property(
   SETL_SYSTEM_PROTO
     specifier *target,                  /* return value                      */
	 specifier *left,                    /* The LPDispatch        */
	 specifier *right)                    /* The property name    */

{
   target->sp_form = ft_omega;
   return;
}



#endif