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
 *  \package{File Names}
 *
 *  Throughout this system we have gone to a lot of effort to code in a
 *  portable style of C.  Unfortunately, this is not possible when it
 *  comes to file name handling, unless we do not wish to provide
 *  conveniences for the user in this area.  We do wish to provide those
 *  conveniences, so we accept the non-portability.  We have tried to
 *  isolate functions which are intimately linked to file names in this
 *  package.  There are necessarily different versions of each function
 *  for each operating system we support.
 *
 *  \texify{filename.h}
 *
 *  \packagebody{File Names}
\*/


/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "filename.h"                  /* file name functions               */

/* standard C header files */
#ifdef WIN32
#include <dos.h>                       /* DOS emulation stuff               */
#include <direct.h>                    /* directory macros                  */
#include <io.h>
#endif

#if WATCOM
#include <dos.h>                       /* DOS emulation stuff               */
#include <direct.h>                    /* directory macros                  */
#endif
#include <ctype.h>                     /* character macros                  */
#if VMS
#include <descrip.h>                   /* descriptor definition             */
#endif

#if MACINTOSH && __MWERKS__
#include <stdio.h>
int access(const char *fileName, int i);
#endif

#if WATCOM

/* workpile structure */

struct workpile_item {                 /* workpile structure                */
   char wp_front[PATH_LENGTH + 5];     /* front (traversed) part of path    */
   char wp_back[PATH_LENGTH + 5];      /* back (untraversed) part of path   */
   struct workpile_item *wp_next;      /* next item in list                 */
};

/* package-global data */

static struct workpile_item *wp_head = NULL;
                                       /* first item in specifier list      */

#endif

/*\
 *  \function{expand\_filename() (Watcom version)}
 *
 *  This function expands a file name into a unique form by prepending
 *  the drive and path from the root to the file.  Generally, we prefer
 *  file names to be in this form, for storage in libraries (we would not
 *  want a file name to change depending on the current directory at the
 *  time of an execution) and to avoid puting files into tables more than
 *  once with different names.
 *
 *  All we do here is prepend the drive and current directory, if needed.
 *  We'll use the library functions available in the Watcom library.
\*/

#if WATCOM

void expand_filename(
   SETL_SYSTEM_PROTO
   char *source_filename)              /* file name to be expanded          */

{
char target_filename[PATH_LENGTH + 1];
                                       /* working target name               */
unsigned drive, maxdrives;             /* current drive                     */
char *s,*t;                            /* temporary looping variables       */

   /* first we copy from source to target */

   s = source_filename;
   t = target_filename;

   /* save the current drive */

#ifdef WIN32
   drive=_getdrive();
#else
   _dos_getdrive(&drive);
#endif

   /* check for change in drive */

   if (strlen(source_filename) >= 2 && *(s + 1) == ':') {

      if (isupper(*s))
         *s = tolower(*s);

#ifdef WIN32
      _chdrive((unsigned)(*s - 'a' + 1));
#else
      _dos_setdrive((unsigned)(*s - 'a' + 1), &maxdrives);
#endif

      *t++ = *s++;
      *t++ = *s++;

   }
   else {

      *t++ = 'a' + drive - 1;
      *t++ = ':';

   }

   /*
    *  At this point, the current drive is the same as the source name.
    *  If the name doesn't start from the root, add the current directory.
    *
    *  Note:  Unlike Lattice, Microsoft, and others, Watcom includes the
    *  drive.
    */

   if (*s != '\\') {

      if (getcwd(t,PATH_LENGTH - 2) == NULL)
         giveup(SETL_SYSTEM msg_no_dir_space);

      for (; *(t+2); *t = *(t+2), t++);
      *t = '\0';
      if (*(t - 1) != '\\')
         *t++ = '\\';

   }
   else {

      *t++ = '\\';
      *t = '\0';
      s++;

   }

   /* replace the current drive */

#ifdef WIN32
   _chdrive(drive);
#else
   _dos_setdrive(drive, &maxdrives);
#endif

   /* copy the file name, correcting for period directories */

   while (*s && !(isspace(*s))) {

      /* .. directories indicate parent */

      if (*s == '.' &&
          *(s + 1) == '.' &&
          (!*(s + 2) || *(s + 2) == '\\')) {

         t--;
         if (*t == '\\')
            t--;
         while (t >= target_filename && *t != '\\' && *t != ':')
            t--;
         t++;
         s += 3;

      }

      /* . directories can be discarded */

      else if (*s == '.' &&
               (!*(s + 1) || *(s + 1) == '\\')) {

         s += 2;

      }

      /* we copy anything else */

      else {

         while (*s && *s != '\\')
            *t++ = *s++;
         if (*s)
            *t++ = *s++;

      }
   }

   *t = '\0';

   /* copy the target back to the source, since we expand in place */

   strcpy(source_filename,target_filename);
   strlwr(source_filename);
  
}

#endif

/*\
 *  \function{expand\_filename() (Unix version)}
 *
 *  The Unix version of \verb"expand_filename()" is very similar to the
 *  MS-DOS version.  The only difference is that we don't have to worry
 *  about the drive.
\*/

#if UNIX

void expand_filename(
   SETL_SYSTEM_PROTO
   char *source_filename)              /* file name to be expanded          */

{
char target_filename[PATH_LENGTH + 1]; /* working target name               */
char *s, *t;                           /* temporary looping variables       */

   /* first we copy from source to target */

   s = source_filename;
   t = target_filename;

    /* if the name doesn't start from the root, add the current directory */

   if (*s != '/') {

#ifdef WIN32
      if (_getcwd(t,PATH_LENGTH) == NULL)
#else     
      if (getcwd(t,PATH_LENGTH) == NULL)
#endif
         giveup(SETL_SYSTEM msg_no_dir_space);

      while (*t) t++;
      if (*(t-1) != '/')
         *t++ = '/';

   }

   /* copy the file name, correcting for period directories */

   while (*s && !isspace(*s)) {

      /* .. directories indicate parent */

      if (*s == '.' &&
          *(s + 1) == '.' &&
          (!*(s + 2) || *(s + 2) == '/')) {

         t--;
         if (*t == '/')
            t--;
         while (*t != '/' && *t != ':')
            t--;
         t++;
         s += 3;

      }

      /* . directories can be discarded */

      else if (*s == '.' &&
               (!*(s + 1) || *(s + 1) == '/')) {

         s += 2;

      }

      /* we copy anything else */

      else {

         while (*s && *s != '\\')
            *t++ = *s++;
         if (*s)
            *t++ = *s++;

      }
   }

   *t = '\0';

   /* copy the target back to the source, since we expand in place */

   strcpy(source_filename,target_filename);

}

#endif

/*\
 *  \function{expand\_filename() (VMS version)}
 *
 *  I am lazy right now with the VMS version of \verb"expand_filename()".
 *  I just call a VMS service to do the expansion.  This will work fine
 *  if the file exists, but fail if it does not.  We only need worry
 *  about existing files for the SETL2 system, but someday I should write
 *  a more robust function.
\*/

#if VMS

void expand_filename(
   SETL_SYSTEM_PROTO
   char *source_filename)              /* file name to be expanded          */

{
int status;                            /* return from system calls          */
struct dsc$descriptor_s d_fwild, d_ffound;
                                       /* wild card name and resolved name  */
static unsigned int32 context;         /* used by system routines           */
static char fixed_name[PATH_LENGTH + 1];
                                       /* resolved name                     */
char *p;                               /* temporary looping variable        */

   /* set up for a system service call */

   d_fwild.dsc$w_length = strlen(source_filename);
   d_fwild.dsc$a_pointer = source_filename;
   d_fwild.dsc$b_class = DSC$K_CLASS_S;
   d_fwild.dsc$b_dtype = DSC$K_DTYPE_T;

   context = 0L;

   d_ffound.dsc$w_length = PATH_LENGTH;
   d_ffound.dsc$a_pointer = fixed_name;
   d_ffound.dsc$b_class = DSC$K_CLASS_S;
   d_ffound.dsc$b_dtype = DSC$K_DTYPE_T;

   /* try to find a matching file name */

   status = LIB$FIND_FILE (&d_fwild, &d_ffound, &context);
   status = status & 1;
   LIB$FIND_FILE_END(&context);

   /* if we found it, copy to source */

   if (status != 0) {

      fixed_name[d_ffound.dsc$w_length] = '\0';

      for (p = fixed_name + d_ffound.dsc$w_length - 1;
           p > fixed_name && *p == ' ';
           *p-- = '\0');

      for (;
           p >= fixed_name && *p != ';';
           p--);

      if (p >= fixed_name)
         *p-- = '\0';

      if (p >= fixed_name && *p == '.')
         *p = '\0';

      strcpy(source_filename,fixed_name);

   }
}

#endif

#if MACINTOSH && __MWERKS__

int access(const char *fileName, int i)
{
        FILE *file;

        file = fopen(fileName,"r");

        if (file) {
                fclose(file);
                return 0;
        }

        return 1;
}

#endif

/*\
 *  \function{expand\_filename() (Macintosh version)}
\*/

#if MACINTOSH && !__MWERKS__ /* Already defined there ... */

#include <IOCtl.h>                     /* MPW tool stuff                    */
#include <fcntl.h>                     /* low level I/O                     */

void getcwd(
   char buffer[])                      /* return buffer                     */

{
static char savecwd[PATH_LENGTH] = ""; /* saved cwd                         */
char dummyname[PATH_LENGTH];           /* dummy file name                   */
int dummy;                             /* dummy file handle                 */
int i;                                 /* temporary looping variable        */

   /* if we've already found a working directory, return */

   if (savecwd[0] != '\0') {
      strcpy(buffer,savecwd);
      return;
   }

   /* find a dummy file name we can create */

   for (i = 0;;i++) {

     sprintf(dummyname,"dummy.%d",i);
     if (access(dummyname,0) != 0)
        break;

   }

   /* open the file, just to get a descriptor! */

   dummy = open(dummyname,O_WRONLY | O_CREAT);
#ifndef __MWERKS__
   ioctl(dummy,FIOFNAME,(long *)(&savecwd));
#endif
   close(dummy);
   unlink(dummyname);

   /* strip off the dummy name */

   savecwd[strlen(savecwd)-strlen(dummyname)] = '\0';

   strcpy(buffer,savecwd);
   return;

}

#endif

#if MACINTOSH

void expand_filename(
   SETL_SYSTEM_PROTO
   char *source_filename)              /* file name to be expanded          */

{
char target_filename[PATH_LENGTH + 1]; /* working target name               */
char *s, *t;                           /* temporary looping variables       */

   /* first we copy from source to target */

   s = source_filename;
   t = target_filename;

   /* if the name doesn't start from the root, add the current directory */

   for (s = source_filename; *s && *s != PATH_SEP; s++);
   if (s == source_filename || *s == '\0') {

      getcwd(target_filename,PATH_LENGTH);

      while (*t) {
         if (isupper(*t)) {
            *t = tolower(*t);
         }
         t++;
      }

      if (*(t-1) != PATH_SEP)
         *t++ = PATH_SEP;

   }

   /* copy the file name, correcting for period directories */

   s = source_filename;
   if (*s == PATH_SEP)
      s++;

   while (*s) {

      /* .. directories indicate parent */

      if (*s == '.' &&
          *(s + 1) == '.' &&
          (!*(s + 2) || *(s + 2) == PATH_SEP)) {

         t--;
         if (*t == PATH_SEP)
            t--;
         while (*t != PATH_SEP)
            t--;
         t++;
         s += 3;

      }

      /* . directories can be discarded */

      else if (*s == '.' &&
               (!*(s + 1) || *(s + 1) == PATH_SEP)) {

         s += 2;

      }

      /* we copy anything else */

      else {

         while (*s && *s != PATH_SEP) {
            if (isupper(*s)) {
               *t = tolower(*s);
            }
            else {
               *t = *s;
            }
            s++;
            t++;
         }

         if (*s)
            *t++ = *s++;

      }
   }

   *t = '\0';

   /* copy the target back to the source, since we expand in place */

   strcpy(source_filename,target_filename);

}

#endif

/*\
 *  \function{get\_filelist() (Watcom version)}
 *
 *  This function gets a list of files matching a string of filespecs.
 *  The filespecs may have the wildcards \verb"*" and \verb"?" in any
 *  node of a filespec.
 *
 *  The MS-DOS version of this function is a little tricky.  MS-DOS will
 *  not automatically expand wildcards unless they are in the final node
 *  of a path.  We have chosen to give some compatibility with Unix
 *  systems, by expanding wildcards at any point.  To do this, we use a
 *  workpile of partially expanded files (an alternative is to use
 *  recursion, but we would be keeping many copies of long character
 *  strings on the stack in that case, and we haven't stack space to
 *  throw away!).
 *
 *  We use the Watcom directory search functions here.
\*/

#if WATCOM

filelist_ptr_type setl_get_filelist(
   SETL_SYSTEM_PROTO
   char *specifier_list)               /* list of file specifiers           */

{
filelist_ptr_type return_ptr;          /* returned file list                */
filelist_ptr_type *filelist_tail;      /* tail of returned file list        */
struct workpile_item *wp_current;      /* current (active) workpile item    */
struct workpile_item **wp_tail;        /* tail of current workpile list     */
#ifdef WIN32
struct _finddata_t fileinfo;                /* file information block            */
#else
struct find_t fileinfo;                /* file information block            */
#endif
int is_wild;                           /* YES if current node has wildcards */
char *s,*t;                            /* temporary looping variables       */
#ifdef WIN32
long hFile=0;
#endif

   /*
    *  The first phase of this function separates the file specification
    *  list into single file specifications delimited by spaces or
    *  semicolons.  It places each file specification it finds on a
    *  workpile, to be expanded by the second phase.
    */

   s = specifier_list;
   wp_tail = &wp_head;

   /* create a workpile entry for each file specifier in the list */

   while (*s) {

      /* skip past white space */

      while (*s && (isspace(*s) || *s == ';'))
         s++;
      if (!*s)
         break;

      /* allocate a new item, and add it to the workpile */

      wp_current = (struct workpile_item *)malloc(
                    sizeof(struct workpile_item));
      if (wp_current == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);

      *wp_tail = wp_current;
      wp_tail = &(wp_current->wp_next);
      wp_current->wp_next = NULL;

      wp_current->wp_front[0] = '\0';

      /* pick out the next file specifier */

      for (t = wp_current->wp_back;
           *s && *s != ';';
           *t++ = *s++);
      *t = '\0';

      /* add the current drive and directory, if necessary */

      expand_filename(SETL_SYSTEM wp_current->wp_back);

   }

   /*
    *  At this point we are finished with the first phase and ready to
    *  begin the second.  We pick up items from the workpile, and expand
    *  them.  We move file nodes from the back to the front, until we
    *  reach the end of a string or a node with wildcards.  If we find
    *  wildcards, we create new workpile items by expanding the wildcards
    *  with DOS searches.
    */

   /* start a chain of file names */

   return_ptr = NULL;
   filelist_tail = &return_ptr;

   /* keep looping while the workpile isn't empty */

   while (wp_head != NULL) {

      /* we will copy nodes from back to front */

      wp_current = wp_head;
      for (t = wp_current->wp_front; *t; t++);
      s = wp_current->wp_back;

      for (;;) {

         /* pick the next node off of the back, append it to the front */

         is_wild = NO;
         while (*s && *s != '\\' &&
                t <= wp_current->wp_front + PATH_LENGTH) {

            if (*s == '*' || *s == '?') {
                is_wild = YES;
            }

            *t++ = *s++;

         }
         *t = '\0';
         if (t > wp_current->wp_front + PATH_LENGTH)
            giveup(SETL_SYSTEM msg_bad_file_path,
                   wp_current->wp_front,PATH_LENGTH);

         /* if there weren't any wildcards in this node ... */

         if (!is_wild) {

            /*
             *  if we're not at the end of a string, copy the backslash
             *  and cycle to get the next node
             */

            if (*s) {

               *t++ = *s++;
               continue;

            }

            /* if the file exists and is normal, append it to the list */

#ifdef WIN32
            if ((hFile=_findfirst(wp_current->wp_front,
                               &fileinfo)) != -1L &&
               !(fileinfo.attrib & (_A_SYSTEM | _A_SUBDIR))) {
               
#else
            if (_dos_findfirst(wp_current->wp_front, _A_NORMAL,
                               &fileinfo) == 0 &&
               !(fileinfo.attrib & (_A_SYSTEM | _A_VOLID | _A_SUBDIR))) {
#endif

               *filelist_tail = (filelist_ptr_type)
                               malloc(sizeof(struct filelist_item));
               if (*filelist_tail == NULL)
                  giveup(SETL_SYSTEM msg_malloc_error);

               strcpy((*filelist_tail)->fl_name,wp_current->wp_front);
               strlwr((*filelist_tail)->fl_name);
               filelist_tail = &((*filelist_tail)->fl_next);
               *filelist_tail = NULL;

            }

            /* remove the current workpile item */

            wp_head = wp_current->wp_next;
            free(wp_current);

            break;

         }

         /*
          *  At this point we have a node with wildcard characters, so we
          *  will have to perform a search.
          */

         /* we add items to the front of the workpile */

         wp_tail = &wp_head;

         /* call DOS to start search */

#ifdef WIN32
            if ((hFile=_findfirst(wp_current->wp_front,
                               &fileinfo)) != -1L ) {

#else
            if (_dos_findfirst(wp_current->wp_front, _A_NORMAL,
                               &fileinfo) == 0) {
#endif

            /* back up to the start of the node */

            while (*t != '\\')
               t--;
            *(++t) = '\0';

            do {

               if (strcmp(fileinfo.name,".") == 0)
                  continue;
               if (strcmp(fileinfo.name,"..") == 0)
                  continue;

               if (*s) {

                  /*
                   *  if we have more nodes to process, only accept
                   *  directories
                   */

                  if (!(fileinfo.attrib & _A_SUBDIR))
                     continue;

                  /* make a new workpile item */

                  *wp_tail = (struct workpile_item *)malloc(
                           sizeof(struct workpile_item));
                  if (*wp_tail == NULL)
                     giveup(SETL_SYSTEM msg_malloc_error);

                  strcpy((*wp_tail)->wp_front,wp_current->wp_front);
                  strcat((*wp_tail)->wp_front,fileinfo.name);
                  strcpy((*wp_tail)->wp_back,s);
                  wp_tail = &((*wp_tail)->wp_next);

               }
               else {

                  /* there are no more nodes, so create a file list item */

#ifdef WIN32
                  if (fileinfo.attrib & (_A_SYSTEM | _A_SUBDIR))
#else
                  if (fileinfo.attrib & (_A_SYSTEM | _A_VOLID | _A_SUBDIR))
#endif
                     continue;

                  *filelist_tail = (filelist_ptr_type)
                                  malloc(sizeof(struct filelist_item));
                  if (*filelist_tail == NULL)
                     giveup(SETL_SYSTEM msg_malloc_error);

                  strcpy((*filelist_tail)->fl_name,wp_current->wp_front);
                  strcat((*filelist_tail)->fl_name,fileinfo.name);
                  strlwr((*filelist_tail)->fl_name);

                  filelist_tail = &((*filelist_tail)->fl_next);
                  *filelist_tail = NULL;

               }

#ifdef WIN32
            } while (_findnext(hFile,&fileinfo) == 0);
            _findclose(hFile);
#else
            } while (_dos_findnext(&fileinfo) == 0);
#endif

         }

         /* remove the current workpile item */

         *wp_tail = wp_current->wp_next;
         free(wp_current);

         break;

      }
   }

   return return_ptr;

}

#endif

/*\
 *  \function{get\_filelist() (Unix version)}
 *
 *  This function gets a list of files matching a string of filespecs.
 *  The filespecs may have the wildcards \verb"*" and \verb"?" in any
 *  node of a filespec.
 *
 *  I'm not really happy with this implementation, but it will do for
 *  now.  When we discover wildcard characters in a file specification,
 *  we fork an \verb"ls" command to return the list of matching files.
 *  We probably should use library system calls, as we did for MS-DOS,
 *  but I can't find them in the man pages right now.  This works, and
 *  may never be necessary since the Unix shell will do the expansion
 *  for us in most cases.
\*/

#if UNIX

filelist_ptr_type setl_get_filelist(
   SETL_SYSTEM_PROTO
   char *specifier_list)               /* list of file specifiers           */

{
filelist_ptr_type return_ptr;          /* returned file list                */
filelist_ptr_type *filelist_tail;      /* tail of returned file list        */
char name_buffer[PATH_LENGTH + 1];     /* file name buffer                  */
int is_wild;                           /* YES if a path name has wildcards  */
char ls_command[PATH_LENGTH + 10];     /* ls command (for popen)            */
FILE *ls_file;                         /* stdout from ls                    */
char *s,*t;                            /* temporary looping variables       */

   /* start a chain of file names */
#ifdef WIN32
   return NULL;
#else

   return_ptr = NULL;
   filelist_tail = &return_ptr;

   /* process each specifier in the list */

   s = specifier_list;
   while (*s) {

      /* skip past white space */

      while (*s && (isspace(*s) || *s == ';'))
         s++;
      if (!*s)
         break;

      /* pick out the next file specifier */

      for (t = name_buffer;
           *s && *s != ';';
           *t++ = *s++);
      *t = '\0';

      /* add the current directory, if necessary */

      expand_filename(SETL_SYSTEM name_buffer);

      /* check for wildcards in the path */

      is_wild = NO;
      for (t = name_buffer; *t; t++)
         if (*t == '*' || *t == '?')
            is_wild = YES;

      /* if we have no wild cards, just add the name to the list */

      if (!is_wild) {

         if (os_access(name_buffer,0) != 0)
             continue;

         *filelist_tail = (filelist_ptr_type)
                         malloc(sizeof(struct filelist_item));
         if (*filelist_tail == NULL)
            giveup(SETL_SYSTEM msg_malloc_error);

         strcpy((*filelist_tail)->fl_name,name_buffer);

         filelist_tail = &((*filelist_tail)->fl_next);
         *filelist_tail = NULL;

         continue;

      }

      /* we did have wild cards, so we have to do a search */

      sprintf(ls_command,"/bin/ls -d -p %s",name_buffer);

      ls_file = popen(ls_command,"r");

      while (fgets(name_buffer,PATH_LENGTH,ls_file) != NULL) {

         /* remove the newline at the end of the name */

         t = name_buffer + strlen(name_buffer) - 1;
         *t-- = '\0';

         /* skip directories and executable files */

         if (*t == '/' || *t == '*')
            continue;

         /* build a new list item */

         *filelist_tail = (filelist_ptr_type)
                         malloc(sizeof(struct filelist_item));
         if (*filelist_tail == NULL)
            giveup(SETL_SYSTEM msg_malloc_error);

         strcpy((*filelist_tail)->fl_name,name_buffer);

         filelist_tail = &((*filelist_tail)->fl_next);
         *filelist_tail = NULL;

      }

      pclose(ls_file);

   }

   return return_ptr;

#endif
}

#endif

/*\
 *  \function{get\_filelist() (VMS version)}
 *
 *  This function gets a list of files matching a string of filespecs.
 *  The filespecs may have the wildcards \verb"*" and \verb"?" in any
 *  node of a filespec.
 *
 *  I'm not really happy with this implementation, but it will do for
 *  now.  When we discover wildcard characters in a file specification,
 *  we fork an \verb"ls" command to return the list of matching files.
 *  We probably should use library system calls, as we did for MS-DOS,
 *  but I can't find them in the man pages right now.  This works, and
 *  may never be necessary since the Unix shell will do the expansion
 *  for us in most cases.
\*/

#if VMS

filelist_ptr_type setl_get_filelist(
   SETL_SYSTEM_PROTO
   char *specifier_list)               /* list of file specifiers           */

{
filelist_ptr_type return_ptr;          /* returned file list                */
filelist_ptr_type *filelist_tail;      /* tail of returned file list        */
char name_buffer[PATH_LENGTH + 1];     /* file name buffer                  */
char fixed_name[PATH_LENGTH + 1];      /* resolved name buffer              */
int status;                            /* return from system calls          */
struct dsc$descriptor_s d_fwild, d_ffound;
                                       /* wild card name and resolved name  */
static unsigned int32 context;         /* used by system routines           */
char *s,*t;                            /* temporary looping variables       */

   /* start a chain of file names */

   return_ptr = NULL;
   filelist_tail = &return_ptr;

   /* process each specifier in the list */

   s = specifier_list;
   while (*s) {

      /* skip past white space */

      while (*s && isspace(*s))
         s++;
      if (!*s)
         break;

      /* pick out the next file specifier */

      for (t = name_buffer;
           *s && *s != ';';
           *t++ = *s++);
      *t = '\0';

      /* use system calls to expand the file name */

      d_fwild.dsc$w_length = strlen(name_buffer);
      d_fwild.dsc$a_pointer = name_buffer;
      d_fwild.dsc$b_class = DSC$K_CLASS_S;
      d_fwild.dsc$b_dtype = DSC$K_DTYPE_T;

      context = 0L;

      for (;;) {

         d_ffound.dsc$w_length = PATH_LENGTH;
         d_ffound.dsc$a_pointer = fixed_name;
         d_ffound.dsc$b_class = DSC$K_CLASS_S;
         d_ffound.dsc$b_dtype = DSC$K_DTYPE_T;

         status = LIB$FIND_FILE (&d_fwild, &d_ffound, &context);
         status = status & 1;

         if (status == 0) {

            LIB$FIND_FILE_END(&context);
            break;

         }

         /* remove trailing version and period */

         fixed_name[d_ffound.dsc$w_length] = '\0';

         for (t = fixed_name + d_ffound.dsc$w_length - 1;
              t > fixed_name && *t == ' ';
              *t-- = '\0');

         for (;
              t >= fixed_name && *t != ';';
              t--);

         if (t >= fixed_name)
            *t-- = '\0';

         if (t >= fixed_name && *t == '.')
            *t = '\0';

         /* build a new list item */

         *filelist_tail = (filelist_ptr_type)
                         malloc(sizeof(struct filelist_item));
         if (*filelist_tail == NULL)
            giveup(SETL_SYSTEM msg_malloc_error);

         strcpy((*filelist_tail)->fl_name,fixed_name);

         filelist_tail = &((*filelist_tail)->fl_next);
         *filelist_tail = NULL;

      }
   }

   return return_ptr;

}

#endif

/*\
 *  \function{get\_filelist() (MS-DOS version)}
 *
 *  This function gets a list of files matching a string of filespecs.
 *  The filespecs may have the wildcards \verb"*" and \verb"?" in any
 *  node of a filespec.
 *
 *  The MS-DOS version of this function is a little tricky.  MS-DOS will
 *  not automatically expand wildcards unless they are in the final node
 *  of a path.  We have chosen to give some compatibility with Unix
 *  systems, by expanding wildcards at any point.  To do this, we use a
 *  workpile of partially expanded files (an alternative is to use
 *  recursion, but we would be keeping many copies of long character
 *  strings on the stack in that case, and we haven't stack space to
 *  throw away!).
\*/

#if MACINTOSH

filelist_ptr_type setl_get_filelist(
   SETL_SYSTEM_PROTO
   char *specifier_list)               /* list of file specifiers           */

{
filelist_ptr_type return_ptr;          /* returned file list                */
filelist_ptr_type *filelist_tail;      /* tail of returned file list        */
char *s,*t;                            /* temporary looping variables       */

   /*
    *  The first phase of this function separates the file specification
    *  list into single file specifications delimited by spaces or
    *  semicolons.  It places each file specification it finds on a
    *  workpile, to be expanded by the second phase.
    */

   /* start a chain of file names */

   return_ptr = NULL;
   filelist_tail = &return_ptr;
   s = specifier_list;

   /* create a workpile entry for each file specifier in the list */

   while (*s) {

      /* skip past white space */

      while (*s && (isspace(*s) || *s == ';'))
         s++;
      if (!*s)
         break;

      *filelist_tail = (filelist_ptr_type)
                      malloc(sizeof(struct filelist_item));
      if (*filelist_tail == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);

      for (t = (*filelist_tail)->fl_name;
           *s && *s != ';';
           *t++ = *s++);
      *t = '\0';

      /* add the current drive and directory, if necessary */

      expand_filename(SETL_SYSTEM (*filelist_tail)->fl_name);

      filelist_tail = &((*filelist_tail)->fl_next);
      *filelist_tail = NULL;

   }

   return return_ptr;

}

#endif

/*\
 *  \function{free\_filelist() (all versions)}
 *
 *  This function just frees the memory used by a file list.
\*/

void setl_free_filelist(
   filelist_ptr_type fl_head)          /* list of file names                */

{
filelist_ptr_type to_delete;           /* item to be deleted                */

   while (fl_head != NULL) {

      to_delete = fl_head;
      fl_head = fl_head->fl_next;
      free(to_delete);

   }
}

/*\
 *  \function{get\_tempname() (all versions)}
 *
 *  This function finds an unused temporary file name with a given
 *  prefix.  It just keeps trying file names with the prefix, the process
 *  identification, and a unique suffix.
\*/

void get_tempname(
   SETL_SYSTEM_PROTO
   char *prefix,                       /* file name prefix (temporary path) */
   char *temp_filename)                /* file name buffer                  */

{
int pid;                               /* process identification            */
static char suffix[3] = "aa";          /* unique file name suffix           */

   /* get the process identification */

   pid = getpid();
   sprintf(temp_filename,"%s%d.%s",prefix,pid,suffix);

   /* keep looping until we find an unused file name */

   while (os_access(temp_filename,0) == 0) {

      /* increment the suffix */

      if (suffix[1] == 'z') {

         if (suffix[0] == 'z')
            giveup(SETL_SYSTEM msg_no_temp_file);

         suffix[0]++;
         suffix[1] = 'a';

      }
      else {

         suffix[1]++;

      }

      /* reform the file name */

      sprintf(temp_filename,"%s%d.%s",prefix,pid,suffix);

   }

   /* increment the suffix */

   if (suffix[1] == 'z') {

      if (suffix[0] == 'z')
         giveup(SETL_SYSTEM msg_no_temp_file);

      suffix[0]++;
      suffix[1] = 'a';

   }
   else {

      suffix[1]++;

   }
}


filelist_ptr_type 
setl_get_next_file(
  filelist_ptr_type current_file)
{
	return current_file->fl_next;

}

char *
setl_get_filename(
  filelist_ptr_type current_file)
{
	return current_file->fl_name;
}
