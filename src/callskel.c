/*\
 *  Skeleton Callout
 *  ================
 *
 *  This is a skeleton of a callout handler in C.  There is just enough
 *  here for me to test the callout facility.
\*/

/* standard C header files */

#if UNIX_VARARGS
#include <varargs.h>                   /* variable argument macros          */
#else
#include <stdarg.h>                    /* variable argument macros          */
#endif
#include <stdio.h>                     /* standard I/O library              */
#include "interp.h"

specifier global_fn;

/* SETL2 provides these functions */

#if UNIX_VARARGS
char *setl2_callback();                /* call back to SETL2                */
void abend();                          /* abort program                     */
#else
char *setl2_callback(char *, ...);     /* call back to SETL2                */
void abend(SETL_SYSTEM_PROTO char *, ...);               /* abort program                     */
#endif

char *setl2_callout(
   SETL_SYSTEM_PROTO
   int service,                        /* service selector                  */
   int argc,                           /* length of argument vector         */
   char **argv)                        /* argument vector                   */

{
char *return_arg[5];                   /* return arguments                  */
int i,j;                               /* temporary looping variables       */

   switch (service) {

      case 0:
      	
      	break;

      /*
       *  This service echos the arguments through a callback.  It is
       *  just to test callout and callback.
       */

      case -32766 :

         /* return our arguments in groups of 5 */

         for (i = 0; i < argc; i += 5) {

            for (j = 0; j < 5; j++) {

               if (i + j < argc)
                  return_arg[j] = argv[i + j];
               else
                  return_arg[j] = NULL;

            }

            setl2_callback("return_args",return_arg[0],
                                         return_arg[1],
                                         return_arg[2],
                                         return_arg[3],
                                         return_arg[4],
                                         NULL);

         }

         return "done with echo test";

      /*
       *  That's all the skeleton accepts.
       */

      default:

         abend(SETL_SYSTEM "Invalid service to callout => %d\n",service);

   }

   return NULL;

}
