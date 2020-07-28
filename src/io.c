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
 *  \package{Input / Output Procedures}
 *
 *  In this package we have built-in procedures which are
 *  input/output oriented.  We hope that the procedures provided here are
 *  relatively temporary -- we would like some more powerful and general
 *  I/O eventually, but for now we provide procedures quite similar to
 *  those in SETL.
 *
 *  There are a few changes we have made now.  The most significant
 *  change is the use of file `handles' rather than names.  This is
 *  similar to most other languages.
 *
 *  We use atoms for file handles and keep a map of open files keyed by
 *  these handles.
\*/


/* standard C header files */

#if UNIX
#include <sys/types.h>                 /* file types                        */
#include <sys/stat.h>                  /* file status                       */
#endif
#include <math.h>                      /* math function headers             */
#include <ctype.h>                     /* character macros                  */



/* SETL2 system header files */

#include "system.h"                    /* SETL2 system constants            */
#include "interp.h"                    /* SETL2 interpreter constants       */
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "form.h"                      /* form codes                        */
#include "filename.h"                  /* file name utilities               */
#include "chartab.h"                   /* character type table              */
#include "builtins.h"                  /* built-in symbols                  */
#include "unittab.h"                   /* unit table                        */
#include "loadunit.h"                  /* unit loader                       */
#include "execute.h"                   /* core interpreter                  */
#include "abend.h"                     /* abnormal end handler              */
#include "specs.h"                     /* specifiers                        */
#include "x_integers.h"                /* integers                          */
#include "x_reals.h"                   /* real numbers                      */
#include "x_strngs.h"                  /* strings                           */
#include "sets.h"                      /* sets                              */
#include "maps.h"                      /* maps                              */
#include "tuples.h"                    /* tuples                            */
#include "objects.h"                   /* objects                           */
#include "process.h"                   /* objects                           */
#include "mailbox.h"                   /* objects                           */
#include "slots.h"                     /* procedures                        */
#include "x_files.h"                   /* files                             */

#ifdef PLUGIN
#define fprintf plugin_fprintf
#define fputs plugin_fputs
extern int javascript_full;
extern char *javascript_buffer;
#endif 

#ifdef HAVE_SYS_SOCKET_H
#include <sys/types.h>
#ifdef MACINTOSH
#include <sys/errno.h>
#else
#include <errno.h>
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
#endif

/*
 *  Some compilers, notably Lattice, can not handle a very long text
 *  segment.  This stuff is used to break up this file, so we can run
 *  through it several times.
 */

/* constants */

#define BINFLAG     "setl2bin"         /* binary file flag                  */
#define SKIP_CODE   -1                 /* skip code for sparse tuples       */

/* file modes */

#define TEXT_IN     0                  /* coded file, input mode            */
#define TEXT_OUT    1                  /* coded file, output mode           */
#define BYTE_IN     2                  /* character input mode              */
                                       /* for keyboard                      */
#define BINARY_IN   3                  /* binary file, input mode           */
#define BINARY_OUT  4                  /* binary file, output mode          */
#define RANDOM      5                  /* random string file                */

#define TCP         6                  /* TCP client sockets                */

/* read_spec return codes */

#define SPEC        0                  /* found a specifier                 */
#define RBRACKET    1                  /* found right bracket ']'           */
#define RBRACE      2                  /* found right brace '}'             */
#define ENDOFFILE   3                  /* found end of file                 */

/* package-global variables */
      
static map_h_ptr_type file_map=NULL;   /* map of open files keyed by handle */
static char *file_name;                /* file name                         */
static FILE *file_stream;              /* file stream pointer               */
static int file_fd;                    /* file descriptor for print_spec    */
static unsigned char *file_buffer;     /* buffer                            */
static unsigned char *start;           /* start position of token           */
static unsigned char *lookahead;       /* lookahead pointer                 */
static unsigned char *endofbuffer;     /* last filled position in buffer    */
static unsigned char *eof_ptr;         /* pointer to end of file character  */
static file_ptr_type stdin_ptr;        /* standard input pointer            */
static file_ptr_type reads_ptr;        /* dummy file for reads              */
static file_ptr_type file_ptr;         /* current file pointer              */
static string_c_ptr_type reads_cell;   /* current cell for reads            */
static char *reads_char_ptr, *reads_char_end;
                                       /* character pointers for reads      */
static int32 reads_length;             /* length of input string            */
static int eof_flag;                   /* YES if last read yielded eof      */
static time_t runtime;                 /* current run time                  */
static int process_id;                 /* process identifier                */
static string_h_ptr_type binstr_curr_hdr;
                                       /* binstr return string              */
static string_c_ptr_type binstr_curr_cell;
                                       /* current cell in above             */
static char *binstr_char_ptr, *binstr_char_end;
                                       /* character pointers in above       */

/* advance lookahead macro */

#define advance_la \
    {if (++lookahead > endofbuffer) fill_buffer(SETL_SYSTEM_VOID);}

/* forward function declarations */
      
static void fill_buffer(SETL_SYSTEM_PROTO_VOID);
                                       /* fill the character buffer         */
static int read_spec(SETL_SYSTEM_PROTO specifier *);
                                       /* read one item from an input file  */
static void print_spec(SETL_SYSTEM_PROTO specifier *); 
                                       /* print one item to a stream file   */
static void binstr_cat_spec(SETL_SYSTEM_PROTO specifier *);
                                       /* save one item in a binary string  */
static void binstr_cat_string(SETL_SYSTEM_PROTO char *, size_t);
                                       /* concatenate a string on binstr's  */
                                       /* return value                      */
static void unbinstr_spec(SETL_SYSTEM_PROTO specifier *);
                                       /* unbinstring one specifier         */

#ifdef HAVE_SYS_SOCKET_H

extern int errno;

int to_portnum(SETL_SYSTEM_PROTO const char *s) {
  long i,j, k;
  i = 0; while (is_white_space(s[i])) i++;
  j = i;
  if (isdigit(s[j]),10) {
    while (is_digit(s[j],10)) j++;
    k = j; while ((k<strlen(s)&&(is_white_space(s[k])))) k++;
    if (k == strlen(s)) {
      long r;
      int saved_errno = errno;
      errno = 0;
      r = strtol(&s[i],NULL,10);
      if (errno == ERANGE) {
        abend(SETL_SYSTEM "Port number too large");
      } else if (r > 65535) {
        abend(SETL_SYSTEM "Port number %ld too large",r);
      } else {
        errno = saved_errno;
        return (int)r;
      }
    } else {
      abend(SETL_SYSTEM "Junk after digits in port number");
    }
  } else {
    abend(SETL_SYSTEM "Port number must consist entirely of decimal digits");
  }
} /* end to_portnum */

struct hostent *os_findhostbyaddr(const struct in_addr *addr) {
  return gethostbyaddr((const char *)addr, sizeof (struct in_addr),
                                                             AF_INET);
}

struct hostent *os_findhostbyname(const char *name) {
  return gethostbyname(name);
}

struct hostent *find_hostent(const char *s) {
  struct hostent *r;
  struct in_addr peer;
  /* First try for a dotted address, then resort to a lookup by name */

  if ((peer.s_addr = inet_addr(s)) != INADDR_NONE) {
    r = os_findhostbyaddr(&peer);
    if (r) return r;
  }
  return os_findhostbyname(s);
} /* end find_hostent */


int os_connect(int fd, const struct sockaddr_in *addr) {
  int len = sizeof (struct sockaddr_in);
  int saved_errno = errno;
  while (1) {
    int r = connect(fd, (struct sockaddr *)addr, len);
    if (r != -1) return r;
    if (errno != EINTR) return -1;
    /* (On EINTR, retry the connect.)  */
    errno = saved_errno;
  }
}
#endif

/*\
 *  \function{open\_io()}
 *
 *  This function initializes the input / output subsystem.  It
 *  establishes the map we use to keep track of open files and sets up
 *  standard input and output.
\*/

void open_io(
   SETL_SYSTEM_PROTO_VOID)

{
int i;                                 /* temporary looping variable        */

   /* create a file map */

   get_map_header(file_map);
   file_map->m_use_count = 1;
   file_map->m_hash_code = 0;
   file_map->m_ntype.m_root.m_cardinality = 0;
   file_map->m_ntype.m_root.m_cell_count = 0;
   file_map->m_ntype.m_root.m_height = 0;
   for (i = 0;
        i < MAP_HASH_SIZE;
        file_map->m_child[i++].m_cell = NULL);

   /* set up a node for standard input */

   get_file(stdin_ptr);

   stdin_ptr->f_file_buffer =
      (unsigned char *)malloc((size_t)(FILE_BUFF_SIZE + MAX_LOOKAHEAD + 1));
   if (stdin_ptr->f_file_buffer == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   stdin_ptr->f_start = stdin_ptr->f_endofbuffer = stdin_ptr->f_file_buffer;
   stdin_ptr->f_eof_ptr = NULL;

   /* set up a node for reads */

   get_file(reads_ptr);

   reads_ptr->f_file_buffer =
      (unsigned char *)malloc((size_t)(FILE_BUFF_SIZE + MAX_LOOKAHEAD + 1));
   if (reads_ptr->f_file_buffer == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   eof_flag = NO;
   time(&runtime);
   process_id = getpid();

   return;

}

/*\
 *  \function{close\_io()}
 *
 *  This function closes the input / output subsystem.  It loops over the
 *  file map closing all open files.
\*/

void close_io(
   SETL_SYSTEM_PROTO_VOID)

{
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */

   /* if the file map is null, we never opened the package */

   if (file_map == NULL)
      return;

   /* set up to loop over the map */

   map_work_hdr = file_map;
   map_height = file_map->m_ntype.m_root.m_height;
   map_cell = NULL;
   map_index = 0;

   /* loop over the elements of source */

   for (;;) {

      /* find the next cell in the map */

      while (map_cell == NULL) {

         /* start on the next clash list, if we're at a leaf */

         if (!map_height && map_index < MAP_HASH_SIZE) {

            map_cell = map_work_hdr->m_child[map_index].m_cell;
            map_index++;
            continue;

         }

         /* move up if we're at the end of a node */

         if (map_index >= MAP_HASH_SIZE) {

            /* there are no more elements, so break */

            if (map_work_hdr == file_map)
               break;

            /* otherwise move up */

            map_height++;
            map_index =
               map_work_hdr->m_ntype.m_intern.m_child_index + 1;
            map_work_hdr =
               map_work_hdr->m_ntype.m_intern.m_parent;

            continue;

         }

         /* skip over null nodes */

         if (map_work_hdr->m_child[map_index].m_header == NULL) {

            map_index++;
            continue;

         }

         /* otherwise drop down a level */

         map_work_hdr =
            map_work_hdr->m_child[map_index].m_header;
         map_index = 0;
         map_height--;

      }

      /* if there are no more cells, break */

      if (map_cell == NULL)
         break;

      /*
       *  At this point we have a map cell.  Because of the way this map
       *  was constructed and maintained, we know that each cell has a
       *  single range value.  All we do is close the open file.
       */

      file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;
      map_cell = map_cell->m_next;

      switch (file_ptr->f_mode) {

         case TEXT_IN :

            fclose(file_ptr->f_file_stream);
            break;

         case TEXT_OUT :

            fclose(file_ptr->f_file_stream);
            break;

         case BINARY_IN :

            fclose(file_ptr->f_file_stream);
            break;

         case BINARY_OUT :

            fclose(file_ptr->f_file_stream);
            break;

         case RANDOM :

            fclose(file_ptr->f_file_stream);
            break;

         case TCP :

            close(file_ptr->f_file_fd);
            break;

      }

   }

   file_map = NULL;
   return;

}

/*\
 *  \function{setl2\_internal\_open()}
 *
\*/

void setl2_internal_open(
   SETL_SYSTEM_PROTO 
   int new_flag,                       /* 0=Old modes, 1=New Modes          */
   specifier *target,                  /* return value                      */
   char *mode_string,
   char *file_name)

{
specifier file_atom;                   /* file handle atom                  */
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
map_c_ptr_type *map_tail;              /* attach new cells here             */
int map_height, map_index;             /* current height and index          */
map_h_ptr_type new_hdr;                /* created header node               */
map_c_ptr_type new_cell;               /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int pid;                               /* process identifier                */
char flag_string[20];                  /* binary flag?                      */
time_t create_time;                    /* file creation time                */
int i;                                 /* temporary looping variable        */

   /*
    *  For text input files we use level 0 I/O and manage our own buffer.
    */

   if (strcmp(mode_string,"text-in") == 0) {

      expand_filename(SETL_SYSTEM file_name);

      if (os_access(file_name,4) != 0) {
         if (new_flag==0) {

            /* return om */

            unmark_specifier(target);
            target->sp_form = ft_omega;

            return;

         } else {

            get_file(file_ptr);
            file_ptr->f_flag = 1;

         }

      } else {

         get_file(file_ptr);
         file_ptr->f_flag = 0;

      }


      file_ptr->f_type = new_flag;
      file_ptr->f_file_fd = -1;

      file_ptr->f_mode = TEXT_IN;
      strcpy(file_ptr->f_file_name,file_name);

      file_ptr->f_file_stream = fopen(file_name,BINARY_RD);
      if ((file_ptr->f_file_stream == NULL)&&(new_flag==0)) {

         /* return om */

         unmark_specifier(target);
         target->sp_form = ft_omega;
         free_file(file_ptr);

         return;

      }

      file_ptr->f_file_buffer = (unsigned char *)malloc(
                               (size_t)(FILE_BUFF_SIZE + MAX_LOOKAHEAD + 1));
      if (file_ptr->f_file_buffer == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);

      file_ptr->f_start = file_ptr->f_endofbuffer = file_ptr->f_file_buffer;
      file_ptr->f_eof_ptr = NULL;

   }

   /*
    *  For text output files we use level 2 I/O, and let the C library do
    *  most of the work.
    */

   else if (strcmp(mode_string,"text-out") == 0) {
      FILE *tmp_file;
      expand_filename(SETL_SYSTEM file_name);

      if (SAFE_MODE==0) {

      get_file(file_ptr);

      file_ptr->f_type = new_flag;
      file_ptr->f_flag = 0;
      file_ptr->f_file_fd = -1;

      file_ptr->f_mode = TEXT_OUT;
      strcpy(file_ptr->f_file_name,file_name);

      file_ptr->f_file_stream = fopen(file_name,"w");
      if (file_ptr->f_file_stream == NULL) {

         /* return om */

         unmark_specifier(target);
         target->sp_form = ft_omega;
         free_file(file_ptr);

         return;

        } else {
          tmp_file = fopen(file_name,"w");
          fclose(tmp_file);
        }
      }
   }

   /*
    *  For text output files we use level 2 I/O, and let the C library do
    *  most of the work.
    */

   else if ((strcmp(mode_string,"text-append") == 0)&&(SAFE_MODE==0)) {

      expand_filename(SETL_SYSTEM file_name);

      get_file(file_ptr);

      file_ptr->f_type = new_flag;
      file_ptr->f_flag = 0;
      file_ptr->f_file_fd = -1;

      file_ptr->f_mode = TEXT_OUT;
      strcpy(file_ptr->f_file_name,file_name);

      file_ptr->f_file_stream = fopen(file_name,"a");
      if (file_ptr->f_file_stream == NULL) {

         /* return om */

         unmark_specifier(target);
         target->sp_form = ft_omega;
         free_file(file_ptr);

         return;

      }
   }

   /*
    *  For binary input files we use level 0 I/O, and let the C library
    *  do the buffering.
    */

   else if (strcmp(mode_string,"binary-in") == 0) {

      expand_filename(SETL_SYSTEM file_name);
      if (os_access(file_name,4) != 0) {

         /* return om */

         file_ptr->f_type = new_flag;
         file_ptr->f_flag = 0;
         file_ptr->f_file_fd = -1;

         unmark_specifier(target);
         target->sp_form = ft_omega;

         return;

      }

      get_file(file_ptr);

      file_ptr->f_mode = BINARY_IN;
      strcpy(file_ptr->f_file_name,file_name);

      file_ptr->f_file_stream = fopen(file_name,BINARY_RD);
      if (file_ptr->f_file_stream == NULL) {

         /* return om */

         unmark_specifier(target);
         target->sp_form = ft_omega;
         free_file(file_ptr);

         return;

      }

      /* check file for binary */

      if (fread((void *)flag_string,
                sizeof(char),
                (size_t)strlen(BINFLAG),
                file_ptr->f_file_stream) != (size_t)strlen(BINFLAG))
         abend(SETL_SYSTEM "Read error on file => %s\n",file_name);
      flag_string[strlen(BINFLAG)] = '\0';
      if (strcmp(flag_string,BINFLAG) != 0)
         abend(SETL_SYSTEM msg_file_not_binary,file_name);

      /* check whether all binary values can be read */

      file_ptr->f_samerun = YES;

      if (fread((void *)&pid,
               sizeof(int),
               (size_t)1,
               file_ptr->f_file_stream) != (size_t)1)
         abend(SETL_SYSTEM "Read error on file => %s\n",file_name);
      if (pid != process_id)
         file_ptr->f_samerun = NO;

      if (fread((void *)&create_time,
                sizeof(time_t),
                (size_t)1,
                file_ptr->f_file_stream) != (size_t)1)
         abend(SETL_SYSTEM "Read error on file => %s\n",file_name);
      if (create_time != runtime)
         file_ptr->f_samerun = NO;

   }

   /*
    *  For binary output files we use level 0 I/O, and let the C library
    *  do the buffering.
    */

   else if ((strcmp(mode_string,"binary-out") == 0)&&(SAFE_MODE==0)) {

      expand_filename(SETL_SYSTEM file_name);

      get_file(file_ptr);

      file_ptr->f_type = new_flag;
      file_ptr->f_flag = 0;
      file_ptr->f_file_fd = -1;

      file_ptr->f_mode = BINARY_OUT;
      strcpy(file_ptr->f_file_name,file_name);

      file_ptr->f_file_stream = fopen(file_name,BINARY_WR);
      if (file_ptr->f_file_stream == NULL) {

         /* return om */

         unmark_specifier(target);
         target->sp_form = ft_omega;
         free_file(file_ptr);

         return;

      }

      /* mark file as binary and time stamp */

      if (fwrite((void *)BINFLAG,
                 sizeof(char),
                 (size_t)strlen(BINFLAG),
                 file_ptr->f_file_stream) != (size_t)strlen(BINFLAG))
         abend(SETL_SYSTEM "Write error on file => %s\n",file_name);

      if (fwrite((void *)&process_id,
                 sizeof(int),
                 (size_t)1,
                 file_ptr->f_file_stream) != (size_t)1)
         abend(SETL_SYSTEM "Write error on file => %s\n",file_name);

      if (fwrite((void *)&runtime,
                 sizeof(time_t),
                 (size_t)1,
                 file_ptr->f_file_stream) != (size_t)1)
         abend(SETL_SYSTEM "Write error on file => %s\n",file_name);

   }

   /*
    *  For random files we let Jack do most of the work!
    */

   else if (strcmp(mode_string,"random") == 0) {

      expand_filename(SETL_SYSTEM file_name);

      get_file(file_ptr);

      file_ptr->f_type = new_flag;
      file_ptr->f_flag = 0;
      file_ptr->f_file_fd = -1;

      file_ptr->f_mode = RANDOM;
      strcpy(file_ptr->f_file_name,file_name);

      file_ptr->f_file_stream = fopen(file_name,BINARY_RDWR);
      if (file_ptr->f_file_stream == NULL) {

         file_ptr->f_file_stream = fopen(file_name,BINARY_CREATE_RDWR);

         if (file_ptr->f_file_stream == NULL) {

            /* return om */

            unmark_specifier(target);
            target->sp_form = ft_omega;
            free_file(file_ptr);

            return;

         }
      }
#ifdef HAVE_SYS_SOCKET_H
   } else if ((strcmp(mode_string,"socket") == 0)&&(SAFE_MODE==0)) {

      char *name;
      char *colon = strchr(file_name,':');
      int fd;
      int portnum;
      struct sockaddr_in server, client;
      struct hostent *hp;

      if (colon) {

        long n = colon-file_name;
        name = (char *) malloc(n+1);
        if (!name)
           abend(SETL_SYSTEM 
                 "Call to malloc(%ld) failed, hostname too long",
                 n+1);
        memcpy (name, file_name, n);
        name[n] = '\0';
        portnum = to_portnum(SETL_SYSTEM colon+1);
      } else {
        name = file_name;
        portnum = 80;  /* so does this make me weird? */
      }
      fd=socket(AF_INET, SOCK_STREAM,0);
      if (fd == -1) abend(SETL_SYSTEM "Error in socket()");

      hp = find_hostent(name);
      if (!hp || !hp->h_addr_list[0]) {
        close(fd);
        fd = -1;
      } else {
         server.sin_family = AF_INET;
         server.sin_port = htons(portnum);
         memcpy(&server.sin_addr, hp->h_addr_list[0], hp->h_length);
         if (os_connect(fd, &server) == -1) {
           close(fd);
           fd = -1;
         } else {

         }
      }
     

      get_file(file_ptr);

      file_ptr->f_type = new_flag;
      file_ptr->f_flag = 0;

      file_ptr->f_mode = TCP;
      strcpy(file_ptr->f_file_name,file_name);


      if (fd== -1) {

            /* return om */

            unmark_specifier(target);
            target->sp_form = ft_omega;
            free_file(file_ptr);

            return;

      }
      file_ptr->f_file_fd=fd;
      file_ptr->f_file_buffer = (unsigned char *)malloc(
                               (size_t)(FILE_BUFF_SIZE + MAX_LOOKAHEAD + 1));
      if (file_ptr->f_file_buffer == NULL)
         giveup(SETL_SYSTEM msg_malloc_error);

      file_ptr->f_start = file_ptr->f_endofbuffer = file_ptr->f_file_buffer;
      file_ptr->f_eof_ptr = NULL;
#endif
   }

   else {

      abend(SETL_SYSTEM msg_bad_file_mode,mode_string);

   }

   /*
    *     At this point the file is opened successfully.  We get an atom
    *     to represent it and insert that atom in the file map.
    */

   file_atom.sp_form = ft_omega;
   setl2_newat(SETL_SYSTEM 0,NULL,&file_atom);

   map_work_hdr = file_map;
   work_hash_code = file_atom.sp_val.sp_atom_num;
   for (map_height = file_map->m_ntype.m_root.m_height;
        map_height;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code >>= MAP_SHIFT_DIST;

      /* if we're missing a header record, insert it */

      if (map_work_hdr->m_child[map_index].m_header == NULL) {

         get_map_header(new_hdr);
         new_hdr->m_ntype.m_intern.m_parent = map_work_hdr;
         new_hdr->m_ntype.m_intern.m_child_index = map_index;
         for (i = 0;
              i < MAP_HASH_SIZE;
              new_hdr->m_child[i++].m_cell = NULL);
         map_work_hdr->m_child[map_index].m_header = new_hdr;
         map_work_hdr = new_hdr;

      }
      else {

         map_work_hdr = map_work_hdr->m_child[map_index].m_header;

      }
   }

   /*
    *  At this point, map_work_hdr points to the lowest level
    *  header record.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   map_tail = &(map_work_hdr->m_child[map_index].m_cell);
   for (map_cell = *map_tail;
        map_cell != NULL &&
           map_cell->m_hash_code < file_atom.sp_val.sp_atom_num;
        map_cell = map_cell->m_next) {

      map_tail = &(map_cell->m_next);

   }

   /* we don't have to worry about duplicates -- add a cell */

   get_map_cell(new_cell);
   new_cell->m_domain_spec.sp_form = ft_atom;
   new_cell->m_domain_spec.sp_val.sp_atom_num = file_atom.sp_val.sp_atom_num;
   new_cell->m_range_spec.sp_form = ft_file;
   new_cell->m_range_spec.sp_val.sp_file_ptr = file_ptr;
   new_cell->m_is_multi_val = NO;
   new_cell->m_hash_code = file_atom.sp_val.sp_atom_num;
   new_cell->m_next = *map_tail;
   *map_tail = new_cell;
   file_map->m_ntype.m_root.m_cardinality++;
   file_map->m_ntype.m_root.m_cell_count++;
   file_map->m_hash_code ^= file_atom.sp_val.sp_atom_num;

   expansion_trigger =
         (1 << ((file_map->m_ntype.m_root.m_height + 1)
                    * MAP_SHIFT_DIST)) * MAP_CLASH_SIZE;

   /* expand the map header if necessary */

   if (file_map->m_ntype.m_root.m_cardinality > expansion_trigger) {

      file_map = map_expand_header(SETL_SYSTEM file_map);

   }

   /* return the atom */

   unmark_specifier(target);
   target->sp_form = ft_atom;
   target->sp_val.sp_atom_num = file_atom.sp_val.sp_atom_num;

   return;

}

/*\
 *  \function{setl2\_newopen()}
 *
 *  This is the new 1 argument version of open...
 *
\*/

void setl2_newopen(
   SETL_SYSTEM_PROTO 
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
char mode_string[30];                  /* mode (character string)           */
char file_name_buf[PATH_LENGTH + 1];   /* file name                         */
char *s, *t;                           /* temporary looping variables       */
int i;                                 /* temporary looping variable        */
char *file_name;

   /* convert the file name to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_file_spec,abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   if (string_hdr->s_length > PATH_LENGTH)
      abend(SETL_SYSTEM msg_bad_file_spec,abend_opnd_str(SETL_SYSTEM argv));

   t = file_name_buf;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < file_name_buf + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';
   file_name=file_name_buf;

#ifdef HAVE_SYS_SOCKET_H
   if ((strncmp(file_name,"tcp:",4)==0)||
       (strncmp(file_name,"tcp_client:",11)==0)) {

      char *name;
      char *colon = strchr(file_name,':');
      int fd;
      int portnum;
      struct sockaddr_in server, client;
      struct hostent *hp;

      file_name=colon+1;
      if ((*file_name!='/')||((*(file_name+1)!='/')))
          abend(SETL_SYSTEM msg_bad_file_mode,
                abend_opnd_str(SETL_SYSTEM argv));

      file_name += 2;
      setl2_internal_open(SETL_SYSTEM 1,target,"socket",file_name);
      return;
   }

   else 
#endif
   if ((strncmp(file_name,"file:",5)==0)||
       (strncmp(file_name,"text_file:",10)==0)) {

      char *name;
      char *colon = strchr(file_name,':');

      file_name=colon+1;
      setl2_internal_open(SETL_SYSTEM 1,target,"text-in",file_name);
      return;
   }
   else 
      abend(SETL_SYSTEM msg_bad_file_mode,abend_opnd_str(SETL_SYSTEM argv));

}


/*\
 *  \function{setl2\_open()}
 *
 *  This function opens a file and returns a file handle.  The handle is
 *  an atom which must be used in other procedure calls to identify the
 *  file being accessed.
\*/

void setl2_open(
   SETL_SYSTEM_PROTO 
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
char mode_string[30];                  /* mode (character string)           */
char file_name[PATH_LENGTH*2 + 1];     /* file name                         */
char *s, *t, *q;                       /* temporary looping variables       */
int i;                                 /* temporary looping variable        */

   
   /* convert the file name to a C character string */
   if ((argc==1)&&(SAFE_MODE==0))  {
      setl2_newopen(SETL_SYSTEM argc,argv,target);
      return;
   }
   if (argc!=2)
      abend(SETL_SYSTEM msg_wrong_parms);

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_file_spec,abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   if (((SAFE_MODE==1)&&(string_hdr->s_length!=5))||
       (string_hdr->s_length > PATH_LENGTH))
      abend(SETL_SYSTEM msg_bad_file_spec,abend_opnd_str(SETL_SYSTEM argv));

   q = t = file_name;
   *file_name=0;
   if (SAFE_MODE==1) {
	if (SAFE_PREFIX!=NULL) {
		strcpy(file_name,SAFE_PREFIX);
	 	t+=strlen(file_name);
		*t='/';t++;
		q=t;
	}
   }
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < q + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   if ((SAFE_MODE==1)&&
	((strncmp(q,"File",4)!=0)||(*(q+4)<'1')||(*(q+4)>'5')))
      abend(SETL_SYSTEM msg_bad_file_spec,abend_opnd_str(SETL_SYSTEM argv));

   /* convert the file mode to a C character string */

   if (argv[1].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_file_mode,abend_opnd_str(SETL_SYSTEM argv+1));

   string_hdr = argv[1].sp_val.sp_string_ptr;

   if (string_hdr->s_length > 30)
      abend(SETL_SYSTEM msg_bad_file_mode,abend_opnd_str(SETL_SYSTEM argv+1));

   t = mode_string;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < mode_string + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           s++, t++) {

         if (isupper(*s))
            *t = tolower(*s);
         else
            *t = *s;

      }
   }
   *t = '\0';

   setl2_internal_open(SETL_SYSTEM 0,target,mode_string,file_name);

}

/*\
 *  \function{setl2\_close()}
 *
 *  This function closes a file opened with the \verb"open()" function
 *  above.
\*/

void setl2_close(
   SETL_SYSTEM_PROTO 
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
map_c_ptr_type *map_tail;              /* attach new cells here             */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   map_tail = &(map_work_hdr->m_child[map_index].m_cell);
   for (map_cell = *map_tail;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next) {

      map_tail = &(map_cell->m_next);

   }

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* save file pointer then remove cell */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;
   file_map->m_ntype.m_root.m_cardinality--;
   file_map->m_ntype.m_root.m_cell_count--;
   file_map->m_hash_code ^= argv[0].sp_val.sp_atom_num;
   *map_tail = map_cell->m_next;
   free_map_cell(map_cell);

   /* close the file */

   switch (file_ptr->f_mode) {

      case TEXT_IN :

         free(file_ptr->f_file_buffer);
         fclose(file_ptr->f_file_stream);
         break;

      case TEXT_OUT :

         fclose(file_ptr->f_file_stream);
         break;

      case BYTE_IN :

         fclose(file_ptr->f_file_stream);
         break;

      case BINARY_IN :

         fclose(file_ptr->f_file_stream);
         break;

      case BINARY_OUT :

         fclose(file_ptr->f_file_stream);
         break;

      case RANDOM :

         fclose(file_ptr->f_file_stream);
         break;

      case TCP :

         close(file_ptr->f_file_fd);
	 break;

#ifdef TRAPS

      default :

         trap(__FILE__,__LINE__,"Invalid file mode => %d",file_ptr->f_mode);

#endif

   }

   /* we're done with the file pointer */

   free_file(file_ptr);

   /* return omega */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_get()}
 *
 *  This function reads one line from standard input.  It uses the same
 *  mechanism as the read procedures, but simply reads entire lines into
 *  character strings.
\*/

void setl2_get(
   SETL_SYSTEM_PROTO 
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (none here)       */
   specifier *target)                  /* return value                      */

{
string_h_ptr_type string_hdr;          /* created string header             */
string_c_ptr_type string_cell;         /* current string cell               */
char *string_char, *string_end;        /* string character pointers         */

   /* load file stuff from standard input node */

   file_ptr = stdin_ptr;
   file_name = file_ptr->f_file_name;
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;
   file_buffer = file_ptr->f_file_buffer;
   lookahead = start = file_ptr->f_start;
   endofbuffer = file_ptr->f_endofbuffer;
   eof_ptr = file_ptr->f_eof_ptr;

   /* push lines on the stack */

   while (argc--) {

      /* check for end of file */

      eof_flag = NO;
      advance_la;
      if (lookahead == eof_ptr) {
      
         eof_flag = YES;
         lookahead = start;
         unmark_specifier(target);
         target->sp_form = ft_omega;

         push_pstack(target);
         continue;

      }

      /* initialize the return string */

      get_string_header(string_hdr);
      string_hdr->s_use_count = 1;
      string_hdr->s_hash_code = -1;
      string_hdr->s_length = 0;
      string_hdr->s_head = string_hdr->s_tail = string_cell = NULL;
      string_char = string_end = NULL;

      /* copy each character of the string */

      while (*lookahead != '\n' && *lookahead != '\r' &&
             lookahead != eof_ptr) {

         start = lookahead;

         if (string_char == string_end) {

            get_string_cell(string_cell);
            if (string_hdr->s_tail != NULL)
               (string_hdr->s_tail)->s_next = string_cell;
            string_cell->s_prev = string_hdr->s_tail;
            string_cell->s_next = NULL;
            string_hdr->s_tail = string_cell;
            if (string_hdr->s_head == NULL)
               string_hdr->s_head = string_cell;
            string_char = string_cell->s_cell_value;
            string_end = string_char + STR_CELL_WIDTH;

         }

         *string_char++ = *lookahead;
         string_hdr->s_length++;

         advance_la;

      }

      /* skip over the newline */

      if (*lookahead == '\r') {
         advance_la;
         if (*lookahead == '\n')
            start = lookahead;
         else
            start = lookahead - 1;
      }
      else
         start = lookahead;

      file_ptr->f_start = start;
      file_ptr->f_endofbuffer = endofbuffer;
      file_ptr->f_eof_ptr = eof_ptr;

      /* set and push the target */

      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = string_hdr;

      push_pstack(target);

   }

   /* return om */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_geta()}
 *
 *  This function works like \verb"get", except that it gets from a
 *  file opened for text input.
\*/

void setl2_geta(
   SETL_SYSTEM_PROTO 
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */
string_h_ptr_type string_hdr;          /* created string header             */
string_c_ptr_type string_cell;         /* current string cell               */
char *string_char, *string_end;        /* string character pointers         */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;
   if ((file_ptr->f_mode != TEXT_IN)&&(file_ptr->f_mode != TCP))
      abend(SETL_SYSTEM msg_get_not_text,file_ptr->f_file_name);
   file_name = file_ptr->f_file_name;
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;
   file_buffer = file_ptr->f_file_buffer;
   lookahead = start = file_ptr->f_start;
   endofbuffer = file_ptr->f_endofbuffer;
   eof_ptr = file_ptr->f_eof_ptr;

   /* push lines on the stack */

   while (argc-- > 1) {

      /* check for end of file */

      eof_flag = NO;
      advance_la;
      if (lookahead == eof_ptr) {
      
         eof_flag = YES;
         lookahead = start;
         unmark_specifier(target);
         target->sp_form = ft_omega;

         push_pstack(target);
         continue;

      }

      /* initialize the return string */

      get_string_header(string_hdr);
      string_hdr->s_use_count = 1;
      string_hdr->s_hash_code = -1;
      string_hdr->s_length = 0;
      string_hdr->s_head = string_hdr->s_tail = string_cell = NULL;
      string_char = string_end = NULL;

      /* copy each character of the string */

      while (*lookahead != '\n' && *lookahead != '\r' &&
             lookahead != eof_ptr) {

         start = lookahead;

         if (string_char == string_end) {

            get_string_cell(string_cell);
            if (string_hdr->s_tail != NULL)
               (string_hdr->s_tail)->s_next = string_cell;
            string_cell->s_prev = string_hdr->s_tail;
            string_cell->s_next = NULL;
            string_hdr->s_tail = string_cell;
            if (string_hdr->s_head == NULL)
               string_hdr->s_head = string_cell;
            string_char = string_cell->s_cell_value;
            string_end = string_char + STR_CELL_WIDTH;

         }

         *string_char++ = *lookahead;
         string_hdr->s_length++;

         advance_la;

      }

      /* skip over the newline */

      if (*lookahead == '\r') {
         advance_la;
         if (*lookahead == '\n')
            start = lookahead;
         else
            start = lookahead - 1;
      }
      else
         start = lookahead;

      file_ptr->f_start = start;
      file_ptr->f_endofbuffer = endofbuffer;
      file_ptr->f_eof_ptr = eof_ptr;

      /* set and push the target */

      unmark_specifier(target);
      target->sp_form = ft_string;
      target->sp_val.sp_string_ptr = string_hdr;

      push_pstack(target);

   }

   /* return om */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_read()}
 *
 *  This function handles formatted input from standard input.  It can
 *  read any SETL value, but is a little picky about the format.
\*/

void setl2_read(
   SETL_SYSTEM_PROTO 
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (none here)       */
   specifier *target)                  /* return value                      */

{
int return_code;                       /* read_spec return code             */

   /* load file stuff from standard input node */

   file_ptr = stdin_ptr;
   file_name = file_ptr->f_file_name;
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;
   file_buffer = file_ptr->f_file_buffer;
   lookahead = start = file_ptr->f_start;
   endofbuffer = file_ptr->f_endofbuffer;
   eof_ptr = file_ptr->f_eof_ptr;

   /* push values on the stack */

   while (argc--) {

      eof_flag = NO;
      return_code = read_spec(SETL_SYSTEM target);
      if (return_code != SPEC && return_code != ENDOFFILE) {

         abend(SETL_SYSTEM msg_not_setl_value);

      }

      push_pstack(target);

   }

   file_ptr->f_start = start;
   file_ptr->f_endofbuffer = endofbuffer;
   file_ptr->f_eof_ptr = eof_ptr;

   /* return om */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_reada()}
 *
 *  This function works like \verb"read", except that it reads from a
 *  file opened for text input.
\*/

void setl2_reada(
   SETL_SYSTEM_PROTO 
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
int return_code;                       /* read_spec return code             */
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;
   if ((file_ptr->f_mode != TEXT_IN)&&(file_ptr->f_mode != TCP))
      abend(SETL_SYSTEM msg_read_not_text,file_ptr->f_file_name);
   file_name = file_ptr->f_file_name;
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;
   file_buffer = file_ptr->f_file_buffer;
   lookahead = start = file_ptr->f_start;
   endofbuffer = file_ptr->f_endofbuffer;
   eof_ptr = file_ptr->f_eof_ptr;

   /* push values on the stack */

   while (argc-- > 1) {

      eof_flag = NO;
      return_code = read_spec(SETL_SYSTEM target);
      if (return_code != SPEC && return_code != ENDOFFILE) {

         abend(SETL_SYSTEM msg_not_setl_value);

      }

      push_pstack(target);

   }

   file_ptr->f_start = start;
   file_ptr->f_endofbuffer = endofbuffer;
   file_ptr->f_eof_ptr = eof_ptr;

   /* return om */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_reads()}
 *
 *  This function handles formatted input from a string.  It is useful in
 *  converting string representations to internal form.
\*/

void setl2_reads(
   SETL_SYSTEM_PROTO 
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (none here)       */
   specifier *target)                  /* return value                      */

{
int return_code;                       /* read_spec return code             */
string_h_ptr_type target_hdr;          /* tail string                       */
string_c_ptr_type target_cell;         /* tail cell                         */
char *target_char_ptr, *target_char_end;
                                       /* target string pointers            */
int32 tail_pstack_top;                 /* stack position of string tail     */
int save_eof_flag;                     /* reads destroys eof_flag           */

   /* make sure the argument is a string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"reads",
            abend_opnd_str(SETL_SYSTEM argv));

   /* save the eof flag */

   save_eof_flag = eof_flag;

   /* load file stuff from string node */

   file_ptr = reads_ptr;
   file_buffer = file_ptr->f_file_buffer;
   lookahead = start = endofbuffer = file_buffer;
   eof_ptr = NULL;

   /* load input string */

   reads_length = argv[0].sp_val.sp_string_ptr->s_length;
   reads_cell = argv[0].sp_val.sp_string_ptr->s_head;
   if (reads_cell == NULL) {
      reads_char_ptr = reads_char_end = NULL;
   }
   else {
      reads_char_ptr = reads_cell->s_cell_value;
      reads_char_end = reads_char_ptr + STR_CELL_WIDTH;
   }

   /* save a position for the string tail */

   unmark_specifier(target);
   target->sp_form = ft_omega;
   push_pstack(target);
   tail_pstack_top = pstack_top;

   /* push values on the stack */

   while (argc-- > 1) {

      return_code = read_spec(SETL_SYSTEM target);
      if (return_code != SPEC && return_code != ENDOFFILE)
         abend(SETL_SYSTEM msg_bad_arg,"SETL2 value string",1,"reads",
               abend_opnd_str(SETL_SYSTEM argv));

      push_pstack(target);

   }

   /* make a target string */

   get_string_header(target_hdr);
   target_hdr->s_use_count = 1;
   target_hdr->s_hash_code = -1;
   target_hdr->s_length = 0;
   target_hdr->s_head = target_hdr->s_tail = NULL;
   target_char_ptr = target_char_end = NULL;

   /* copy the rest of the input string */

   lookahead = start;
   advance_la;
   while (lookahead != eof_ptr) {

      if (target_char_ptr == target_char_end) {

         get_string_cell(target_cell);
         if (target_hdr->s_tail != NULL)
            (target_hdr->s_tail)->s_next = target_cell;
         target_cell->s_prev = target_hdr->s_tail;
         target_cell->s_next = NULL;
         target_hdr->s_tail = target_cell;
         if (target_hdr->s_head == NULL)
            target_hdr->s_head = target_cell;
         target_char_ptr = target_cell->s_cell_value;
         target_char_end = target_char_ptr + STR_CELL_WIDTH;

      }

      *target_char_ptr++ = *lookahead;
      target_hdr->s_length++;
      start = lookahead;
      advance_la;

   }

   pstack[tail_pstack_top].sp_form = ft_string;
   pstack[tail_pstack_top].sp_val.sp_string_ptr = target_hdr;

   /* return om */

   unmark_specifier(target);
   target->sp_form = ft_omega;
   eof_flag = save_eof_flag;

   return;

}

/*\
 *  \function{setl2\_unstr()}
 *
 *  This is the functional form of \verb"reads".  Why in the world did I
 *  make it a procedure!?  I'd like to get rid of \verb"reads".
\*/

void setl2_unstr(
   SETL_SYSTEM_PROTO 
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (none here)       */
   specifier *target)                  /* return value                      */

{
int return_code;                       /* read_spec return code             */

   /* make sure the argument is a string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"unstr",
            abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from string node */

   file_ptr = reads_ptr;
   file_buffer = file_ptr->f_file_buffer;
   lookahead = start = endofbuffer = file_buffer;
   eof_ptr = NULL;

   /* load input string */

   reads_length = argv[0].sp_val.sp_string_ptr->s_length;
   reads_cell = argv[0].sp_val.sp_string_ptr->s_head;
   if (reads_cell == NULL) {
      reads_char_ptr = reads_char_end = NULL;
   }
   else {
      reads_char_ptr = reads_cell->s_cell_value;
      reads_char_end = reads_char_ptr + STR_CELL_WIDTH;
   }

   /* read the value */

   return_code = read_spec(SETL_SYSTEM target);
   if (return_code != SPEC && return_code != ENDOFFILE)
      abend(SETL_SYSTEM msg_bad_arg,"SETL2 value string",1,"reads",
            abend_opnd_str(SETL_SYSTEM argv));

   return;

}

/*\
 *  \function{read\_spec()}
 *
 *  This function tries to read a single item from the current text
 *  input file.  It is essentially a miniature lexical analyzer,
 *  following nearly the same rules and using the same structure as the
 *  compiler's lexical analyzer.
\*/
      
static int read_spec(
   SETL_SYSTEM_PROTO 
   specifier *spec)                    /* item to be read                   */

{
int found_comma = NO;                  /* YES if we've already seen a comma */

   /* get the next input character */

   lookahead = start;
   advance_la;

   /* loop until we explicitly return */

   for (;;) {

      /* skip white space */

      while (is_white_space(*lookahead)) {

         start = lookahead;
         advance_la;

      }

/*\
 *  \case{end of file}
 *
 *  When we see the end of file character, we just return it and do NOT
 *  advance the pointers.  If we are called again we return end of file
 *  again.
\*/

      if (lookahead == eof_ptr) {

         /* flag end of file TRUE */

         eof_flag = YES;

         /* we return omega on end of file */

         unmark_specifier(spec);
         spec->sp_form = ft_omega;

         return ENDOFFILE;

      }

      switch (*lookahead) {

/*\
 *  \case{whitespace}
 *
 *  We separated a few classes of whitespace in the character handling
 *  macros for the convenience of the compiler.  Here we must handle
 *  those.
\*/

case '\n' : case '\r' : case '\t' : case 8 :

{

   start = lookahead;
   advance_la;

   break;

}

/*\
 *  \case{identifier strings}
 *
 *  We allow strings without quotes to be read in, provided that the
 *  strings follow the same rules as identifiers.
\*/

case 'a': case 'b': case 'c': case 'd': case 'e': case 'f' : case 'g' :
case 'h': case 'i': case 'j': case 'k': case 'l': case 'm' : case 'n' :
case 'o': case 'p': case 'q': case 'r': case 's': case 't' : case 'u' :
case 'v': case 'w': case 'x': case 'y': case 'z':
case 'A': case 'B': case 'C': case 'D': case 'E': case 'F' : case 'G' :
case 'H': case 'I': case 'J': case 'K': case 'L': case 'M' : case 'N' :
case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T' : case 'U' :
case 'V': case 'W': case 'X': case 'Y': case 'Z':

{
string_h_ptr_type string_hdr;          /* created string header             */
string_c_ptr_type string_cell;         /* current string cell               */
char *string_char, *string_end;        /* string character pointers         */

   /* initialize the return string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = string_cell = NULL;
   string_char = string_end = NULL;

   /* copy each character of the string */

   while (is_id_char(*lookahead)) {

      start = lookahead;

      if (string_char == string_end) {

         get_string_cell(string_cell);
         if (string_hdr->s_tail != NULL)
            (string_hdr->s_tail)->s_next = string_cell;
         string_cell->s_prev = string_hdr->s_tail;
         string_cell->s_next = NULL;
         string_hdr->s_tail = string_cell;
         if (string_hdr->s_head == NULL)
            string_hdr->s_head = string_cell;
         string_char = string_cell->s_cell_value;
         string_end = string_char + STR_CELL_WIDTH;

      }

      *string_char++ = *lookahead;
      string_hdr->s_length++;

      advance_la;

   }

   /* special checks for om, true and false */

   if (string_hdr->s_length == 2) {

      for (string_char = (string_hdr->s_head)->s_cell_value,
              string_end = "om";
           *string_end;
           string_char++, string_end++) {

         if (isupper(*string_char)) {
            if (tolower(*string_char) != *string_end)
               break;
         }
         else {
            if (*string_char != *string_end)
               break;
         }
      }

      if (!(*string_end)) {

         free_string(SETL_SYSTEM string_hdr);
         unmark_specifier(spec);
         spec->sp_form = ft_omega;

         return SPEC;

      }
   }
   if (string_hdr->s_length == 4) {

      for (string_char = (string_hdr->s_head)->s_cell_value,
              string_end = "true";
           *string_end;
           string_char++, string_end++) {

         if (isupper(*string_char)) {
            if (tolower(*string_char) != *string_end)
               break;
         }
         else {
            if (*string_char != *string_end)
               break;
         }
      }

      if (!(*string_end)) {

         free_string(SETL_SYSTEM string_hdr);
         unmark_specifier(spec);
         spec->sp_form = ft_atom;
         spec->sp_val.sp_atom_num = spec_true->sp_val.sp_atom_num;

         return SPEC;

      }
   }
   else if (string_hdr->s_length == 5) {

      for (string_char = (string_hdr->s_head)->s_cell_value,
              string_end = "false";
           *string_end;
           string_char++, string_end++) {

         if (isupper(*string_char)) {
            if (tolower(*string_char) != *string_end)
               break;
         }
         else {
            if (*string_char != *string_end)
               break;
         }
      }

      if (!(*string_end)) {

         free_string(SETL_SYSTEM string_hdr);
         unmark_specifier(spec);
         spec->sp_form = ft_atom;
         spec->sp_val.sp_atom_num = spec_false->sp_val.sp_atom_num;

         return SPEC;

      }
   }

   /* set the target and return */

   unmark_specifier(spec);
   spec->sp_form = ft_string;
   spec->sp_val.sp_string_ptr = string_hdr;

   return SPEC;

}

/*\
 *  \case{quoted strings}
 *
 *  Quoted strings follow the same rules as string literals in the
 *  compiler.  They are similar to string literals in C, except for the
 *  concept of a null-terminated string.  Strings do not necessarily stop
 *  at a null.
\*/

case '\"' :

{
string_h_ptr_type string_hdr;          /* created string header             */
string_c_ptr_type string_cell;         /* current string cell               */
char *string_char, *string_end;        /* string character pointers         */

   /* initialize the return string */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 0;
   string_hdr->s_head = string_hdr->s_tail = string_cell = NULL;
   string_char = string_end = NULL;

   /* advance past the opening quote */

   advance_la;

   /* scan the string, translating escape sequences */

   for (;;) {

      start = lookahead;

      /* if we found the ending quote break */

      if (*lookahead == '\"')
         break;

      /* check for unterminated literal */

      if (*lookahead == '\r' ||
          *lookahead == '\n' ||
          lookahead == eof_ptr) {

         abend(SETL_SYSTEM "Unterminated quoted string in read");

      }

      /* expand the string structure if necessary */

      if (string_char == string_end) {

         get_string_cell(string_cell);
         if (string_hdr->s_tail != NULL)
            (string_hdr->s_tail)->s_next = string_cell;
         string_cell->s_prev = string_hdr->s_tail;
         string_cell->s_next = NULL;
         string_hdr->s_tail = string_cell;
         if (string_hdr->s_head == NULL)
            string_hdr->s_head = string_cell;
         string_char = string_cell->s_cell_value;
         string_end = string_char + STR_CELL_WIDTH;

      }

      /* check escape sequences */

      if (*lookahead == '\\') {

         start = lookahead;
         advance_la;

         switch (*lookahead) {

            case '\\' :

               *string_char++ = '\\';
               string_hdr->s_length++;

               break;

            case '0' :

               *string_char++ = '\0';
               string_hdr->s_length++;

               break;

            case 'n' :

               *string_char++ = '\n';
               string_hdr->s_length++;

               break;

            case 'r' :

               *string_char++ = '\r';
               string_hdr->s_length++;

               break;

            case 'f' :

               *string_char++ = '\f';
               string_hdr->s_length++;

               break;

            case 't' :

               *string_char++ = '\t';
               string_hdr->s_length++;

               break;

            case '\"' :

               *string_char++ = '\"';
               string_hdr->s_length++;

               break;

            case 'x' : case 'X' :

               advance_la;
               advance_la;

               if (!is_digit(*(lookahead-1),16) ||
                   !is_digit(*lookahead,16)) {

                  abend(SETL_SYSTEM "Invalid hex character => %c%c",
                        *(lookahead-1),
                        *lookahead);

               }

               *string_char++ = numeric_val(*(lookahead-1)) * 16 +
                                numeric_val(*lookahead);
               string_hdr->s_length++;

               break;

            default :

               abend(SETL_SYSTEM "Invalid escape sequence => \\%c",*lookahead);

         }

         start = lookahead;
         advance_la;

         continue;

      }

      *string_char++ = *lookahead;
      string_hdr->s_length++;

      advance_la;

   }

   /* set the target and return */

   unmark_specifier(spec);
   spec->sp_form = ft_string;
   spec->sp_val.sp_string_ptr = string_hdr;

   return SPEC;

}

/*\
 *  \case{numbers}
 *
 *  This case handles numbers, both integer and real.  Numeric literals
 *  in SETL2 borrow ideas from Ada, Icon, and SETL. Like SETL, integers
 *  can be infinite in length during execution, but literals are limited
 *  by the maximum length of a lexical token.  Like Ada, we use the pound
 *  sign, \verb"#", to delimit base changes.  Like Icon, we allow numbers
 *  to use any base from 2 to 36, using alphabetic characters to
 *  represent the digits 10 to 35.
\*/

case '0': case '1': case '2': case '3': case '4': case '5': case '6':
case '7': case '8': case '9': case '-': case '+':

{
int is_negative;                       /* YES if the number is negative     */
int base;                              /* numeric base                      */
int special_base;                      /* YES if base is changed            */
int32 max_multiplier;                  /* maximum value we can multiply     */
                                       /* by cell                           */
int32 multiplier;                      /* actual value we multiply by cell  */
int32 addend;                          /* amount to add to next cell        */
int32 hi_bits;                         /* high order bits of short integer  */
integer_h_ptr_type integer_hdr;        /* root integer pointer              */
integer_c_ptr_type integer_cell;       /* integer cell pointer              */
double whole_part;                     /* whole part of real                */
double decimal_part;                   /* fraction part of real             */
double decimal_divisor;                /* decimal divisor                   */
int exponent_sign;                     /* 1 => positive exponent, -1 o/w    */
int exponent;                          /* exponent value                    */
i_real_ptr_type real_ptr;              /* real node pointer                 */

   /* take care of the sign if we have one */

   if (*lookahead == '-') {

      is_negative = YES;
      start = lookahead;
      advance_la;

   }
   else if (*lookahead == '+') {

      is_negative = NO;
      start = lookahead;
      advance_la;

   }
   else {

      is_negative = NO;

   }

   /* we find either the base or whole part */

   special_base = NO;
   addend = 0;
   multiplier = 1;
   while ((is_digit(*lookahead,10) || *lookahead == '_') &&
          addend <= 36) {

      start = lookahead;

      if (*lookahead == '_') {

         advance_la;
         continue;

      }

      addend = addend * 10 + numeric_val(*lookahead);
      multiplier = multiplier * 10;

      advance_la;

   }

   /* if we found a pound sign we have a base */

   if (*lookahead == '#') {

      special_base = YES;
      start = lookahead;
      advance_la;

      /* we need to use the base to determine if characters are digits */

      base = (int)addend;

      /* we allow bases from 2 to 36 */

      if (base < 2 || base > 36)
         abend(SETL_SYSTEM "Invalid number base => %d",base);

      addend = 0;

   }
   else {

      base = 10;

   }

   /* find the maximum cell multiplier */

   max_multiplier = MAX_INT_CELL / base;

   /* loop over the whole part */

   integer_hdr = NULL;
   for (;;) {

      /* pick out as many digits as we can handle */

      while ((is_digit(*lookahead,base) || *lookahead == '_') &&
             multiplier < max_multiplier) {

         start = lookahead;

         if (*lookahead == '_') {

            advance_la;

            continue;

         }

         addend = addend * base + numeric_val(*lookahead);
         multiplier = multiplier * base;

         advance_la;

      }

      /* if we've reached the end of the whole part, break */

      if (!is_digit(*lookahead,base) && *lookahead != '_' &&
          integer_hdr == NULL)
         break;

      if (integer_hdr == NULL) {

         get_integer_header(integer_hdr);
         integer_hdr->i_use_count = 1;
         integer_hdr->i_hash_code = -1;
         integer_hdr->i_cell_count = 1;
         integer_hdr->i_is_negative = NO;

         get_integer_cell(integer_cell);
         integer_cell->i_cell_value = addend;
         integer_cell->i_next = integer_cell->i_prev = NULL;
         integer_hdr->i_head = integer_hdr->i_tail = integer_cell;

         addend = 0;
         multiplier = 1;

         continue;

      }

      /* traverse the list, updating each cell */

      for (integer_cell = integer_hdr->i_head;
           integer_cell != NULL || addend != 0;
           integer_cell = integer_cell->i_next) {

         /* if the next pointer is null, extend the list */

         if (integer_cell == NULL) {

            get_integer_cell(integer_cell);
            (integer_hdr->i_tail)->i_next = integer_cell;
            integer_cell->i_prev = integer_hdr->i_tail;
            integer_hdr->i_tail = integer_cell;
            integer_cell->i_next = NULL;
            integer_hdr->i_cell_count++;
            integer_cell->i_cell_value = 0;

         }

         /* update the cell */

         addend = integer_cell->i_cell_value * multiplier + addend;
         integer_cell->i_cell_value = addend & MAX_INT_CELL;
         addend >>= INT_CELL_WIDTH;

      }

      if (!is_digit(*lookahead,base) && *lookahead != '_')
         break;

      addend = 0;
      multiplier = 1;

   }

   /*
    *  At this point we have reached the end of the whole part of the
    *  number.  If the number is very short the value is in addend.
    *  Otherwise we have a long integer.  We check for a decimal point
    *  and a following digit.  If we find these, we convert to a real
    *  number.
    */

   if (*lookahead == '.') {

      advance_la;

      if (is_digit(*lookahead,base)) {

         /* convert the whole part from integer to real */

         if (integer_hdr == NULL) {

            whole_part = (double)addend;

         }
         else {

            whole_part = 0.0;

            for (integer_cell = integer_hdr->i_tail;
                 integer_cell != NULL;
                 integer_cell = integer_cell->i_prev) {

               whole_part = whole_part * (double)(MAX_INT_CELL + 1) +
                            (double)integer_cell->i_cell_value;

            }
	    /* Free the integer */


            free_interp_integer(SETL_SYSTEM integer_hdr);

         }

         /* pick out the decimal part */

         decimal_part = 0.0;
         decimal_divisor = 1.0;
         while (is_digit(*lookahead,base) || *lookahead == '_') {

            start = lookahead;

            if (*lookahead == '_') {

               advance_la;

               continue;

            }

            decimal_part = decimal_part * (double)base +
                           (double)(numeric_val(*lookahead));
            decimal_divisor *= base;

            advance_la;

         }

         /* make sure based literals have following '#' */

         if (special_base) {
            if (*lookahead != '#') {
               abend(SETL_SYSTEM "Missing '#' in based number");
            }
            else {
               start = lookahead;
               advance_la;
            }
         }

         /* pick out the exponent */

         exponent = 0;
         exponent_sign = 1;
         if (*lookahead == 'e' || *lookahead == 'E') {

            start = lookahead;
            advance_la;

            if (*lookahead == '-') {

               start = lookahead;
               exponent_sign = -1;
               advance_la;

            }
            else if (*lookahead == '+') {

               start = lookahead;
               advance_la;

            }

            while (is_digit(*lookahead,base) || *lookahead == '_') {

               start = lookahead;
               exponent = exponent * 10 + numeric_val(*lookahead);
               advance_la;

            }
         }

         /* allocate and set a real node */

         i_get_real(real_ptr);
         real_ptr->r_use_count = 1;
         real_ptr->r_value = (whole_part +
                             (decimal_part / decimal_divisor)) *
                             pow((double)base,
                                 (double)(exponent * exponent_sign));

         if (is_negative)
            real_ptr->r_value = -real_ptr->r_value;

         /* set the target and return */

         unmark_specifier(spec);
         spec->sp_form = ft_real;
         spec->sp_val.sp_real_ptr = real_ptr;

         return SPEC;

      }
   }

   /*
    *  Now we know the value is an integer.  All we must do is normalize
    *  it (make sure it is a long or short as appropriate), and return
    *  the result.
    */

   /* make sure based literals have following '#' */

   if (special_base) {
      if (*lookahead != '#') {
         abend(SETL_SYSTEM "Missing '#' in based number");
      }
      else {
         start = lookahead;
         advance_la;
      }
   }

   if (integer_hdr == NULL) {

      if (is_negative)
         addend = -addend;

      /* check whether the result remains short */

      if (!(hi_bits = addend & INT_HIGH_BITS) ||
          hi_bits == INT_HIGH_BITS) {

         unmark_specifier(spec);
         spec->sp_form = ft_short;
         spec->sp_val.sp_short_value = addend;

         return SPEC;

      }

      /* if we exceed the maximum short, convert to long */

      short_to_long(SETL_SYSTEM spec,addend);

      return SPEC;

   }

   if (is_negative)
      integer_hdr->i_is_negative = YES;

   /*
    *  Now we have a long value in the target.  We would like to use
    *  short values whenever possible, so we check whether it will fit in
    *  a short.  If so, we convert it.
    */

   if (integer_hdr->i_cell_count < 3) {

      /* build up a long value */

      addend = (integer_hdr->i_head)->i_cell_value;
      if (integer_hdr->i_cell_count == 2) {

         addend += (((integer_hdr->i_head)->i_next)->i_cell_value) <<
                    INT_CELL_WIDTH;

      }

      if (integer_hdr->i_is_negative)
         addend = -addend;

      /* check whether it will fit in a short */

      if (!(hi_bits = addend & INT_HIGH_BITS) ||
          hi_bits == INT_HIGH_BITS) {

         free_interp_integer(SETL_SYSTEM integer_hdr);
         unmark_specifier(spec);
         spec->sp_form = ft_short;
         spec->sp_val.sp_short_value = addend;

         return SPEC;

      }
   }

   /* we couldn't convert to short, so return the long */

   unmark_specifier(spec);
   spec->sp_form = ft_long;
   spec->sp_val.sp_long_ptr = integer_hdr;

   return SPEC;

}

/*\
 *  \case{sets}
 *
 *  When we find an opening brace we start entering subsequent items into
 *  a set.  We return when we find the closing brace.
\*/

case '{' :

{
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
specifier target_element;              /* set element                       */
int32 target_hash_code;                /* hash code of target element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int is_equal;                          /* YES if two specifiers are equal   */
int i;                                 /* temporary looping variable        */

   /* advance past the opening bracket */

   start = lookahead;

   /* create a new set for the target */

   get_set_header(target_root);
   target_root->s_use_count = 1;
   target_root->s_hash_code = 0;
   target_root->s_ntype.s_root.s_cardinality = 0;
   target_root->s_ntype.s_root.s_height = 0;
   for (i = 0;
        i < SET_HASH_SIZE;
        target_root->s_child[i++].s_cell = NULL);
   expansion_trigger = SET_HASH_SIZE * SET_CLASH_SIZE;

   /* insert elements until we find a right brace */

   for (;;) {

      /* get the next spec from the input stream */

      target_element.sp_form = ft_omega;

      switch (read_spec(SETL_SYSTEM &target_element)) {

         case SPEC :

            break;

         case RBRACE :

            unmark_specifier(spec);
            spec->sp_form = ft_set;
            spec->sp_val.sp_set_ptr = target_root;

            return SPEC;

         default :

            abend(SETL_SYSTEM msg_not_setl_value);

      }

      /*
       *  At this point we have an element we would like to insert into
       *  the target.
       */

      target_work_hdr = target_root;

      /* get the element's hash code */

      spec_hash_code(work_hash_code,&target_element);
      target_hash_code = work_hash_code;

      /* descend the header tree until we get to a leaf */

      target_height = target_root->s_ntype.s_root.s_height;
      while (target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & SET_HASH_MASK;
         work_hash_code = work_hash_code >> SET_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (target_work_hdr->s_child[target_index].s_header == NULL) {

            get_set_header(new_hdr);
            new_hdr->s_ntype.s_intern.s_parent = target_work_hdr;
            new_hdr->s_ntype.s_intern.s_child_index = target_index;
            for (i = 0;
                 i < SET_HASH_SIZE;
                 new_hdr->s_child[i++].s_cell = NULL);
            target_work_hdr->s_child[target_index].s_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->s_child[target_index].s_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  The next problem is to determine if the element is
       *  already in the set.  We compare the element with the clash
       *  list.
       */

      target_index = work_hash_code & SET_HASH_MASK;
      target_tail = &(target_work_hdr->s_child[target_index].s_cell);
      for (target_cell = *target_tail;
           target_cell != NULL &&
              target_cell->s_hash_code < target_hash_code;
           target_cell = target_cell->s_next) {

         target_tail = &(target_cell->s_next);

      }

      /* check for a duplicate element */

      is_equal = NO;
      while (target_cell != NULL &&
             target_cell->s_hash_code == target_hash_code) {

         spec_equal(is_equal,&(target_cell->s_spec),&target_element);

         if (is_equal)
            break;

         target_tail = &(target_cell->s_next);
         target_cell = target_cell->s_next;

      }

      /* if we have a duplicate, unmark it and get the next one */

      if (is_equal) {

         unmark_specifier(&target_element);
         continue;

      }

      /* if we reach this point we didn't find the element, so we insert it */

      get_set_cell(new_cell);
      new_cell->s_spec.sp_form = target_element.sp_form;
      new_cell->s_spec.sp_val.sp_biggest =
         target_element.sp_val.sp_biggest;
      new_cell->s_hash_code = target_hash_code;
      new_cell->s_next = *target_tail;
      *target_tail = new_cell;
      target_root->s_ntype.s_root.s_cardinality++;
      target_root->s_hash_code ^= target_hash_code;

      /* expand the set header if necessary */

      if (target_root->s_ntype.s_root.s_cardinality > expansion_trigger) {

         target_root = set_expand_header(SETL_SYSTEM target_root);
         expansion_trigger *= SET_HASH_SIZE;

      }
   }
}

case '}' :

{

   start = lookahead;

   return RBRACE;

}

/*\
 *  \case{tuple}
 *
 *  When we find an opening bracket we start entering subsequent items into
 *  a tuple.  We return when we find the closing bracket.
\*/

case '[' :

{
tuple_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
int target_height, target_index;       /* current height and index          */
specifier target_element;              /* tuple element                     */
int32 target_number;                   /* target element number             */
tuple_h_ptr_type new_hdr;              /* created header node               */
tuple_c_ptr_type new_cell;             /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int i;                                 /* temporary looping variable        */

   /* advance past the opening bracket */

   start = lookahead;

   /* create a new tuple for the target */

   get_tuple_header(target_root);
   target_root->t_use_count = 1;
   target_root->t_hash_code = 0;
   target_root->t_ntype.t_root.t_length = 0;
   target_root->t_ntype.t_root.t_height = 0;
   for (i = 0;
        i < TUP_HEADER_SIZE;
        target_root->t_child[i++].t_cell = NULL);
   expansion_trigger = TUP_HEADER_SIZE;

   /* insert elements until we find a right brace */

   for (;;) {

      /* get the next item from the input stream */

      target_element.sp_form = ft_omega;

      i = read_spec(SETL_SYSTEM &target_element);
      if (i == RBRACKET) 
         break;
      if (i != SPEC)
         abend(SETL_SYSTEM msg_not_setl_value);

      /*
       *  At this point we have an element we would like to insert into
       *  the target.
       */

      target_number = target_root->t_ntype.t_root.t_length++;

      /* expand the target header if necessary */

      if (target_root->t_ntype.t_root.t_length >= expansion_trigger) {

         target_work_hdr = target_root;

         get_tuple_header(target_root);

         target_root->t_use_count = 1;
         target_root->t_hash_code =
               target_work_hdr->t_hash_code;
         target_root->t_ntype.t_root.t_length =
               target_work_hdr->t_ntype.t_root.t_length;
         target_root->t_ntype.t_root.t_height =
               target_work_hdr->t_ntype.t_root.t_height + 1;

         for (i = 1;
              i < TUP_HEADER_SIZE;
              target_root->t_child[i++].t_header = NULL);

         target_root->t_child[0].t_header = target_work_hdr;

         target_work_hdr->t_ntype.t_intern.t_parent = target_root;
         target_work_hdr->t_ntype.t_intern.t_child_index = 0;

         expansion_trigger *= TUP_HEADER_SIZE;

      }

      /* descend the header tree until we get to a leaf */

      target_work_hdr = target_root;
      for (target_height = target_root->t_ntype.t_root.t_height;
           target_height;
           target_height--) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * TUP_SHIFT_DIST)) &
                           TUP_SHIFT_MASK;

         /* if we're missing a header record, allocate one */

         if (target_work_hdr->t_child[target_index].t_header == NULL) {

            get_tuple_header(new_hdr);
            new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
            new_hdr->t_ntype.t_intern.t_child_index = target_index;

            for (i = 0;
                 i < TUP_HEADER_SIZE;
                 new_hdr->t_child[i++].t_cell = NULL);

            target_work_hdr->t_child[target_index].t_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->t_child[target_index].t_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  We insert the new element in the appropriate slot.
       */

      if (target_element.sp_form == ft_omega)
         continue;
      get_tuple_cell(new_cell);
      new_cell->t_spec.sp_form = target_element.sp_form;
      new_cell->t_spec.sp_val.sp_biggest =
         target_element.sp_val.sp_biggest;
      spec_hash_code(new_cell->t_hash_code,&target_element);
      target_index = target_number & TUP_SHIFT_MASK;
      target_work_hdr->t_child[target_index].t_cell = new_cell;
      target_root->t_hash_code ^= new_cell->t_hash_code;

   }

   /*
    *  I don't really know why, but someone might just type a bunch of
    *  OM's at the end of a tuple.  I have to get rid of them.
    */

   /* if the length is zero, don't try this */

   if (target_root->t_ntype.t_root.t_length == 0) {

      unmark_specifier(spec);
      spec->sp_form = ft_tuple;
      spec->sp_val.sp_tuple_ptr = target_root;

      return SPEC;

   }

   /* drop to a leaf at the rightmost position */

   target_number = target_root->t_ntype.t_root.t_length - 1;
   target_work_hdr = target_root;
   for (target_height = target_root->t_ntype.t_root.t_height;
        target_height;
        target_height--) {

      /* extract the element's index at this level */

      target_index = (target_number >>
                           (target_height * TUP_SHIFT_DIST)) &
                        TUP_SHIFT_MASK;

      /* if we're missing a header record, allocate one */

      if (target_work_hdr->t_child[target_index].t_header == NULL) {

         get_tuple_header(new_hdr);
         new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
         new_hdr->t_ntype.t_intern.t_child_index = target_index;

         for (i = 0;
              i < TUP_HEADER_SIZE;
              new_hdr->t_child[i++].t_cell = NULL);

         target_work_hdr->t_child[target_index].t_header = new_hdr;
         target_work_hdr = new_hdr;

      }
      else {

         target_work_hdr =
            target_work_hdr->t_child[target_index].t_header;

      }
   }

   /* set the target index to the last element */

   target_index = target_number & TUP_SHIFT_MASK;

   /* keep stripping omegas */

   for (;;) {

      if (!target_height && target_index >= 0) {

         if (target_work_hdr->t_child[target_index].t_cell != NULL)
            break;

         target_root->t_ntype.t_root.t_length--;
         target_index--;

         continue;

      }

      /* move up if we're at the end of a node */

      if (target_index < 0) {

         if (target_work_hdr == target_root)
            break;

         target_height++;
         target_index = target_work_hdr->t_ntype.t_intern.t_child_index;
         target_work_hdr = target_work_hdr->t_ntype.t_intern.t_parent;
         free_tuple_header(target_work_hdr->t_child[target_index].t_header);
         target_work_hdr->t_child[target_index].t_header = NULL;
         target_index--;

         continue;

      }

      /* skip over null nodes */

      if (target_work_hdr->t_child[target_index].t_header == NULL) {

         target_root->t_ntype.t_root.t_length -=
               1L << (target_height * TUP_SHIFT_DIST);
         target_index--;

         continue;

      }

      /* otherwise drop down a level */

      target_work_hdr =
         target_work_hdr->t_child[target_index].t_header;
      target_index = TUP_HEADER_SIZE - 1;

      target_height--;

   }

   /* we've shortened the tuple -- now reduce the height */

   while (target_root->t_ntype.t_root.t_height > 0 &&
          target_root->t_ntype.t_root.t_length <=
             (int32)(1L << (target_root->t_ntype.t_root.t_height *
                           TUP_SHIFT_DIST))) {

      target_work_hdr = target_root->t_child[0].t_header;

      /* it's possible that we deleted internal headers */

      if (target_work_hdr == NULL) {

         target_root->t_ntype.t_root.t_height--;
         continue;

      }

      /* delete the root node */

      target_work_hdr->t_use_count = target_root->t_use_count;
      target_work_hdr->t_hash_code =
            target_root->t_hash_code;
      target_work_hdr->t_ntype.t_root.t_length =
            target_root->t_ntype.t_root.t_length;
      target_work_hdr->t_ntype.t_root.t_height =
            target_root->t_ntype.t_root.t_height - 1;

      free_tuple_header(target_root);
      target_root = target_work_hdr;

   }

   /* finally, we set the target value */

   unmark_specifier(spec);
   spec->sp_form = ft_tuple;
   spec->sp_val.sp_tuple_ptr = target_root;

   return SPEC;

}

case ']' :

{

   start = lookahead;

   return RBRACKET;

}

/*\
 *  \case{commas}
 *
 *  We allow commas to appear between items in the input stream, but only
 *  one between each pair.
\*/

case ',' :

{

   if (found_comma)
      abend(SETL_SYSTEM msg_not_setl_value);

   found_comma = YES;
   start = lookahead;
   advance_la;

   continue;

}

/*\
 *  \case{errors}
 *
 *  If we get here, we must have a lexical error.
\*/

default :

{

   abend(SETL_SYSTEM msg_not_setl_value);

}

/* back to normal indentation */

      }
   }

   return ENDOFFILE;

}

/*\
 *  \function{fill\_buffer()}
 *
 *  This function loads the file buffer from an input file.  First we
 *  shift the current buffer from the start of the current token to the
 *  front of the source buffer.  We then read from the source file to the
 *  lookahead pointer.  We never lose part of the current token by
 *  overwriting it.
 *
 *  This scheme works reasonably efficiently provided the buffer size is
 *  considerably longer than the average token size.  We don't see why
 *  this would not be the case.
\*/
      
static void fill_buffer(
   SETL_SYSTEM_PROTO_VOID)

{
int readcount;                         /* number of characters read         */
unsigned char *s,*t;                   /* temporary looping variables       */

#ifdef TRAPS

   if (lookahead - start > MAX_LOOKAHEAD)
      giveup(SETL_SYSTEM "Interpreter error -- token too long in fill_buffer()");

#endif

   /* shift the current token to the start of the source buffer */

   for (t = file_buffer, s = start; s < lookahead; *t++ = *s++);
   start = file_buffer;
   lookahead = t;

   /* read a block starting at the lookahead pointer */

   if (file_ptr == reads_ptr) {

      /* copy the source string */

      while (reads_length &&
             t < lookahead + FILE_BUFF_SIZE) {

         if (reads_char_ptr == reads_char_end) {

            reads_cell = reads_cell->s_next;
            reads_char_ptr = reads_cell->s_cell_value;
            reads_char_end = reads_char_ptr + STR_CELL_WIDTH;

         }

         *t++ = *reads_char_ptr++;
         reads_length--;

      }

      if (!reads_length) {
         endofbuffer = t;
         eof_ptr = t;
         *eof_ptr = EOFCHAR;
      }
      else {
         endofbuffer = t - 1;
      }

   }
   else if (file_ptr == stdin_ptr) {

#ifdef FIXMEPLUGIN
      javascript_buffer=NULL;
      JavaScript("javascript:__setl2input=prompt('SETL2 Input','');");
      strcpy((char *)lookahead,"");
      if (javascript_buffer!=NULL) {
         strcpy((char *)lookahead,javascript_buffer);
	 free(javascript_buffer);
	 javascript_buffer=NULL;
      }
      
      strcat((char *)lookahead,"\n");
#else
      if (SAFE_MODE==0) fputs(":",stdout);

      if (fgets((char *)lookahead,FILE_BUFF_SIZE,stdin) == NULL) {

         if (feof(stdin)) {
            endofbuffer = lookahead;
            eof_ptr = lookahead;
            *eof_ptr = EOFCHAR;
         }
         else {
            abend(SETL_SYSTEM "I/O error on input file");
         }

      }
      else {
#endif
      /*
       *  Special check for Mac weirdness: the Mac reads the ':' prompt
       *  as part of the input string!  If the user forgets to delete it
       *  he's in trouble, since ':' is not a valid SETL2 value.
       */

#if MPWC

         if (*lookahead == ':')
            lookahead++;

#endif

         for (endofbuffer = lookahead;
              *endofbuffer;
              endofbuffer++);
         endofbuffer--;

      }
#ifndef FIXMEPLUGIN
   }
#endif
   else if (file_ptr->f_mode == TCP ) {
      if ((readcount = read(file_ptr->f_file_fd,
			     (void *)lookahead,
                             (size_t)FILE_BUFF_SIZE)) <0)
         giveup(SETL_SYSTEM "Error reading from socket %s",file_name);

      /* adjust the end of buffer pointer */

      if (readcount == 0) {
         eof_ptr = t;
         *eof_ptr = EOFCHAR;
         endofbuffer = lookahead;
      }
      else {
         endofbuffer = lookahead + readcount - 1;
      }

   }
   else {

      if ((readcount = fread((void *)lookahead,
                             sizeof(char),
                             (size_t)FILE_BUFF_SIZE,
                             file_stream)) < 0)
         giveup(SETL_SYSTEM "Disk error reading %s",file_name);

      if (ferror(file_stream) != 0)
         giveup(SETL_SYSTEM "Disk error reading %s",file_name);

      /* adjust the end of buffer pointer */

      if (readcount == 0) {
         eof_ptr = t;
         *eof_ptr = EOFCHAR;
         endofbuffer = lookahead;
      }
      else {
         endofbuffer = lookahead + readcount - 1;
      }

   }

   return;

}

/*\
 *  \function{setl2\_print()}
 *
 *  This function handles formatted output to standard output.  It can
 *  print any SETL value, but not in a form in which they can be read
 *  back in.
\*/

void setl2_print(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector                   */
   specifier *target)                  /* return value                      */

{
specifier *arg;                        /* used to loop over arguments       */

   /* we use standard output */

   file_stream = stdout;
   file_fd = -1;

   /* print each argument */

   for (arg = argv; arg < argv + argc; arg++) {

      print_spec(SETL_SYSTEM arg);

   }

   /* print a newline and return */

   fputs("\n",file_stream);

#if MACINTOSH

   /* we flush on blank lines on the Mac, it buffers too much! */

   if (!argc)
      fflush(file_stream);

#endif

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_nprint()}
 *
 *  This function is identical to \verb"print", with the exception that
 *  it doesn't automatically append a newline.
\*/

void setl2_nprint(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector                   */
   specifier *target)                  /* return value                      */

{
specifier *arg;                        /* used to loop over arguments       */

   /* we use standard output */

   file_stream = stdout;
   file_fd = -1;

   /* print each argument */

   for (arg = argv; arg < argv + argc; arg++) {

      print_spec(SETL_SYSTEM arg);

   }

#if MACINTOSH

   /* we flush on blank lines on the Mac, it buffers too much! */

   if (!argc)
      fflush(file_stream);

#endif

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_printa()}
 *
 *  This function works like \verb"print", except that it prints to a
 *  file opened for text output.
\*/

void setl2_printa(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */
specifier *arg;                        /* used to loop over arguments       */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;

   if ((file_ptr->f_type == 1)&&(file_ptr->f_mode == TEXT_IN))
      file_ptr->f_mode = TEXT_OUT;

   if ((file_ptr->f_mode != TEXT_OUT)&&(file_ptr->f_mode != TCP))
      abend(SETL_SYSTEM "Attempt to print to file not opened for TEXT-OUT:\nFile => %s",
             file_ptr->f_file_name);
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;

   /* print each argument */
   for (arg = argv + 1; arg < argv + argc; arg++) {

      print_spec(SETL_SYSTEM arg);

   }

   /* print a newline and return */
   if (file_fd<0) 
      fputs("\n",file_stream);
   else 
      write(file_fd,"\n",1);

#if MACINTOSH

   /* we flush on blank lines on the Mac, it buffers too much! */

   if ((!argc)&&(file_fd<0))
      fflush(file_stream);

#endif

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_nprinta()}
 *
 *  This function is identical to \verb"printa", with the exception that
 *  it doesn't automatically append a newline.
\*/

void setl2_nprinta(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */
specifier *arg;                        /* used to loop over arguments       */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;

   if ((file_ptr->f_type == 1)&&(file_ptr->f_mode == TEXT_IN))
      file_ptr->f_mode = TEXT_OUT;

   if ((file_ptr->f_mode != TEXT_OUT)&&(file_ptr->f_mode != TCP))
      abend(SETL_SYSTEM "Attempt to print to file not opened for TEXT-OUT:\nFile => %s",
            file_ptr->f_file_name);
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;

   /* print each argument */

   for (arg = argv + 1; arg < argv + argc; arg++) {

      print_spec(SETL_SYSTEM arg);

   }

#if MACINTOSH

   /* we flush on blank lines on the Mac, it buffers too much! */

   if ((!argc)&&(file_fd<0))
      fflush(file_stream);

#endif

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{print_to_stream_or_fd()}
 *
 *  This function prints a string to a stream or a file descriptor
\*/

void print_to_stream_or_fd(char *string) 
{
   if (file_fd<0) 
      fputs(string,file_stream);
   else
      write(file_fd,string,strlen(string));

}

/*\
 *  \function{print\_spec()}
 *
 *  This function prints one specifier on a stream file.
\*/
      
static void print_spec(
   SETL_SYSTEM_PROTO
   specifier *spec)                    /* specifier to be printed           */

{
char string_buffer[64];
   
   switch (spec->sp_form) {

/*\
 *  \case{unprintable types}
 *
 *  We have a few types which we hope will not be printed, since we can
 *  not print anything meaningful for them.  We do allow these types to
 *  be printed, but just print something to let the operator know the
 *  type of the thing he printed.
\*/

case ft_omega :

{

   print_to_stream_or_fd("<om>");

   return;

}

case ft_atom :

{

   if (spec->sp_val.sp_atom_num == spec_true->sp_val.sp_atom_num)
      print_to_stream_or_fd("TRUE");
   else if (spec->sp_val.sp_atom_num == spec_false->sp_val.sp_atom_num)
      print_to_stream_or_fd("FALSE");
   else {
      sprintf(string_buffer,"<atom %ld>",spec->sp_val.sp_atom_num);
      print_to_stream_or_fd(string_buffer);
   }
   return;

}

case ft_opaque :

{

#if WATCOM

	sprintf(string_buffer,"<opaque %p>",spec->sp_val.sp_opaque_ptr);

#else

	sprintf(string_buffer,"<opaque %ld>",(int32)(spec->sp_val.sp_opaque_ptr));

#endif
   print_to_stream_or_fd(string_buffer);

   return;

}

case ft_label :

{

#if WATCOM

   sprintf(string_buffer,"<label %p>",spec->sp_val.sp_label_ptr);

#else

   sprintf(string_buffer,"<label %ld>",(int32)(spec->sp_val.sp_label_ptr));

#endif
   print_to_stream_or_fd(string_buffer);

   return;

}

case ft_file :

{

#if WATCOM

   sprintf(string_buffer,"<file %p>",spec->sp_val.sp_file_ptr);

#else

   sprintf(string_buffer,"<file %ld>",(int32)(spec->sp_val.sp_file_ptr));

#endif
   print_to_stream_or_fd(string_buffer);

   return;

}

case ft_proc :

{

#if WATCOM

   sprintf(string_buffer,"<procedure %p>",spec->sp_val.sp_proc_ptr);

#else

   sprintf(string_buffer,"<procedure %ld>",(int32)(spec->sp_val.sp_proc_ptr));

#endif
   print_to_stream_or_fd(string_buffer);

   return;

}

case ft_mailbox :

{
mailbox_c_ptr_type cell_ptr;           /* mailbox cell                      */
int first_element;                     /* first entry switch                */

#if WATCOM

   sprintf(string_buffer,"<mailbox %p",spec->sp_val.sp_mailbox_ptr);

#else

   sprintf(string_buffer,"<mailbox %ld",(int32)(spec->sp_val.sp_mailbox_ptr));

#endif
   print_to_stream_or_fd(string_buffer);

   first_element = YES;
   for (cell_ptr = spec->sp_val.sp_mailbox_ptr->mb_head;
        cell_ptr != NULL;
        cell_ptr = cell_ptr->mb_next) {

      if (!first_element)
         print_to_stream_or_fd(",");
      else
         first_element = NO;

      print_to_stream_or_fd(" ");
      print_spec(SETL_SYSTEM &(cell_ptr->mb_spec));

   }

   print_to_stream_or_fd(">");

   return;

}

case ft_iter :

{

#if WATCOM

   sprintf(string_buffer,"<iterator %p>",spec->sp_val.sp_iter_ptr);

#else

   sprintf(string_buffer,"<iterator %ld>",(int32)(spec->sp_val.sp_iter_ptr));

#endif
   print_to_stream_or_fd(string_buffer);

   return;

}

/*\
 *  \case{integers}
 *
 *  We have two kinds of integers: short and long.  Short integers
 *  we can normally handle quite easily, but longs are more work.
\*/

case ft_short :

{

   sprintf(string_buffer,"%ld",spec->sp_val.sp_short_value);
   print_to_stream_or_fd(string_buffer);
   return;

}

case ft_long :

{
char *p;                               /* printable integer string          */

   p = integer_string(SETL_SYSTEM spec,10);
   print_to_stream_or_fd(p);
   free(p);

   return;

}

/*\
 *  \case{real numbers}
 *
 *  We depend on the C library to do most of the work in printing a real
 *  number.
\*/

case ft_real :

{

   sprintf(string_buffer,"%#.11g",(spec->sp_val.sp_real_ptr)->r_value);
   print_to_stream_or_fd(string_buffer);

   return;

}

/*\
 *  \case{strings}
 *
 *  Strings are complex structures, because we allow infinite length.  We
 *  have to print each cell individually, and translate nulls to blanks.
\*/

case ft_string :

{
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
int32 chars_to_print;                  /* characters left in string         */
char cell_string[STR_CELL_WIDTH + 1];  /* printable cell string             */
char *s, *t;                           /* temporary looping variables       */

   /* loop over the cells ... */

   string_hdr = spec->sp_val.sp_string_ptr;
   chars_to_print = string_hdr->s_length;
   for (string_cell = string_hdr->s_head;
        chars_to_print && string_cell != NULL;
        string_cell = string_cell->s_next) {

      /* translate nulls to spaces */

      for (s = string_cell->s_cell_value, t = cell_string;
           s < string_cell->s_cell_value + STR_CELL_WIDTH;
           s++, t++) {

         if (*s == '\0')
            *t = ' ';
         else
            *t = *s;

      }

      /* print the cell (or as much as necessary) */

      if (chars_to_print < STR_CELL_WIDTH) {

         cell_string[chars_to_print] = '\0';
         chars_to_print = 0;

      }
      else {

         cell_string[STR_CELL_WIDTH] = '\0';
         chars_to_print -= STR_CELL_WIDTH;

      }

      print_to_stream_or_fd(cell_string);

   }

   return;

}

/*\
 *  \case{sets}
 *
 *  We loop over the elements of a set printing each.  When we encounter
 *  a string we print enclosing quotes.
\*/

case ft_set :

{
int first_element;                     /* YES for the first element         */
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */

   /* print the opening brace, and start looping over the set */

   print_to_stream_or_fd("{");
   first_element = YES;

   source_root = spec->sp_val.sp_set_ptr;
   source_work_hdr = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_cell = NULL;
   source_index = 0;

   /* loop over the elements of source */

   for (;;) {

      /* find the next element in the set */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, break */

         if (source_cell != NULL) {

            source_element = &(source_cell->s_spec);
            source_cell = source_cell->s_next;
            break;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < SET_HASH_SIZE) {

            source_cell = source_work_hdr->s_child[source_index].s_cell;
            source_index++;
            continue;

         }

         /* move up if we're at the end of a node */

         if (source_index >= SET_HASH_SIZE) {

            /* there are no more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->s_ntype.s_intern.s_child_index + 1;
            source_work_hdr =
               source_work_hdr->s_ntype.s_intern.s_parent;
            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->s_child[source_index].s_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->s_child[source_index].s_header;
         source_index = 0;
         source_height--;

      }

      /* if we've exhausted the set break again */

      if (source_element == NULL)
         break;

      /*
       *  At this point we have an element in source_element which must
       *  be printed.
       */

      /* print a comma after the previous element */

      if (first_element)
         first_element = NO;
      else
         print_to_stream_or_fd(", ");

      /* print enclosing quotes around strings */

      if (source_element->sp_form == ft_string) {

         print_to_stream_or_fd("\"");
         print_spec(SETL_SYSTEM source_element);
         print_to_stream_or_fd("\"");

      }

      /* otherwise, just print the element */

      else {

         print_spec(SETL_SYSTEM source_element);

      }
   }

   /* that's it */

   print_to_stream_or_fd("}");

   return;

}

/*\
 *  \case{maps}
 *
 *  We loop over the elements of a map printing each pair.  When we
 *  encounter a string we print enclosing quotes.
\*/

case ft_map :

{
int first_element;                     /* YES for the first element         */
map_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type valset_root, valset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type valset_cell;            /* current cell pointer              */
int valset_height, valset_index;       /* current height and index          */
specifier *domain_element, *range_element;
                                       /* pair from map                     */

   /* print the opening brace, and start looping over the map */

   print_to_stream_or_fd("{");
   first_element = YES;

   source_root = spec->sp_val.sp_map_ptr;
   source_work_hdr = source_root;
   source_height = source_root->m_ntype.m_root.m_height;
   source_cell = NULL;
   source_index = 0;
   valset_root = NULL;

   /* loop over the elements of source */

   for (;;) {

      /* find the next cell in the map */

      while (source_cell == NULL) {

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < MAP_HASH_SIZE) {

            source_cell = source_work_hdr->m_child[source_index].m_cell;
            source_index++;
            continue;

         }


         /* move up if we're at the end of a node */

         if (source_index >= MAP_HASH_SIZE) {

            /* there are no more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->m_ntype.m_intern.m_child_index + 1;
            source_work_hdr =
               source_work_hdr->m_ntype.m_intern.m_parent;
            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->m_child[source_index].m_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->m_child[source_index].m_header;
         source_index = 0;
         source_height--;

      }

      /* if there are no more cells, break */

      if (source_cell == NULL)
         break;

      /* if the cell is not multi-value, use the pair directly */

      if (!(source_cell->m_is_multi_val)) {

         domain_element = &(source_cell->m_domain_spec);
         range_element = &(source_cell->m_range_spec);
         source_cell = source_cell->m_next;

      }

      /* otherwise find the next element in the value set */

      else {

         domain_element = &(source_cell->m_domain_spec);
         if (valset_root == NULL) {

            valset_root = source_cell->m_range_spec.sp_val.sp_set_ptr;
            valset_work_hdr = valset_root;
            valset_height = valset_root->s_ntype.s_root.s_height;
            valset_cell = NULL;
            valset_index = 0;

         }

         range_element = NULL;
         for (;;) {

            /* if we have an element already, break */

            if (valset_cell != NULL) {

               range_element = &(valset_cell->s_spec);
               valset_cell = valset_cell->s_next;
               break;

            }

            /* start on the next clash list, if we're at a leaf */

            if (!valset_height && valset_index < SET_HASH_SIZE) {

               valset_cell = valset_work_hdr->s_child[valset_index].s_cell;
               valset_index++;
               continue;

            }

            /* the current header node is exhausted -- find the next one */

            if (valset_index >= SET_HASH_SIZE) {

               /* there are no more elements, so break */

               if (valset_work_hdr == valset_root)
                  break;

               /* otherwise move up */

               valset_height++;
               valset_index =
                  valset_work_hdr->s_ntype.s_intern.s_child_index + 1;
               valset_work_hdr =
                  valset_work_hdr->s_ntype.s_intern.s_parent;
               continue;

            }

            /* skip over null nodes */

            if (valset_work_hdr->s_child[valset_index].s_header == NULL) {

               valset_index++;
               continue;

            }

            /* otherwise drop down a level */

            valset_work_hdr =
               valset_work_hdr->s_child[valset_index].s_header;
            valset_index = 0;
            valset_height--;

         }

         if (range_element == NULL) {

            source_cell = source_cell->m_next;
            valset_root = NULL;
            continue;

         }
      }

      /*
       *  At this point we have a pair from the map which we would like to
       *  print.
       */

      /* print a comma after the previous element */

      if (first_element)
         first_element = NO;
      else
         print_to_stream_or_fd(", ");

      /* print the domain element */

       print_to_stream_or_fd("[");

      if (domain_element->sp_form == ft_string) {

         print_to_stream_or_fd("\"");
         print_spec(SETL_SYSTEM domain_element);
         print_to_stream_or_fd("\"");

      }

      /* otherwise, just print the element */

      else {

         print_spec(SETL_SYSTEM domain_element);

      }

      /* print the range element */

       print_to_stream_or_fd(", ");

      if (range_element->sp_form == ft_string) {

         print_to_stream_or_fd("\"");
         print_spec(SETL_SYSTEM range_element);
         print_to_stream_or_fd("\"");

      }

      /* otherwise, just print the element */

      else {

         print_spec(SETL_SYSTEM range_element);

      }

       print_to_stream_or_fd("]");

   }

   /* that's it */

   fputs("}",file_stream);

   return;

}

/*\
 *  \case{tuples}
 *
 *  We loop over the elements of a tuple printing each. When we encounter
 *  a string we print enclosing quotes.
\*/

case ft_tuple :

{
int32 printed_number;                  /* number of elements printed        */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */

   /* print the opening bracket, and start looping over the tuple */

   print_to_stream_or_fd("[");
   printed_number = 0;

   source_root = spec->sp_val.sp_tuple_ptr;
   source_work_hdr = source_root;
   source_number = -1;
   source_height = source_root->t_ntype.t_root.t_height;
   source_index = 0;

   /* loop over the elements of source */

   while (source_number < source_root->t_ntype.t_root.t_length) {

      /* find the next element in the tuple */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, return it */

         if (!source_height && source_index < TUP_HEADER_SIZE) {

            if (source_work_hdr->t_child[source_index].t_cell == NULL) {

               source_number++;
               source_index++;

               continue;

            }

            source_cell = source_work_hdr->t_child[source_index].t_cell;
            source_element = &(source_cell->t_spec);

            source_number++;
            source_index++;

            break;

         }

         /* move up if we're at the end of a node */

         if (source_index >= TUP_HEADER_SIZE) {

            /* break if we've exhausted the source */

            if (source_work_hdr == source_root)
               break;

            source_height++;
            source_index =
               source_work_hdr->t_ntype.t_intern.t_child_index + 1;
            source_work_hdr =
               source_work_hdr->t_ntype.t_intern.t_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->t_child[source_index].t_header == NULL) {

            source_number += 1L << (source_height * TUP_SHIFT_DIST);
            source_index++;

            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      if (source_element == NULL)
         break;

      /*
       *  At this point we have an element in source_element which must
       *  be printed.  We might have to print a bunch of OM's first
       *  though.
       */

      /* print a comma after the previous element */

      if (printed_number != 0)
         print_to_stream_or_fd(", ");

      while (printed_number++ < source_number) {

         print_to_stream_or_fd("<om>");

      }

      /* print enclosing quotes around strings */

      if (source_element->sp_form == ft_string) {

         print_to_stream_or_fd("\"");
         print_spec(SETL_SYSTEM source_element);
         print_to_stream_or_fd("\"");

      }

      /* otherwise, just print the element */

      else {

         print_spec(SETL_SYSTEM source_element);

      }
   }

   /* that's it */

   print_to_stream_or_fd("]");

   return;

}

/*\
 *  \case{objects}
 *
 *  We use the \verb"str" procedure, and perhaps the corresponding
 *  method, to find the printable string for an object.  Then we print
 *  the string.
\*/

case ft_object :
case ft_process :

{
specifier spare;                       /* spare specifier                   */

   spare.sp_form = ft_omega;
   setl2_str(SETL_SYSTEM 1L,spec,&spare);
   print_spec(SETL_SYSTEM &spare);
   unmark_specifier(&spare);

   return;

}

/* back to normal indentation */

   }

   return;

}

/*\
 *  \function{setl2\_getb()}
 *
 *  This function gets one item from a binary file.
\*/

void setl2_getb(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
int32 string_length;                   /* characters left in string         */
int cell_length;                       /* characters in cell to be printed  */
specifier spare1, spare2;              /* spares for unbinstr               */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;
   if (file_ptr->f_mode != BINARY_IN)
      abend(SETL_SYSTEM "Attempt to getb from file not opened for BINARY-IN:\nFile => %s",
            file_ptr->f_file_name);
   file_name = file_ptr->f_file_name;
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;

   /* convert each value to internal */

   eof_flag = NO;
   while (argc-- > 1) {

      if (eof_flag) {
         spare1.sp_form = ft_omega;
         push_pstack(&spare1);
         continue;
      }

      /* read in a string */

      if (fread((void *)&string_length,
                sizeof(int32),
                (size_t)1,
                file_stream) != (size_t)1) {

         if (!feof(file_stream))  
            abend(SETL_SYSTEM "Error reading file => %s",file_name);

         eof_flag = YES;
         spare1.sp_form = ft_omega;
         push_pstack(&spare1);
         continue;

      }

      /* initialize a string structure */

      get_string_header(string_hdr);
      string_hdr->s_use_count = 1;
      string_hdr->s_hash_code = -1;
      string_hdr->s_length = string_length;
      string_hdr->s_head = string_hdr->s_tail = string_cell = NULL;

      /* read each cell */

      while (string_length) {

         get_string_cell(string_cell);
         if (string_hdr->s_tail != NULL)
            (string_hdr->s_tail)->s_next = string_cell;
         string_cell->s_prev = string_hdr->s_tail;
         string_cell->s_next = NULL;
         string_hdr->s_tail = string_cell;
         if (string_hdr->s_head == NULL)
            string_hdr->s_head = string_cell;

         cell_length = min(STR_CELL_WIDTH,string_length);
         if (fread((void *)string_cell->s_cell_value,
                   sizeof(char),
                   (size_t)cell_length,
                   file_stream) != (size_t)cell_length)
            abend(SETL_SYSTEM "Error reading file => %s",file_name);

         string_length -= cell_length;

      }

      /* use unbinstr to convert to internal form */

      spare1.sp_form = ft_string;
      spare1.sp_val.sp_string_ptr = string_hdr;
      spare2.sp_form = ft_omega;
      setl2_unbinstr(SETL_SYSTEM 1L,&spare1,&spare2);
      push_pstack(&spare2);
      unmark_specifier(&spare1);
      unmark_specifier(&spare2);

   }

   /* return om */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_putb()}
 *
 *  This function writes specifiers to a binary file.
\*/

void setl2_putb(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
int32 string_length;                   /* characters left in string         */
int cell_length;                       /* characters in cell to be printed  */
specifier *arg;                        /* used to loop over arguments       */
specifier spare;                       /* conversion to string              */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;
   if (file_ptr->f_mode != BINARY_OUT)
      abend(SETL_SYSTEM "Attempt to putb to file not opened for BINARY-OUT:\nFile: => %s",
            file_ptr->f_file_name);
   file_stream = file_ptr->f_file_stream;
   file_name = file_ptr->f_file_name;
   file_fd = file_ptr->f_file_fd;

   /* write each argument */

   for (arg = argv + 1; arg < argv + argc; arg++) {

      /* convert the specifier to a string */

      spare.sp_form = ft_omega;
      setl2_binstr(SETL_SYSTEM 1L,arg,&spare);

      /* write it to the file */

      string_hdr = spare.sp_val.sp_string_ptr;
      string_length = string_hdr->s_length;

      /* save the length of string */

      if (fwrite((void *)&string_length,
                 sizeof(int32),
                 (size_t)1,
                 file_stream) != (size_t)1)
            abend(SETL_SYSTEM "Error writing file => %s",file_name);

      /* loop over the cells ... */

      for (string_cell = string_hdr->s_head;
           string_length && string_cell != NULL;
           string_cell = string_cell->s_next) {

         cell_length = min(STR_CELL_WIDTH,string_length);
         if (fwrite((void *)string_cell->s_cell_value,
                    sizeof(char),
                    (size_t)cell_length,
                    file_stream) != (size_t)cell_length)
            abend(SETL_SYSTEM "Error writing file => %s",file_name);

         string_length -= cell_length;

      }

      /* we're done with spare */

      unmark_specifier(&spare);

   }

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_gets()}
 *
 *  This function gets one string from a random file.
\*/

void setl2_gets(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int32 start_position;
int32 string_length;
int32 cell_size;
string_h_ptr_type string_hdr;
                                       /* source and target strings         */
string_c_ptr_type string_cell;
                                       /* source and target string cells    */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;
   if (file_ptr->f_mode != RANDOM)
      abend(SETL_SYSTEM "Attempt to gets from file not opened for RANDOM:\nFile => %s",
            file_ptr->f_file_name);
   file_name = file_ptr->f_file_name;
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;

   /* the second and third arguments must be integers */

   if (argv[1].sp_form == ft_short) {

      start_position = (int32)(argv[1].sp_val.sp_short_value);

   }
   else if (argv[1].sp_form == ft_long) {

      start_position = (int32)long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);

   }
   else {

      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"gets",
            abend_opnd_str(SETL_SYSTEM argv+1));

   }

   /* C files are zero-based */

   start_position--;

   if (argv[2].sp_form == ft_short) {

      string_length = (int32)(argv[2].sp_val.sp_short_value);

   }
   else if (argv[2].sp_form == ft_long) {

      string_length = (int32)long_to_short(SETL_SYSTEM argv[2].sp_val.sp_long_ptr);

   }
   else {

      abend(SETL_SYSTEM msg_bad_arg,"integer",3,"gets",
            abend_opnd_str(SETL_SYSTEM argv+2));


   }

   /* position the file */

   if (fseek(file_stream,start_position,SEEK_SET) != 0)
      abend(SETL_SYSTEM "Seek failed on file => %s", file_name);

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = string_length;
   string_hdr->s_head = string_hdr->s_tail = NULL;

   /* build up the string */

   while (string_length) {

      cell_size = min(string_length,STR_CELL_WIDTH);
      string_length -= cell_size;

      get_string_cell(string_cell);
      if (string_hdr->s_tail != NULL)
         (string_hdr->s_tail)->s_next = string_cell;
      string_cell->s_prev = string_hdr->s_tail;
      string_cell->s_next = NULL;
      string_hdr->s_tail = string_cell;
      if (string_hdr->s_head == NULL)
         string_hdr->s_head = string_cell;

      if (fread((void *)string_cell->s_cell_value,
                (size_t)cell_size,
                (size_t)1,
                file_stream) != (size_t)1)
         abend(SETL_SYSTEM "Read error on file => %s",file_name);

   }

   /* push the output string and return OM */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;
   string_hdr->s_use_count--;
   push_pstack(target);

   /* return om */

   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_puts()}
 *
 *  This function puts one string to a random file.
\*/

void setl2_puts(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int32 start_position;
int32 string_length;
int32 cell_size;
string_h_ptr_type string_hdr;
                                       /* source and target strings         */
string_c_ptr_type string_cell;
                                       /* source and target string cells    */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;
   if (file_ptr->f_mode != RANDOM)
      abend(SETL_SYSTEM "Attempt to puts to file not opened for RANDOM:\nFile => %s",
            file_ptr->f_file_name);
   file_name = file_ptr->f_file_name;
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;

   /* the second argument must be an integer */

   if (argv[1].sp_form == ft_short) {

      start_position = (int32)(argv[1].sp_val.sp_short_value);

   }
   else if (argv[1].sp_form == ft_long) {

      start_position = (int32)long_to_short(SETL_SYSTEM argv[1].sp_val.sp_long_ptr);

   }
   else {

      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"puts",
            abend_opnd_str(SETL_SYSTEM argv+1));


   }

   /* C files are zero-based */

   start_position--;

   if ((SAFE_MODE==1)&&(start_position>1024*1024))
      abend(SETL_SYSTEM msg_bad_arg,"integer",2,"puts",
	 abend_opnd_str(SETL_SYSTEM argv+1));

   /* position the file */

#if MACINTOSH

{
char junk_str[STR_CELL_WIDTH];
int32 eof_position;

   if (fseek(file_stream,0L,SEEK_END) != 0)
      abend(SETL_SYSTEM "Seek error on file => %s",file_name);
   eof_position = ftell(file_stream);
   if (eof_position == -1)
      abend(SETL_SYSTEM "Seek error on file => %s",file_name);

   while (eof_position < start_position) {

      if (fwrite((void *)(junk_str),
                 (size_t)STR_CELL_WIDTH,
                 (size_t)1,
                 file_stream) != (size_t)1)
         abend(SETL_SYSTEM "Write error on file => %s",file_name);

      eof_position += STR_CELL_WIDTH;

   }
}

#endif

   if (fseek(file_stream,start_position,SEEK_SET) != 0) 
      abend(SETL_SYSTEM "Seek failed on file => %s", file_name);

   /* initialize the source string */

   if (argv[2].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",3,"puts",
            abend_opnd_str(SETL_SYSTEM argv+2));

   string_hdr = argv[2].sp_val.sp_string_ptr;
   string_length = string_hdr->s_length;
   string_cell = string_hdr->s_head;

   if (string_length+start_position>1024*1024)
      abend(SETL_SYSTEM msg_bad_arg,"string",3,"puts",
            abend_opnd_str(SETL_SYSTEM argv+2));

   /* copy the source until we find something not in the span set */

   while (string_length) {

      cell_size = min(string_length,STR_CELL_WIDTH);
      string_length -= cell_size;

      if (fwrite((void *)string_cell->s_cell_value,
                 (size_t)cell_size,
                 (size_t)1,
                 file_stream) != (size_t)1)
         abend(SETL_SYSTEM "Write error on file => %s",file_name);

      string_cell = string_cell->s_next;

   }

   /* return om */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;

}

/*\
 *  \function{setl2\_fsize()}
 *
 *  This function gets one item from a random file.
\*/

void setl2_fsize(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int32 eof_position;
int32 short_hi_bits;                   /* high order bits of short          */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;

   if ((file_ptr->f_type == 1) && (file_ptr->f_flag == 1)) {

      unmark_specifier(target);
      target->sp_form = ft_omega;

      return;

   }

   file_name = file_ptr->f_file_name;
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;

   if (fseek(file_stream,0L,SEEK_END) != 0)
      abend(SETL_SYSTEM "Seek error on file => %s",file_name);
   eof_position = ftell(file_stream);
   if (eof_position == -1)
      abend(SETL_SYSTEM "Seek error on file => %s",file_name);

   /* check whether the result remains short */

   if (!(short_hi_bits = eof_position & INT_HIGH_BITS) ||
       short_hi_bits == INT_HIGH_BITS) {

      unmark_specifier(target);
      target->sp_form = ft_short;
      target->sp_val.sp_short_value = eof_position;

      return;

   }

   /* if we exceed the maximum short, convert to long */

   short_to_long(SETL_SYSTEM target,eof_position);

   return;

}

/*\
 *  \function{setl2\_eof()}
 *
 *  This function returns true if the last input operation found an end
 *  of file, and false otherwise.
\*/

void setl2_eof(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (none here)       */
   specifier *target)                  /* return value                      */

{

   if (eof_flag) {

      unmark_specifier(target);
      target->sp_form = spec_true->sp_form;
      target->sp_val.sp_biggest = spec_true->sp_val.sp_biggest;

   }
   else {

      unmark_specifier(target);
      target->sp_form = spec_false->sp_form;
      target->sp_val.sp_biggest = spec_false->sp_val.sp_biggest;

   }

   return;

}

/*\
 *  \function{setl2\_binstr()}
 *
 *  This function is the \verb"binstr" built-in function.  We initialize a
 *  string, then call a recursive function which generates the string
 *  contents.
\*/

void setl2_binstr(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (only 1 here)     */
   specifier *target)                  /* return value                      */

{

   /* initialize the return string */

   get_string_header(binstr_curr_hdr);
   binstr_curr_hdr->s_use_count = 1;
   binstr_curr_hdr->s_hash_code = -1;
   binstr_curr_hdr->s_length = 0;
   binstr_curr_hdr->s_head = binstr_curr_hdr->s_tail =
                             binstr_curr_cell = NULL;
   binstr_char_ptr = binstr_char_end = NULL;

   /* call a recursive function to make the string */

   binstr_cat_spec(SETL_SYSTEM argv);

   /* set the return value and return */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = binstr_curr_hdr;

   return;

}

/*\
 *  \function{binstr\_cat\_spec()}
 *
 *  This function appends one specifier on a binary string.
\*/
      
static void binstr_cat_spec(
   SETL_SYSTEM_PROTO
   specifier *spec)                    /* specifier to be appended          */

{

   switch (spec->sp_form) {

/*\
 *  \case{omegas}
 *
 *  The form code is enough to identify an omega.
\*/

case ft_omega :

{

   binstr_cat_string(SETL_SYSTEM (char *)&(spec->sp_form),sizeof(int));

   return;

}

/*\
 *  \case{internal types}
 *
 *  We have a variety of types which we do not permit to be read in
 *  unless they are written in the same execution.  For these is we write
 *  the specifier and the time stamp.
\*/

case ft_atom :          case ft_opaque :        case ft_label :
case ft_file :          case ft_proc :          case ft_iter :
case ft_mailbox :

{

   /*
    *  Make sure we can't release the memory for these values, since
    *  we're only storing pointers!
    */

   mark_specifier(spec);

   binstr_cat_string(SETL_SYSTEM (char *)spec,sizeof(specifier));
   binstr_cat_string(SETL_SYSTEM (char *)&(runtime),sizeof(time_t));

   return;

}

/*\
 *  \case{integers}
 *
 *  We have two kinds of integers: short and long.  Short integers
 *  we can normally handle quite easily, but longs are more work.
\*/

case ft_short :

{

   binstr_cat_string(SETL_SYSTEM (char *)spec,sizeof(specifier));

   return;

}

case ft_long :

{
integer_h_ptr_type integer_hdr;        /* root of long structure            */
integer_c_ptr_type integer_cell;       /* long cell                         */
int32 cell_count;                      /* number of cells in integer        */

   /* we write the form code and number of cells */

   integer_hdr = spec->sp_val.sp_long_ptr;
   cell_count = integer_hdr->i_cell_count;
   if (integer_hdr->i_is_negative)
      cell_count = -cell_count;

   binstr_cat_string(SETL_SYSTEM (char *)&(spec->sp_form),sizeof(int));
   binstr_cat_string(SETL_SYSTEM (char *)&cell_count,sizeof(int32));

   /* write each cell value */

   for (integer_cell = integer_hdr->i_head;
        integer_cell != NULL;
        integer_cell = integer_cell->i_next) {

      binstr_cat_string(SETL_SYSTEM (char *)&(integer_cell->i_cell_value),sizeof(int32));

   }

   return;

}

/*\
 *  \case{real numbers}
 *
 *  We just write the form code and value for reals.
\*/

case ft_real :

{

   binstr_cat_string(SETL_SYSTEM (char *)&(spec->sp_form),sizeof(int));
   binstr_cat_string(SETL_SYSTEM (char *)&((spec->sp_val.sp_real_ptr)->r_value),
                  sizeof(double));

   return;

}

/*\
 *  \case{strings}
 *
 *  Strings are complex structures, because we allow infinite length.  We
 *  have to save each cell individually.
\*/

case ft_string :

{
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
int32 string_length;                   /* characters left in string         */
int cell_length;                       /* characters in cell to be printed  */

   string_hdr = spec->sp_val.sp_string_ptr;
   string_length = string_hdr->s_length;

   /* save the form and length of string */

   binstr_cat_string(SETL_SYSTEM (char *)&(spec->sp_form),sizeof(int));
   binstr_cat_string(SETL_SYSTEM (char *)&string_length,sizeof(int32));

   /* loop over the cells ... */

   for (string_cell = string_hdr->s_head;
        string_length && string_cell != NULL;
        string_cell = string_cell->s_next) {

      cell_length = min(STR_CELL_WIDTH,string_length);
      binstr_cat_string(SETL_SYSTEM string_cell->s_cell_value,(size_t)cell_length);

      string_length -= cell_length;

   }

   return;

}

/*\
 *  \case{sets}
 *
 *  We loop over the elements of a set saving each.
\*/

case ft_set :

{
set_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */

   source_root = spec->sp_val.sp_set_ptr;

   /* save the form and cardinality of set */

   binstr_cat_string(SETL_SYSTEM (char *)&(spec->sp_form),sizeof(int));
   binstr_cat_string(SETL_SYSTEM (char *)&(source_root->s_ntype.s_root.s_cardinality),
                     sizeof(int32));

   source_work_hdr = source_root;
   source_height = source_root->s_ntype.s_root.s_height;
   source_cell = NULL;
   source_index = 0;

   /* loop over the elements of source */

   for (;;) {

      /* find the next element in the set */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, break */

         if (source_cell != NULL) {

            source_element = &(source_cell->s_spec);
            source_cell = source_cell->s_next;
            break;

         }

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < SET_HASH_SIZE) {

            source_cell = source_work_hdr->s_child[source_index].s_cell;
            source_index++;
            continue;

         }

         /* move up if we're at the end of a node */

         if (source_index >= SET_HASH_SIZE) {

            /* there are no more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->s_ntype.s_intern.s_child_index + 1;
            source_work_hdr =
               source_work_hdr->s_ntype.s_intern.s_parent;
            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->s_child[source_index].s_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->s_child[source_index].s_header;
         source_index = 0;
         source_height--;

      }

      /* if we've exhausted the set break again */

      if (source_element == NULL)
         break;

      /*
       *  At this point we have an element in source_element which must
       *  be stored.
       */

      binstr_cat_spec(SETL_SYSTEM source_element);

   }

   return;

}

/*\
 *  \case{maps}
 *
 *  We loop over the elements of a map storing each pair.
\*/

case ft_map :

{
int form_code;                         /* specifier form code               */
int32 map_cardinality;                 /* specifier cardinality             */
map_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
map_c_ptr_type source_cell;            /* current cell pointer              */
int source_height, source_index;       /* current height and index          */
set_h_ptr_type valset_root, valset_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type valset_cell;            /* current cell pointer              */
int valset_height, valset_index;       /* current height and index          */
specifier *domain_element, *range_element;
                                       /* pair from map                     */

   source_root = spec->sp_val.sp_map_ptr;

   /* save the form and cardinality of set */

   form_code = ft_set;
   binstr_cat_string(SETL_SYSTEM (char *)&(form_code),sizeof(int));
   binstr_cat_string(SETL_SYSTEM (char *)&(source_root->m_ntype.m_root.m_cardinality),
                  sizeof(int32));

   source_root = spec->sp_val.sp_map_ptr;
   source_work_hdr = source_root;
   source_height = source_root->m_ntype.m_root.m_height;
   source_cell = NULL;
   source_index = 0;
   valset_root = NULL;

   /* loop over the elements of source */

   for (;;) {

      /* find the next cell in the map */

      while (source_cell == NULL) {

         /* start on the next clash list, if we're at a leaf */

         if (!source_height && source_index < MAP_HASH_SIZE) {

            source_cell = source_work_hdr->m_child[source_index].m_cell;
            source_index++;
            continue;

         }


         /* move up if we're at the end of a node */

         if (source_index >= MAP_HASH_SIZE) {

            /* there are no more elements, so break */

            if (source_work_hdr == source_root)
               break;

            /* otherwise move up */

            source_height++;
            source_index =
               source_work_hdr->m_ntype.m_intern.m_child_index + 1;
            source_work_hdr =
               source_work_hdr->m_ntype.m_intern.m_parent;
            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->m_child[source_index].m_header == NULL) {

            source_index++;
            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->m_child[source_index].m_header;
         source_index = 0;
         source_height--;

      }

      /* if there are no more cells, break */

      if (source_cell == NULL)
         break;

      /* if the cell is not multi-value, use the pair directly */

      if (!(source_cell->m_is_multi_val)) {

         domain_element = &(source_cell->m_domain_spec);
         range_element = &(source_cell->m_range_spec);
         source_cell = source_cell->m_next;

      }

      /* otherwise find the next element in the value set */

      else {

         domain_element = &(source_cell->m_domain_spec);
         if (valset_root == NULL) {

            valset_root = source_cell->m_range_spec.sp_val.sp_set_ptr;
            valset_work_hdr = valset_root;
            valset_height = valset_root->s_ntype.s_root.s_height;
            valset_cell = NULL;
            valset_index = 0;

         }

         range_element = NULL;
         for (;;) {

            /* if we have an element already, break */

            if (valset_cell != NULL) {

               range_element = &(valset_cell->s_spec);
               valset_cell = valset_cell->s_next;
               break;

            }

            /* start on the next clash list, if we're at a leaf */

            if (!valset_height && valset_index < SET_HASH_SIZE) {

               valset_cell = valset_work_hdr->s_child[valset_index].s_cell;
               valset_index++;
               continue;

            }

            /* the current header node is exhausted -- find the next one */

            if (valset_index >= SET_HASH_SIZE) {

               /* there are no more elements, so break */

               if (valset_work_hdr == valset_root)
                  break;

               /* otherwise move up */

               valset_height++;
               valset_index =
                  valset_work_hdr->s_ntype.s_intern.s_child_index + 1;
               valset_work_hdr =
                  valset_work_hdr->s_ntype.s_intern.s_parent;
               continue;

            }

            /* skip over null nodes */

            if (valset_work_hdr->s_child[valset_index].s_header == NULL) {

               valset_index++;
               continue;

            }

            /* otherwise drop down a level */

            valset_work_hdr =
               valset_work_hdr->s_child[valset_index].s_header;
            valset_index = 0;
            valset_height--;

         }

         if (range_element == NULL) {

            source_cell = source_cell->m_next;
            valset_root = NULL;
            continue;

         }
      }

      /*
       *  At this point we have a pair from the map which we would like to
       *  save.
       */

      form_code = ft_tuple;
      binstr_cat_string(SETL_SYSTEM (char *)&(form_code),sizeof(int));
      map_cardinality = 2;
      binstr_cat_string(SETL_SYSTEM (char *)&map_cardinality,sizeof(int32));

      binstr_cat_spec(SETL_SYSTEM domain_element);
      binstr_cat_spec(SETL_SYSTEM range_element);

   }

   return;

}

/*\
 *  \case{tuples}
 *
 *  We loop over the elements of a tuple saving each.
\*/

case ft_tuple :

{
int form_code;                         /* specifier form code               */
int32 saved_number;                    /* number of elements saved          */
tuple_h_ptr_type source_root, source_work_hdr;
                                       /* root and internal node pointers   */
tuple_c_ptr_type source_cell;          /* current cell pointer              */
int32 source_number;                   /* current cell number               */
int source_height, source_index;       /* current height and index          */
specifier *source_element;             /* set element                       */

   source_root = spec->sp_val.sp_tuple_ptr;

   /* save the form and cardinality of tuple */

   binstr_cat_string(SETL_SYSTEM (char *)&(spec->sp_form),sizeof(int));
   binstr_cat_string(SETL_SYSTEM (char *)&(source_root->t_ntype.t_root.t_length),
                  sizeof(int32));

   source_root = spec->sp_val.sp_tuple_ptr;
   source_work_hdr = source_root;
   saved_number = source_number = -1;
   source_height = source_root->t_ntype.t_root.t_height;
   source_index = 0;

   /* loop over the elements of source */

   while (source_number < source_root->t_ntype.t_root.t_length) {

      /* find the next element in the tuple */

      source_element = NULL;
      for (;;) {

         /* if we have an element already, return it */

         if (!source_height && source_index < TUP_HEADER_SIZE) {

            if (source_work_hdr->t_child[source_index].t_cell == NULL) {

               source_number++;
               source_index++;

               continue;

            }

            source_cell = source_work_hdr->t_child[source_index].t_cell;
            source_element = &(source_cell->t_spec);

            source_number++;
            source_index++;

            break;

         }

         /* move up if we're at the end of a node */

         if (source_index >= TUP_HEADER_SIZE) {

            /* break if we've exhausted the source */

            if (source_work_hdr == source_root)
               break;

            source_height++;
            source_index =
               source_work_hdr->t_ntype.t_intern.t_child_index + 1;
            source_work_hdr =
               source_work_hdr->t_ntype.t_intern.t_parent;

            continue;

         }

         /* skip over null nodes */

         if (source_work_hdr->t_child[source_index].t_header == NULL) {

            source_number += 1L << (source_height * TUP_SHIFT_DIST);
            source_index++;

            continue;

         }

         /* otherwise drop down a level */

         source_work_hdr =
            source_work_hdr->t_child[source_index].t_header;
         source_index = 0;
         source_height--;

      }

      if (source_element == NULL)
         break;

      /*
       *  At this point we have an element in source_element which must
       *  be saved.  We might have to advance the current pointer first
       *  though.
       */

      if (++saved_number < source_number) {

         form_code = SKIP_CODE;
         binstr_cat_string(SETL_SYSTEM (char *)&(form_code),sizeof(int));

         binstr_cat_string(SETL_SYSTEM (char *)&source_number,sizeof(void *));

         saved_number = source_number;

      }

      binstr_cat_spec(SETL_SYSTEM source_element);

   }

   return;

}

/*\
 *  \case{objects}
 *
 *  Objects are a little nasty.  We have to save the code, the object
 *  name, and the time stamp before the various data elements.
\*/

case ft_object :

{
unittab_ptr_type class_ptr;            /* class pointer                     */
object_h_ptr_type object_root, object_work_hdr;
                                       /* object header pointers            */
object_c_ptr_type object_cell;         /* object cell pointer               */
struct slot_info_item *slot_info;
                                       /* slot information record           */
int target_index, target_height;
                                       /* used to descend header trees      */
int32 target_number;                   /* slot number                       */
int name_length;                       /* length of object name             */

   /* pick out the object root and class */

   object_root = spec->sp_val.sp_object_ptr;
   class_ptr = object_root->o_ntype.o_root.o_class;

   /* store the code, the name length, and the name */

   binstr_cat_string(SETL_SYSTEM (char *)&(spec->sp_form),sizeof(int));
   name_length = strlen(class_ptr->ut_name);
   binstr_cat_string(SETL_SYSTEM (char *)&name_length,sizeof(int));
   binstr_cat_string(SETL_SYSTEM class_ptr->ut_name,(size_t)name_length);

   /* save each instance variable */

   object_work_hdr = object_root;
   target_height = class_ptr->ut_obj_height;

   /* loop over the instance variables */

   for (slot_info = class_ptr->ut_first_var, target_number = 0;
        slot_info != NULL;
        slot_info = slot_info->si_next_var, target_number++) {

      /* drop down to a leaf */

      while (target_height) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * OBJ_SHIFT_DIST)) &
                           OBJ_SHIFT_MASK;

         /* we'll always have all internal nodes in this situation */

         object_work_hdr =
            object_work_hdr->o_child[target_index].o_header;
         target_height--;

      }

      /*
       *  At this point, object_work_hdr points to the lowest level
       *  header record.  Concatenate the instance variable.
       */

      target_index = target_number & OBJ_SHIFT_MASK;
      object_cell = object_work_hdr->o_child[target_index].o_cell;
      binstr_cat_spec(SETL_SYSTEM &(object_cell->o_spec));

      /*
       *  We move back up the header tree at this point, if it is
       *  necessary.
       */

      target_index++;
      while (target_index >= OBJ_HEADER_SIZE) {

         target_height++;
         target_index =
            object_work_hdr->o_ntype.o_intern.o_child_index + 1;
         object_work_hdr = object_work_hdr->o_ntype.o_intern.o_parent;

      }
   }

   return;

}

/* back to normal indentation */

   }

   return;

}

/*\
 *  \function{binstr\_cat\_string()}
 *
 *  This function concatenates a C character string on the \verb"binstr"
 *  return value.
\*/
      
static void binstr_cat_string(
   SETL_SYSTEM_PROTO
   char *charstring,                   /* C string to be concatenated       */
   size_t length)                      /* length of string                  */

{
char *p;                               /* temporary looping variable        */

   p = charstring;
   while (length--) {

      if (binstr_char_ptr == binstr_char_end) {

         get_string_cell(binstr_curr_cell);
         if (binstr_curr_hdr->s_tail != NULL)
            (binstr_curr_hdr->s_tail)->s_next = binstr_curr_cell;
         binstr_curr_cell->s_prev = binstr_curr_hdr->s_tail;
         binstr_curr_cell->s_next = NULL;
         binstr_curr_hdr->s_tail = binstr_curr_cell;
         if (binstr_curr_hdr->s_head == NULL)
            binstr_curr_hdr->s_head = binstr_curr_cell;
         binstr_char_ptr = binstr_curr_cell->s_cell_value;
         binstr_char_end = binstr_char_ptr + STR_CELL_WIDTH;

      }

      *binstr_char_ptr++ = *p++;
      binstr_curr_hdr->s_length++;

   }
}

/*\
 *  \function{setl2\_unbinstr()}
 *
 *  This is the functional form of \verb"reads".  Why in the world did I
 *  make it a procedure!?  I'd like to get rid of \verb"reads".
\*/

void setl2_unbinstr(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (none here)       */
   specifier *target)                  /* return value                      */

{

   /* make sure the argument is a string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"unbinstr",
            abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from string node */

   file_ptr = reads_ptr;
   file_buffer = file_ptr->f_file_buffer;
   lookahead = start = endofbuffer = file_buffer;

   /* load input string */

   reads_length = argv[0].sp_val.sp_string_ptr->s_length;
   reads_cell = argv[0].sp_val.sp_string_ptr->s_head;
   if (reads_cell == NULL) {
      reads_char_ptr = reads_char_end = NULL;
   }
   else {
      reads_char_ptr = reads_cell->s_cell_value;
      reads_char_end = reads_char_ptr + STR_CELL_WIDTH;
   }

   /* read the value */

   unbinstr_spec(SETL_SYSTEM target);

   return;

}

/*\
 *  \function{unbinstr\_spec()}
 *
 *  This function gets one specifier from a binary file.  It is called
 *  recursively for sets and tuples.
\*/

/* read string macro */

#define unbinstr_get_string(s,l) {\
   for (unbinstr_ptr = s; unbinstr_ptr < (s) + (l); unbinstr_ptr++) {\
      advance_la; *unbinstr_ptr = *lookahead; start = lookahead; }\
}
      
static void unbinstr_spec(
   SETL_SYSTEM_PROTO
   specifier *spec)                    /* specifier to be read              */

{
int form_code;                         /* form code of next specifier       */
char *unbinstr_ptr;                    /* work char pointer                 */

   /* every value starts with a form code */

   unbinstr_get_string((char *)&form_code,sizeof(int));

   switch (form_code) {


/*\
 *  \case{omegas}
 *
 *  The form code is enough to identify an omega.
\*/

case ft_omega :

{

   unmark_specifier(spec);
   spec->sp_form = ft_omega;

   return;

}

/*\
 *  \case{internal types}
 *
 *  We have a variety of types which we do not permit to be read in
 *  unless they are written in the same execution.  For these is it
 *  sufficient to write the specifier.
\*/

case ft_atom :          case ft_opaque :        case ft_label :
case ft_file :          case ft_proc :          case ft_iter :
case ft_mailbox :

{
time_t storetime;                      /* time specifier was stored         */

   unmark_specifier(spec);
   spec->sp_form = form_code;

   unbinstr_get_string((char *)&(spec->sp_val.sp_biggest),sizeof(void *));
   unbinstr_get_string((char *)&storetime,sizeof(time_t));

   if (storetime != runtime &&
       (spec->sp_form != ft_atom ||
        (spec->sp_val.sp_atom_num != spec_true->sp_val.sp_atom_num &&
         spec->sp_val.sp_atom_num != spec_false->sp_val.sp_atom_num)))
      abend(SETL_SYSTEM "Internal values are not preserved across program executions");

   return;

}

/*\
 *  \case{integers}
 *
 *  We have two kinds of integers: short and long.  Short integers
 *  we can normally handle quite easily, but longs are more work.
\*/

case ft_short :         case SKIP_CODE :

{

   unmark_specifier(spec);
   spec->sp_form = form_code;

   unbinstr_get_string((char *)&(spec->sp_val.sp_biggest),sizeof(void *));

   return;

}

case ft_long :

{
integer_h_ptr_type integer_hdr;        /* root of long structure            */
integer_c_ptr_type integer_cell;       /* long cell                         */
int32 cell_count;                      /* number of cells in integer        */

   /* get the number of cells */

   unbinstr_get_string((char *)&cell_count,sizeof(int32));

   /* create a new integer pointer */

   get_integer_header(integer_hdr);
   integer_hdr->i_use_count = 1;
   integer_hdr->i_hash_code = -1;
   integer_hdr->i_is_negative = (cell_count < 0);
   cell_count = labs(cell_count);
   integer_hdr->i_cell_count = cell_count;
   integer_hdr->i_head = integer_hdr->i_tail = NULL;

   while (cell_count--) {

      get_integer_cell(integer_cell);
      if (integer_hdr->i_tail != NULL)
         (integer_hdr->i_tail)->i_next = integer_cell;
      integer_cell->i_prev = integer_hdr->i_tail;
      integer_hdr->i_tail = integer_cell;
      if (integer_hdr->i_head == NULL)
         integer_hdr->i_head = integer_cell;
      integer_cell->i_next = NULL;

      unbinstr_get_string((char *)&(integer_cell->i_cell_value),
                          sizeof(int32));

   }

   /* set the result and return */

   unmark_specifier(spec);
   spec->sp_form = form_code;
   spec->sp_val.sp_long_ptr = integer_hdr;

   return;

}

/*\
 *  \case{real numbers}
 *
 *  We just read the form code and value for reals.
\*/

case ft_real :

{

   unmark_specifier(spec);
   spec->sp_form = form_code;

   i_get_real(spec->sp_val.sp_real_ptr);
   (spec->sp_val.sp_real_ptr)->r_use_count = 1;
   unbinstr_get_string((char *)&((spec->sp_val.sp_real_ptr)->r_value),
                       sizeof(double));

   return;

}

/*\
 *  \case{strings}
 *
 *  Strings are complex structures, because we allow infinite length.  We
 *  have to load each cell individually.
\*/

case ft_string :

{
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
int32 string_length;                   /* characters left in string         */
int cell_length;                       /* characters in cell to be printed  */

   /* get the string length */

   unbinstr_get_string((char *)&string_length,sizeof(int32));

   /* initialize a string structure */

   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = string_length;
   string_hdr->s_head = string_hdr->s_tail = string_cell = NULL;

   /* read each cell */

   while (string_length) {

      get_string_cell(string_cell);
      if (string_hdr->s_tail != NULL)
         (string_hdr->s_tail)->s_next = string_cell;
      string_cell->s_prev = string_hdr->s_tail;
      string_cell->s_next = NULL;
      string_hdr->s_tail = string_cell;
      if (string_hdr->s_head == NULL)
         string_hdr->s_head = string_cell;

      cell_length = min(STR_CELL_WIDTH,string_length);
      unbinstr_get_string((char *)string_cell->s_cell_value,
                          (size_t)cell_length);

      string_length -= cell_length;

   }

   /* set the target and return */

   unmark_specifier(spec);
   spec->sp_form = form_code;
   spec->sp_val.sp_string_ptr = string_hdr;

   return;

}

/*\
 *  \case{sets}
 *
 *  We build sets by calling this function recursively for each element,
 *  and inserting each in the set.
\*/

case ft_set :

{
int32 set_cardinality;                 /* length of set                     */
set_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
set_c_ptr_type target_cell;            /* current cell pointer              */
set_c_ptr_type *target_tail;           /* attach new cells here             */
int target_height, target_index;       /* current height and index          */
specifier target_element;              /* set element                       */
int32 target_hash_code;                /* hash code of target element       */
set_h_ptr_type new_hdr;                /* created header node               */
set_c_ptr_type new_cell;               /* created cell node                 */
int32 work_hash_code;                  /* working hash code (destroyed)     */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int is_equal;                          /* YES if two specifiers are equal   */
int i;                                 /* temporary looping variable        */

   /* get the set cardinality */

   unbinstr_get_string((char *)&set_cardinality,sizeof(int32));

   /* create a new set for the target */

   get_set_header(target_root);
   target_root->s_use_count = 1;
   target_root->s_hash_code = 0;
   target_root->s_ntype.s_root.s_cardinality = 0;
   target_root->s_ntype.s_root.s_height = 0;
   for (i = 0;
        i < SET_HASH_SIZE;
        target_root->s_child[i++].s_cell = NULL);
   expansion_trigger = SET_HASH_SIZE * SET_CLASH_SIZE;

   /* insert elements until we find a right brace */

   while (set_cardinality--) {

      /* get the next spec from the input stream */

      target_element.sp_form = ft_omega;
      unbinstr_spec(SETL_SYSTEM &target_element);

      /*
       *  At this point we have an element we would like to insert into
       *  the target.
       */

      target_work_hdr = target_root;

      /* get the element's hash code */

      spec_hash_code(work_hash_code,&target_element);
      target_hash_code = work_hash_code;

      /* descend the header tree until we get to a leaf */

      target_height = target_root->s_ntype.s_root.s_height;
      while (target_height--) {

         /* extract the element's index at this level */

         target_index = work_hash_code & SET_HASH_MASK;
         work_hash_code = work_hash_code >> SET_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (target_work_hdr->s_child[target_index].s_header == NULL) {

            get_set_header(new_hdr);
            new_hdr->s_ntype.s_intern.s_parent = target_work_hdr;
            new_hdr->s_ntype.s_intern.s_child_index = target_index;
            for (i = 0;
                 i < SET_HASH_SIZE;
                 new_hdr->s_child[i++].s_cell = NULL);
            target_work_hdr->s_child[target_index].s_header = new_hdr;
            target_work_hdr = new_hdr;

         }
         else {

            target_work_hdr =
               target_work_hdr->s_child[target_index].s_header;

         }
      }

      /*
       *  At this point, target_work_hdr points to the lowest level header
       *  record.  The next problem is to determine if the element is
       *  already in the set.  We compare the element with the clash
       *  list.
       */

      target_index = work_hash_code & SET_HASH_MASK;
      target_tail = &(target_work_hdr->s_child[target_index].s_cell);
      for (target_cell = *target_tail;
           target_cell != NULL &&
              target_cell->s_hash_code < target_hash_code;
           target_cell = target_cell->s_next) {

         target_tail = &(target_cell->s_next);

      }

      /* check for a duplicate element */

      is_equal = NO;
      while (target_cell != NULL &&
             target_cell->s_hash_code == target_hash_code) {

         spec_equal(is_equal,&(target_cell->s_spec),&target_element);

         if (is_equal)
            break;

         target_tail = &(target_cell->s_next);
         target_cell = target_cell->s_next;

      }

      /* if we have a duplicate, unmark it and get the next one */

      if (is_equal) {

         unmark_specifier(&target_element);
         continue;

      }

      /* if we reach this point we didn't find the element, so we insert it */

      get_set_cell(new_cell);
      new_cell->s_spec.sp_form = target_element.sp_form;
      new_cell->s_spec.sp_val.sp_biggest =
         target_element.sp_val.sp_biggest;
      new_cell->s_hash_code = target_hash_code;
      new_cell->s_next = *target_tail;
      *target_tail = new_cell;
      target_root->s_ntype.s_root.s_cardinality++;
      target_root->s_hash_code ^= target_hash_code;

      /* expand the set header if necessary */

      if (target_root->s_ntype.s_root.s_cardinality > expansion_trigger) {

         target_root = set_expand_header(SETL_SYSTEM target_root);
         expansion_trigger *= SET_HASH_SIZE;

      }
   }

   /* set the target and return */

   unmark_specifier(spec);
   spec->sp_form = form_code;
   spec->sp_val.sp_set_ptr = target_root;

   return;

}

/*\
 *  \case{tuples}
 *
 *  We build tuples by calling this function recursively for each element,
 *  and inserting each in the tuple.
\*/

case ft_tuple :

{
int32 tuple_length;                    /* length of tuple                   */
tuple_h_ptr_type target_root, target_work_hdr;
                                       /* root and internal node pointers   */
int target_height, target_index;       /* current height and index          */
specifier target_element;              /* tuple element                     */
int32 target_number;                   /* target element number             */
tuple_h_ptr_type new_hdr;              /* created header node               */
tuple_c_ptr_type new_cell;             /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int i;                                 /* temporary looping variable        */

   /* get the tuple_length */

   unbinstr_get_string((char *)&tuple_length,sizeof(int32));

   /* create a new tuple for the target */

   get_tuple_header(target_root);
   target_root->t_use_count = 1;
   target_root->t_hash_code = 0;
   target_root->t_ntype.t_root.t_length = 0;
   target_root->t_ntype.t_root.t_height = 0;
   for (i = 0;
        i < TUP_HEADER_SIZE;
        target_root->t_child[i++].t_cell = NULL);
   expansion_trigger = TUP_HEADER_SIZE;

   /* insert elements until we find a right brace */

   if (tuple_length > 0) {

      for (;;) {

         target_element.sp_form = ft_omega;
         unbinstr_spec(SETL_SYSTEM &target_element);

         if (target_element.sp_form == SKIP_CODE) {

            target_root->t_ntype.t_root.t_length =
               target_element.sp_val.sp_short_value;

            continue;

         }

         /*
          *  At this point we have an element we would like to insert into
          *  the target.
          */

         target_number = target_root->t_ntype.t_root.t_length++;

         /* expand the target header if necessary */

         while (target_root->t_ntype.t_root.t_length >= expansion_trigger) {

            target_work_hdr = target_root;

            get_tuple_header(target_root);

            target_root->t_use_count = 1;
            target_root->t_hash_code =
                  target_work_hdr->t_hash_code;
            target_root->t_ntype.t_root.t_length =
                  target_work_hdr->t_ntype.t_root.t_length;
            target_root->t_ntype.t_root.t_height =
                  target_work_hdr->t_ntype.t_root.t_height + 1;

            for (i = 1;
                 i < TUP_HEADER_SIZE;
                 target_root->t_child[i++].t_header = NULL);

            target_root->t_child[0].t_header = target_work_hdr;

            target_work_hdr->t_ntype.t_intern.t_parent = target_root;
            target_work_hdr->t_ntype.t_intern.t_child_index = 0;

            expansion_trigger *= TUP_HEADER_SIZE;

         }

         /* descend the header tree until we get to a leaf */

         target_work_hdr = target_root;
         for (target_height = target_root->t_ntype.t_root.t_height;
              target_height;
              target_height--) {

            /* extract the element's index at this level */

            target_index = (target_number >>
                                 (target_height * TUP_SHIFT_DIST)) &
                              TUP_SHIFT_MASK;

            /* if we're missing a header record, allocate one */

            if (target_work_hdr->t_child[target_index].t_header == NULL) {

               get_tuple_header(new_hdr);
               new_hdr->t_ntype.t_intern.t_parent = target_work_hdr;
               new_hdr->t_ntype.t_intern.t_child_index = target_index;

               for (i = 0;
                    i < TUP_HEADER_SIZE;
                    new_hdr->t_child[i++].t_cell = NULL);

               target_work_hdr->t_child[target_index].t_header = new_hdr;
               target_work_hdr = new_hdr;

            }
            else {

               target_work_hdr =
                  target_work_hdr->t_child[target_index].t_header;

            }
         }

         /*
          *  At this point, target_work_hdr points to the lowest level header
          *  record.  We insert the new element in the appropriate slot.
          */

         get_tuple_cell(new_cell);
         new_cell->t_spec.sp_form = target_element.sp_form;
         new_cell->t_spec.sp_val.sp_biggest =
            target_element.sp_val.sp_biggest;
         spec_hash_code(new_cell->t_hash_code,&target_element);
         target_index = target_number & TUP_SHIFT_MASK;
         target_work_hdr->t_child[target_index].t_cell = new_cell;
         target_root->t_hash_code ^= new_cell->t_hash_code;

         /* break when we've loaded the entire tuple */

         if (target_number == tuple_length - 1)
            break;

      }
   }

   /* set the target and return */

   unmark_specifier(spec);
   spec->sp_form = form_code;
   spec->sp_val.sp_tuple_ptr = target_root;

   return;

}

/*\
 *  \case{objects}
 *
 *  Objects are a little nasty.  We have to make sure the class is loaded
 *  before we read it.
\*/

case ft_object :

{
unittab_ptr_type class_ptr;            /* class pointer                     */
object_h_ptr_type object_root, object_work_hdr, new_object_hdr;
                                       /* object header pointers            */
object_c_ptr_type object_cell;         /* object cell pointer               */
struct slot_info_item *slot_info;      /* slot information record           */
int target_index, target_height;
                                       /* used to descend header trees      */
int32 target_number;                   /* slot number                       */
char name_buffer[MAX_UNIT_NAME + 1];   /* object name                       */
int name_length;                       /* length of object name             */
int i;                                 /* temporary looping variable        */

   /* get the object's name and length */

   unbinstr_get_string((char *)&name_length,sizeof(int));
   unbinstr_get_string(name_buffer,(size_t)name_length);
   name_buffer[name_length] = '\0';

   /* make sure the object is loaded */

   class_ptr = load_unit(SETL_SYSTEM name_buffer,NULL,NULL);

   /* initialize the object header */

   get_object_header(object_root);
   object_root->o_ntype.o_root.o_class = class_ptr;
   object_root->o_use_count = 1;
   object_root->o_hash_code = (int32)class_ptr;
   object_root->o_process_ptr = NULL;

   for (i = 0;
        i < OBJ_HEADER_SIZE;
        object_root->o_child[i++].o_cell = NULL);

   object_work_hdr = object_root;
   target_height = class_ptr->ut_obj_height;

   /* loop over the instance variables */

   for (slot_info = class_ptr->ut_first_var, target_number = 0;
        slot_info != NULL;
        slot_info = slot_info->si_next_var, target_number++) {

      /* drop down to a leaf */

      while (target_height) {

         /* extract the element's index at this level */

         target_index = (target_number >>
                              (target_height * OBJ_SHIFT_DIST)) &
                           OBJ_SHIFT_MASK;

         /* if the header is missing, allocate one */

         if (object_work_hdr->o_child[target_index].o_header == NULL) {

            get_object_header(new_object_hdr);
            new_object_hdr->o_ntype.o_intern.o_parent = object_work_hdr;
            new_object_hdr->o_ntype.o_intern.o_child_index = target_index;

            for (i = 0;
                 i < OBJ_HEADER_SIZE;
                 new_object_hdr->o_child[i++].o_cell = NULL);

            object_work_hdr->o_child[target_index].o_header =
                  new_object_hdr;
            object_work_hdr = new_object_hdr;

         }

         /* otherwise just drop down a level */

         else {

            object_work_hdr =
               object_work_hdr->o_child[target_index].o_header;

         }

         target_height--;

      }

      /*
       *  At this point, object_work_hdr points to the lowest level
       *  header record.  We insert the new element in the appropriate
       *  slot.
       */

      target_index = target_number & OBJ_SHIFT_MASK;
      get_object_cell(object_cell);
      object_work_hdr->o_child[target_index].o_cell = object_cell;
      object_cell->o_spec.sp_form = ft_omega;
      unbinstr_spec(SETL_SYSTEM &(object_cell->o_spec));
      spec_hash_code(object_cell->o_hash_code,&(object_cell->o_spec));
      object_root->o_hash_code ^= object_cell->o_hash_code;

      /*
       *  We move back up the header tree at this point, if it is
       *  necessary.
       */

      target_index++;
      while (target_index >= OBJ_HEADER_SIZE) {

         target_height++;
         target_index =
            object_work_hdr->o_ntype.o_intern.o_child_index + 1;
         object_work_hdr = object_work_hdr->o_ntype.o_intern.o_parent;

      }
   }

   /* set the target and return */

   unmark_specifier(spec);
   spec->sp_form = ft_object;
   spec->sp_val.sp_object_ptr = object_root;

   return;

}

/* back to normal indentation */

   }

   return;

}

/*\
 *  \function{setl2\_popen()}
 *
 *  This function is a little like the Unix popen() function, except that
 *  it returns two file descriptors, one for stdin and one for stdout.
 *
 *  NOTE:  I should be using one of the exec?? functions, but I used
 *  system instead.  I was too lazy to parse the string.
\*/


void setl2_popen(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (two here)        */
   specifier *target)                  /* return value                      */

{
#if UNIX
string_h_ptr_type string_hdr;          /* string root                       */
string_c_ptr_type string_cell;         /* string cell pointer               */
char *command;                         /* system command                    */
int to_parent[2];                      /* file descriptors to parent        */
int to_child[2];                       /* file descriptors to child         */
FILE *pipe_handle[2];                  /* file pointer versions             */
tuple_h_ptr_type target_root;          /* root of returned tuple            */
tuple_c_ptr_type new_tuple_cell;       /* created cell node                 */
specifier file_atom;                   /* file handle atom                  */
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
map_c_ptr_type *map_tail;              /* attach new cells here             */
int map_height, map_index;             /* current height and index          */
map_h_ptr_type new_hdr;                /* created header node               */
map_c_ptr_type new_map_cell;           /* created cell node                 */
int32 expansion_trigger;               /* cardinality which triggers        */
                                       /* header expansion                  */
int32 work_hash_code;                  /* working hash code (destroyed)     */
char *s, *t;                           /* temporary looping variables       */
int i,j;                               /* temporary looping variables       */

   /* convert the command to a C character string */

   if (argv[0].sp_form != ft_string)
      abend(SETL_SYSTEM msg_bad_arg,"string",1,"popen",
            abend_opnd_str(SETL_SYSTEM argv));

   string_hdr = argv[0].sp_val.sp_string_ptr;

   command = (char *)malloc(string_hdr->s_length + 1);
   if (command == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   t = command;
   for (string_cell = string_hdr->s_head;
        string_cell != NULL;
        string_cell = string_cell->s_next) {

      for (s = string_cell->s_cell_value;
           t < command + string_hdr->s_length &&
              s < string_cell->s_cell_value + STR_CELL_WIDTH;
           *t++ = *s++);

   }
   *t = '\0';

   /* create a new tuple for the target */

   get_tuple_header(target_root);
   target_root->t_use_count = 1;
   target_root->t_hash_code = 0;
   target_root->t_ntype.t_root.t_length = 2;
   target_root->t_ntype.t_root.t_height = 0;
   for (i = 2;
        i < TUP_HEADER_SIZE;
        target_root->t_child[i++].t_cell = NULL);

   /* create the pipes */

   pipe(to_parent);
   pipe(to_child);
   pipe_handle[0] = fdopen(to_child[1],"w");
   pipe_handle[1] = fdopen(to_parent[0],"r");
      
   /*
    *     We add both of the file handles into the file map and the
    *     tuple.
    */

   for (j = 0; j < 2; j++) {

      /* make a file table entry */

      get_file(file_ptr);
      strcpy(file_ptr->f_file_name,command);
      file_ptr->f_file_stream = pipe_handle[j];

      if (j == 0) {
         file_ptr->f_mode = TEXT_OUT;
      }
      else {
         file_ptr->f_mode = BYTE_IN;
      }

      /* now enter the file in the file map */

      file_atom.sp_form = ft_omega;
      setl2_newat(SETL_SYSTEM 0,NULL,&file_atom);

      map_work_hdr = file_map;
      work_hash_code = file_atom.sp_val.sp_atom_num;
      for (map_height = file_map->m_ntype.m_root.m_height;
           map_height;
           map_height--) {

         /* extract the element's index at this level */

         map_index = work_hash_code & MAP_HASH_MASK;
         work_hash_code >>= MAP_SHIFT_DIST;

         /* if we're missing a header record, insert it */

         if (map_work_hdr->m_child[map_index].m_header == NULL) {

            get_map_header(new_hdr);
            new_hdr->m_ntype.m_intern.m_parent = map_work_hdr;
            new_hdr->m_ntype.m_intern.m_child_index = map_index;
            for (i = 0;
                 i < MAP_HASH_SIZE;
                 new_hdr->m_child[i++].m_cell = NULL);
            map_work_hdr->m_child[map_index].m_header = new_hdr;
            map_work_hdr = new_hdr;

         }
         else {

            map_work_hdr = map_work_hdr->m_child[map_index].m_header;

         }
      }

      /*
       *  At this point, map_work_hdr points to the lowest level
       *  header record.
       */

      map_index = work_hash_code & MAP_HASH_MASK;
      map_tail = &(map_work_hdr->m_child[map_index].m_cell);
      for (map_cell = *map_tail;
           map_cell != NULL &&
              map_cell->m_hash_code < file_atom.sp_val.sp_atom_num;
           map_cell = map_cell->m_next) {

         map_tail = &(map_cell->m_next);

      }

      /* we don't have to worry about duplicates -- add a cell */

      get_map_cell(new_map_cell);
      new_map_cell->m_domain_spec.sp_form = ft_atom;
      new_map_cell->m_domain_spec.sp_val.sp_atom_num =
           file_atom.sp_val.sp_atom_num;
      new_map_cell->m_range_spec.sp_form = ft_file;
      new_map_cell->m_range_spec.sp_val.sp_file_ptr = file_ptr;
      new_map_cell->m_is_multi_val = NO;
      new_map_cell->m_hash_code = file_atom.sp_val.sp_atom_num;
      new_map_cell->m_next = *map_tail;
      *map_tail = new_map_cell;
      file_map->m_ntype.m_root.m_cardinality++;
      file_map->m_ntype.m_root.m_cell_count++;
      file_map->m_hash_code ^= file_atom.sp_val.sp_atom_num;

      expansion_trigger =
            (1 << ((file_map->m_ntype.m_root.m_height + 1)
                       * MAP_SHIFT_DIST)) * MAP_CLASH_SIZE;

      /* expand the map header if necessary */

      if (file_map->m_ntype.m_root.m_cardinality > expansion_trigger) {

         file_map = map_expand_header(SETL_SYSTEM file_map);

      }

      /* finally, stick it in the tuple to be returned */

      get_tuple_cell(new_tuple_cell);
      new_tuple_cell->t_spec.sp_form = file_atom.sp_form;
      new_tuple_cell->t_spec.sp_val.sp_biggest =
         file_atom.sp_val.sp_biggest;
      spec_hash_code(new_tuple_cell->t_hash_code,&file_atom);
      target_root->t_child[j].t_cell = new_tuple_cell;
      target_root->t_hash_code ^= new_tuple_cell->t_hash_code;

   }

   /* we're ready to fork */

   if (fork() != 0) {

      /* this is the parent */

      close(to_parent[1]);
      close(to_child[0]);
      free(command);

      /* return the tuple we created */

      unmark_specifier(target);
      target->sp_form = ft_tuple;
      target->sp_val.sp_tuple_ptr = target_root;

      return;

   }

   /* this is the child.  We rearrange descriptors and execute */

   close(0);
   dup(to_child[0]);
   close(1);
   dup(to_parent[1]);
   close(to_parent[0]);
   close(to_parent[1]);
   close(to_child[0]);
   close(to_child[1]);

   system(command);
   exit(0);
#endif

}


/*\
 *  \function{setl2\_getchar()}
 *
 *  Getchar is designed to work with popen.  It returns one character
 *  from an input file.
\*/


void setl2_getchar(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
#ifdef UNIX
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */
string_h_ptr_type string_hdr;          /* created string header             */
string_c_ptr_type string_cell;         /* current string cell               */
char c;                                /* character from file               */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;
   if (file_ptr->f_mode != BYTE_IN)
      abend(SETL_SYSTEM msg_get_not_text,file_ptr->f_file_name);
   file_name = file_ptr->f_file_name;
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;

   /* read one character */

   if (fread((void *)(&c),
             sizeof(char),
             (size_t)1,
             file_ptr->f_file_stream) != (size_t)1) {

      /* if we didn't get one, check for errors */

      if (ferror(file_ptr->f_file_stream) != 0)
         giveup(SETL_SYSTEM "Disk error reading %s",file_ptr->f_file_name);

      /* no error means end of file */

      eof_flag = YES;
      unmark_specifier(target);
      target->sp_form = ft_omega;

      return;

   }

   /* set the return string */

   eof_flag = NO;
   get_string_header(string_hdr);
   string_hdr->s_use_count = 1;
   string_hdr->s_hash_code = -1;
   string_hdr->s_length = 1;
   get_string_cell(string_cell);
   string_cell->s_prev = NULL;
   string_cell->s_next = NULL;
   string_hdr->s_tail = string_cell;
   string_hdr->s_head = string_cell;
   string_cell->s_cell_value[0] = c;

   /* set the target */

   unmark_specifier(target);
   target->sp_form = ft_string;
   target->sp_val.sp_string_ptr = string_hdr;

   return;
#endif

}


/*\
 *  \function{setl2\_fflush()}
 *
\*/


void setl2_fflush(
   SETL_SYSTEM_PROTO
   int argc,                           /* number of arguments passed        */
   specifier *argv,                    /* argument vector (one here)        */
   specifier *target)                  /* return value                      */

{
#if UNIX
map_h_ptr_type map_work_hdr;           /* internal node pointer             */
map_c_ptr_type map_cell;               /* current cell pointer              */
int map_height, map_index;             /* current height and index          */
int32 work_hash_code;                  /* working hash code (destroyed)     */
string_h_ptr_type string_hdr;          /* created string header             */
string_c_ptr_type string_cell;         /* current string cell               */
char c;                                /* character from file               */

   /* file handles must be atoms */

   if (argv[0].sp_form != ft_atom)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* look up the map component */

   map_work_hdr = file_map;
   work_hash_code = argv[0].sp_val.sp_atom_num;

   for (map_height = map_work_hdr->m_ntype.m_root.m_height;
        map_height && map_work_hdr != NULL;
        map_height--) {

      /* extract the element's index at this level */

      map_index = work_hash_code & MAP_HASH_MASK;
      work_hash_code = work_hash_code >> MAP_SHIFT_DIST;

      map_work_hdr = map_work_hdr->m_child[map_index].m_header;

   }

   /* if we can't get to a leaf, there is no matching element */

   if (map_work_hdr == NULL)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /*
    *  At this point, map_work_hdr points to the lowest level header
    *  record.  We look for an element.
    */

   map_index = work_hash_code & MAP_HASH_MASK;
   for (map_cell = map_work_hdr->m_child[map_index].m_cell;
        map_cell != NULL &&
           map_cell->m_hash_code < argv[0].sp_val.sp_atom_num;
        map_cell = map_cell->m_next);

   if (map_cell->m_domain_spec.sp_val.sp_atom_num !=
       argv[0].sp_val.sp_atom_num)
      abend(SETL_SYSTEM msg_bad_file_handle,abend_opnd_str(SETL_SYSTEM argv));

   /* load file stuff from file node */

   file_ptr = map_cell->m_range_spec.sp_val.sp_file_ptr;
   file_stream = file_ptr->f_file_stream;
   file_fd = file_ptr->f_file_fd;

   fflush(file_stream);

   /* set the target */

   unmark_specifier(target);
   target->sp_form = ft_omega;

   return;
#endif

}

