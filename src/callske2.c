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
#include <string.h>                    /* string header file                */
#include "interp.h"

/* return structure for Jack's version */

struct return_struct {
   long rs_length;
   char *rs_data;
};

/* SETL2 provides these functions */

#if UNIX_VARARGS
char *setl2_callback2();               /* call back to SETL2                */
void abend();                          /* abort program                     */
#else
char *setl2_callback2(struct return_struct *, ...);
                                       /* call back to SETL2                */
void abend(SETL_SYSTEM_PROTO char *, ...);               /* abort program                     */
#endif

struct return_struct *setl2_callout2(
   SETL_SYSTEM_PROTO
   int service,                        /* service selector                  */
   int argc,                           /* length of argument vector         */
   char **argv)                        /* argument vector                   */

{
struct return_struct return_arg[5];    /* return arguments                  */
int i,j;                               /* temporary looping variables       */
static struct return_struct rs;

   switch (service) {

      /*
       *  This service echos the arguments through a callback.  It is
       *  just to test callout and callback.
       */

      case -32766 :

         /* return our arguments in groups of 5 */

         for (i = 0; i < argc; i += 5) {

            for (j = 0; j < 5; j++) {

               if (i + j < argc) {
                  return_arg[j].rs_data = argv[i + j];
                  return_arg[j].rs_length = strlen(argv[i + j]);
               }
               else
                  return_arg[j].rs_length = 0;

            }

            setl2_callback2(&return_arg[0],
                            &return_arg[1],
                            &return_arg[2],
                            &return_arg[3],
                            &return_arg[4],
                            NULL);

         }

         rs.rs_data = "done with echo test";
         rs.rs_length = strlen(rs.rs_data);
         return &rs;

      /*
       *  That's all the skeleton accepts.
       */

      default:

         abend(SETL_SYSTEM "Invalid service to callout => %d\n",service);

   }

   return NULL;
}
