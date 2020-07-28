
#ifndef ACTIVEXOBJECTS_LOADED

#ifdef WIN32
#include <windows.h>


struct setl_ax {                      /* Native Object Structure           */
   int32 use_count;                   /* Reference Count                   */
   int32 type;                        /* Encodes Type and Subtype          */
   void *lpDispatch;
   
};

#endif
#ifdef SHARED
int ax_type;
#else
extern int ax_type;
#endif

#define ACTIVEXOBJECTS_LOADED 1
#endif


