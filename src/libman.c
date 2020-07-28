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
 *  \package{The Library Manager}
 *
 *  The library manager provides relatively low level access to SETL2
 *  libraries.  The implementation is somewhat weak, but adequate for
 *  now.  Before looking at those weaknesses and any implementation
 *  details, let's look at the library manager as a black box, and
 *  examine the functions it provides.
 *
 *  We think of a library as a collection of units which are keyed by
 *  name, where for our purposes a unit is a package or a program.  A
 *  unit in turn consists of a collection of streams, which are keyed by
 *  integers.
 *
 *  A unit may be opened either for reading or for writing.  Any streams
 *  within a unit are implicitly opened in the mode of their unit.  A
 *  given stream may be opened only once for output, but any number of
 *  times for input.  If a unit is opened for input, we may specify that
 *  the unit must be found in a particular library, or that all libraries
 *  controlled by the library manager must be searched.  Libraries may be
 *  entered in the library manager either by search path or unique file
 *  name.
 *
 *  We use streams to access different types of program data.  One stream
 *  will be reserved for code, another for public symbols, one for header
 *  information, and many other things.  The data in streams may only be
 *  accessed sequentially --- we randomly access only units and streams
 *  themselves.
 *
 *  Now let's look at the limitations of this implementation.  First,
 *  libraries use an open hashing scheme to allow indexing by unit name.
 *  The hash table size is fixed --- it does not grow with the size of
 *  the file.  We really don't expect this to be a problem.  We make the
 *  hash table fairly large, and don't worry about the space penalty,
 *  since the size of each unit is likely to be quite large by
 *  comparison.
 *
 *  We are somewhat wasteful of disk space for unit headers.  We only
 *  place one unit header per disk block, so the overhead is about one
 *  block per unit in the library.  Actually it's not that bad since we
 *  use most of a block for a header.
 *
 *  We always allocate an integral number of blocks for each stream, so
 *  on average we also waste one half block per stream.  Again, we didn't
 *  want the complexity associated with more efficient schemes.
 *
 *  To minimize the effects of the previous two weaknesses, we would like
 *  to keep blocks fairly small, and count on the operating system to do
 *  some buffering for us.  At the moment, a block is only 256 bytes.
 *
 *  Finally, we might leave some unreachable deleted records in a
 *  library, if through bad luck we are interrupted while updating it.
 *  We were very careful to update the file itself in such a way that the
 *  file will always be usable, we just might leave some garbage in it.
 *
 *  \newpage
 *  \texify{libman.h}
 *
 *  \newpage
 *  \texify{libcom.h}
 *
 *  \packagebody{Library Manager}
\*/

/* standard C header files */

#include "system.h"                    /* SETL2 system constants            */

#if UNIX
#include <sys/types.h>                 /* file types                        */
#include <sys/stat.h>                  /* file status                       */
#endif
#ifdef HAVE_SIGNAL
#include <signal.h>                    /* signal macros                     */
#endif

/* SETL2 system header files */

#include "interp.h"
#include "giveup.h"                    /* severe error handler              */
#include "messages.h"                  /* error messages                    */
#include "filename.h"                  /* file name match                   */
#include "libman.h"                    /* library manager                   */
#include "libcom.h"                    /* library manager -- common         */
#include "libfile.h"                   /* library file table                */
#include "libunit.h"                   /* library unit table                */
#include "libstr.h"                    /* library stream table              */
#include "libfree.h"                   /* library free list table           */

#ifdef MACINTOSH	/* Giuseppe 10/15/1999 */
#include "MacFileUtils.h"
#endif

#define MEM_LIB_INITIAL_SIZE 32768     /* Initial size of a memory library  */

long dl_size=0;
long dl_count=0;
char *mem_lib=NULL;

/*
 *  library header structure --- IMPORTANT!  The field lf_free_list MUST
 *  be the first field in this structure.  Occasionally we rewrite the
 *  free list pointer, without writing the rest of the header.  We do
 *  this by fseeking to the header and writing an unsigned.  If this
 *  field must be moved, those places in the program must be found and
 *  changed also.  At the time this comment is being written, that is
 *  just get_free_pos, but there may be others!
 */

typedef struct lib_header_item {
   int lh_free_list;                   /* first block in free list          */
   char lh_id[8];                      /* library identification            */
   int lh_hash_tab[LIB_HASH_SIZE];     /* hash table                        */
} lib_header;

/* data record format */

struct data_record {
   int ld_next;                        /* next record in list               */
   char ld_data[LIB_DATA_SIZE];        /* data varies widely                */
};

/* package-global data */

static libfile_ptr_type liblist_head = NULL;
                                       /* head of library file list         */
static libfile_ptr_type *liblist_tail; /* next library pointer              */
static struct data_record lib_data;    /* library data record               */
static int lib_files_open;             /* number of libraries open          */

/* forward declarations */

static void open_lib_file(SETL_SYSTEM_PROTO libfile_ptr_type);
                                       /* open a library file               */
void close_lib_file(SETL_SYSTEM_PROTO libfile_ptr_type);
                                       /* close a library file              */
#ifdef LIBWRITE
static unsigned get_free_pos(SETL_SYSTEM_PROTO libfile_ptr_type);
                                       /* find free library record          */
#endif
static void get_lib_rec(SETL_SYSTEM_PROTO libfile_ptr_type, unsigned, char *);
                                       /* read a library record             */
#ifdef LIBWRITE
static void put_lib_rec(SETL_SYSTEM_PROTO libfile_ptr_type, unsigned, char *);
                                       /* write a library record            */
#endif
static unsigned hashpjw(char *);       /* string hash function              */



void to_memcpy(
   long pos,                      
   void *source,
   long size) 

{
void *aux_ptr;
long newpos;
 
   newpos = pos+size;

   if (dl_count<newpos) dl_count = newpos;

   if (dl_count>dl_size) {

      /* Double the size of the mem_lib */

      aux_ptr=(void *)malloc(dl_size*2);

      /* Copy the old content           */

      memcpy((void *)(aux_ptr),mem_lib,dl_size);
      free(mem_lib);
      dl_size*=2;
      mem_lib=aux_ptr;
   }
   memcpy((void *)(mem_lib+pos),source,size);

}

/*\
 *  \function{open\_lib()}
 *
 *  This function opens the library manager.  There really isn't much we
 *  have to do here at the moment.  All we do is initialize a few
 *  variables visible globally in this file, but only in this file.
\*/

void open_lib()

{

   liblist_head = NULL;
   liblist_tail = &liblist_head;
   lib_files_open = 0;

}

/*\
 *  \function{close\_lib()}
 *
 *  This function closes the library manager.  Essentially, that involves
 *  closing any open library files.  We defer closing files until we
 *  close the library to avoid repeated opens and closes on library files
 *  as we search for many imported units during a compilation.
\*/

void close_lib(SETL_SYSTEM_PROTO_VOID)

{
libfile_ptr_type delete_ptr;           /* temporary file pointers           */

   /*
    *  loop over all the library files, closing any open ones and
    *  deallocating the library file nodes
    */

   while (liblist_head != NULL) {

      if (liblist_head->lf_is_open)
         close_lib_file(SETL_SYSTEM liblist_head);

      delete_ptr = liblist_head;
      liblist_head = liblist_head->lf_next;
      free_libfile(delete_ptr);

   }
}

/*\
 *  \function{create\_lib\_file()}
 *
 *  This function creates a brand new library, with an empty directory.
 *  It is used during compilations to create a temporary library, and by
 *  library utility programs.
\*/

#ifdef LIBWRITE

void create_lib_file( 
   SETL_SYSTEM_PROTO
   char *fname)                        /* name of new library file          */

{
lib_header header;                     /* file header record                */
FILE *lib_file;                        /* created file handle               */
int i;                                 /* temporary looping variable        */

   /* open a new library file */
   if (strcmp(fname,MEM_LIB_NAME)==0)  {

      dl_count=0;
      if (mem_lib==NULL) { /* Create initial array... */
          mem_lib=(char *)malloc(MEM_LIB_INITIAL_SIZE);
         if (mem_lib == NULL) giveup(SETL_SYSTEM msg_malloc_error);
      
         dl_size=MEM_LIB_INITIAL_SIZE; 
      }

      /* Now we have the area, create the initial library 
       * structure... 
       */

      header.lh_free_list = -1;
      strcpy(header.lh_id,LIB_ID);
      for (i = 0; i < LIB_HASH_SIZE; i++)
         header.lh_hash_tab[i] = -1;

      to_memcpy(0,
                (void *)(&header),
                sizeof(lib_header));

	return;

   }

   lib_file = fopen(fname,BINARY_WR);
   if (lib_file == NULL)
      giveup(SETL_SYSTEM msg_lib_create_error,fname);

   /* initialize the header record */

   header.lh_free_list = -1;
   strcpy(header.lh_id,LIB_ID);
   for (i = 0; i < LIB_HASH_SIZE; i++)
      header.lh_hash_tab[i] = -1;

   /* write the header record and close */

   if (fwrite((void *)(&header),
              sizeof(lib_header),
              (size_t)1,
              lib_file) != (size_t)1)
      giveup(SETL_SYSTEM msg_lib_write_error,fname);
   fclose(lib_file);

}

#endif

/*\
 *  \function{add\_lib\_file()}
 *
 *  This function adds a single file to the library file table.  It is
 *  usually called when we want to specifiy an output file, since in that
 *  situation we should specify a unique file rather than a list of
 *  matching files.  We still use the file list package, since we would
 *  like to know that we are passed a single file name, and we would like
 *  to fully qualify that file name so we can check for duplicates.
\*/

libfile_ptr_type add_lib_file(
   SETL_SYSTEM_PROTO
   char *fname,                        /* library file name                 */
   int is_writeable)                   /* YES if we can write to file       */

{
char work_fname[PATH_LENGTH + 1];      /* expanded file name                */
libfile_ptr_type libfile_ptr;          /* returned file pointer             */
int dynamic_library=NO;		       /* YES if we are adding the new      */
                                       /* library in memory                 */
lib_header header;                     /* file header record                */
int i;                                 /* temporary looping variable        */

   if (strcmp(fname,MEM_LIB_NAME)==0) 
      dynamic_library=YES;

   /*  first expand the file name and make sure the file exists */

   strcpy(work_fname,fname);
   expand_filename(SETL_SYSTEM work_fname);


   if (dynamic_library==NO) { 
      if (is_writeable) {
   
         if (os_access(work_fname,06) != 0)
            giveup(SETL_SYSTEM msg_bad_library,fname);

      }
      else {
   
         if (os_access(work_fname,04) != 0)
            giveup(SETL_SYSTEM msg_bad_library,fname);
   
      }

    }

   /*
    *  if we have already entered the library in the table, return a
    *  pointer to it
    */

   for (libfile_ptr = liblist_head;
        libfile_ptr != NULL && strcmp(libfile_ptr->lf_fname,work_fname) != 0;
        libfile_ptr = libfile_ptr->lf_next);
   if (libfile_ptr != NULL)
      return libfile_ptr;

   /* otherwise, append it to the table */

   libfile_ptr = get_libfile(SETL_SYSTEM_VOID);
   strcpy(libfile_ptr->lf_fname,work_fname);
   *liblist_tail = libfile_ptr;
   liblist_tail = &(libfile_ptr->lf_next);

   libfile_ptr->lf_is_writeable = is_writeable;
   
   if (dynamic_library==YES) 
      libfile_ptr->lf_mem_lib = YES;

   return libfile_ptr;

}

/*\
 *  \function{add\_lib\_path()}
 *
 *  This function adds all those files matching a passed specification
 *  list to the library file table.  Those files are only to be used as
 *  input files, and generally represent a list of files to be searched
 *  for imported units.  This is really simpler than it sounds, given
 *  that we have the file name matching package available.
\*/

void add_lib_path(
   SETL_SYSTEM_PROTO
   char *path)                         /* library file name                 */

{
libfile_ptr_type libfile_ptr;          /* returned file pointer             */
filelist_ptr_type fl_head;             /* list of matching files            */
filelist_ptr_type fl_ptr;              /* used to loop over file list       */

   /* expand the passed path specification */

   fl_head = setl_get_filelist(SETL_SYSTEM path);
   for (fl_ptr = fl_head; fl_ptr != NULL; fl_ptr = fl_ptr->fl_next) {

      /*
       *  if we have already entered the library in the table, don't
       *  install it again
       */

      for (libfile_ptr = liblist_head;
           libfile_ptr != NULL &&
              strcmp(libfile_ptr->lf_fname,fl_ptr->fl_name) != 0;
           libfile_ptr = libfile_ptr->lf_next);
      if (libfile_ptr != NULL)
         continue;

      /* otherwise, append it to the table */

      libfile_ptr = get_libfile(SETL_SYSTEM_VOID);
      strcpy(libfile_ptr->lf_fname,fl_ptr->fl_name);
      *liblist_tail = libfile_ptr;
      liblist_tail = &(libfile_ptr->lf_next);

      libfile_ptr->lf_is_writeable = NO;

   }

   setl_free_filelist(fl_head);

}

/*\
 *  \function{open\_libunit()}
 *
 *  This function opens a library unit.  We must be able to acommodate
 *  read and write modes, which are very different.  If we open for
 *  reading, then we search through the library list for matching unit,
 *  then read the unit header of that unit into the unit table.  If we
 *  open the unit for output, we just initialize the unit header, but do
 *  not write it yet.  We will install the unit header when we close the
 *  unit.
\*/

libunit_ptr_type open_libunit(
   SETL_SYSTEM_PROTO
   char *unit_name,                    /* name of unit to be opened         */
   libfile_ptr_type libfile_ptr,       /* file containing unit, or NULL     */
                                       /* if we are to search the libraries */
   int mode)                           /* unit open mode                    */

{
libunit_ptr_type libunit_ptr;          /* returned unit pointer             */
unit_header *header_ptr;               /* used to caste data record as a    */
                                       /* unit header                       */
unsigned hash_code;                    /* hash code for unit name           */
int unit_record;                       /* unit header record                */
#ifdef LIBWRITE
int i;                                 /* temporary looping variable        */
#endif

   if (mode == LIB_READ_UNIT) {

      /*
       *  if we are passed a NULL library file pointer in LIB_READ_UNIT mode,
       *  we search through all the libraries on our list for a matching
       *  unit
       */

      if (libfile_ptr == NULL) {

         for (libfile_ptr = liblist_head;
              libfile_ptr != NULL;
              libfile_ptr = libfile_ptr->lf_next) {

            /* make sure the library is open */

            if (!(libfile_ptr->lf_is_open))
               open_lib_file(SETL_SYSTEM libfile_ptr);

            /* look for the unit in the library */

            hash_code = hashpjw(unit_name);
            unit_record = (libfile_ptr->lf_header)->lh_hash_tab[hash_code];

            while (unit_record != -1) {

               get_lib_rec(SETL_SYSTEM libfile_ptr,unit_record,(char *)&lib_data);
               header_ptr = (unit_header *)lib_data.ld_data;
               if (strcmp(header_ptr->uh_name,unit_name) == 0)
                  break;
               unit_record = lib_data.ld_next;

            }

            /* if we found the unit, break */

            if (unit_record != -1)
               break;

         }

         /* if we didn't find the unit, return NULL */

         if (libfile_ptr == NULL)
            return NULL;

      }
      else {

         /*
          *  We are passed a library file pointer, so search just that
          *  file.  Make sure the library is open.
          */

         if (!(libfile_ptr->lf_is_open))
            open_lib_file(SETL_SYSTEM libfile_ptr);

         /* look for the unit in the library */

         hash_code = hashpjw(unit_name);
         unit_record = (libfile_ptr->lf_header)->lh_hash_tab[hash_code];

         while (unit_record != -1) {

            get_lib_rec(SETL_SYSTEM libfile_ptr,unit_record,(char *)&lib_data);
            header_ptr = (unit_header *)lib_data.ld_data;
            if (strcmp(header_ptr->uh_name,unit_name) == 0)
               break;
            unit_record = lib_data.ld_next;

         }

         /* if we didn't find the unit, return NULL */

         if (unit_record == -1)
            return NULL;

      }

      /*
       *  At this point, we have found the unit.  We have the header
       *  record number in unit_record, and libfile_ptr contains the
       *  library in which we found the unit.
       */

      libunit_ptr = get_libunit(SETL_SYSTEM_VOID);

      libunit_ptr->lu_libfile_ptr = libfile_ptr;
      libunit_ptr->lu_next = libfile_ptr->lf_libunit_list;
      libfile_ptr->lf_libunit_list = libunit_ptr;

      memcpy((void *)&(libunit_ptr->lu_header),
             (void *)lib_data.ld_data,
             sizeof(unit_header));

      libunit_ptr->lu_is_output = NO;

      return libunit_ptr;

   }

#ifdef LIBWRITE

   /*
    *  Open the unit for output.
    */

#ifdef TRAPS

   if (libfile_ptr == NULL)
      trap(__FILE__,__LINE__,
           msg_spec_lib);

   if (!(libfile_ptr->lf_is_writeable))
      trap(__FILE__,__LINE__,msg_read_only_library);

#endif

   /* make sure the library is open */

   if (!(libfile_ptr->lf_is_open))
      open_lib_file(SETL_SYSTEM libfile_ptr);

   /* initialize a new unit header */

   libunit_ptr = get_libunit(SETL_SYSTEM_VOID);
   strcpy(libunit_ptr->lu_header.uh_name,unit_name);

   for (i = 0; i < LIB_STREAM_COUNT; i++) {

      libunit_ptr->lu_header.uh_data_head[i] = -1;
      libunit_ptr->lu_header.uh_data_tail[i] = -1;
      libunit_ptr->lu_header.uh_data_length[i] = 0;

   }

   libunit_ptr->lu_libfile_ptr = libfile_ptr;
   libunit_ptr->lu_next = libfile_ptr->lf_libunit_list;
   libfile_ptr->lf_libunit_list = libunit_ptr;
   libunit_ptr->lu_is_output = YES;

   return libunit_ptr;

#else

   return NULL;    /* placate semantic error checker */

#endif

}

/*\
 *  \function{close\_libunit()}
 *
 *  This function closes a library unit.  Again, we must be able to
 *  acommodate read and write modes, which are very different.  If the
 *  unit is open for input, all we must do is close any open streams and
 *  return.  If the unit is open for output, we must also insert the unit
 *  into the library directory.
 *
 *  First we write the header for the unit we've just closed.  Then we
 *  copy the headers of any units which preceed that unit in the
 *  directory, since that chain must be changed to accommodate the unit
 *  we are closing.
\*/

void close_libunit(
   SETL_SYSTEM_PROTO
   libunit_ptr_type libunit_ptr)       /* unit to be closed                 */

{
libfile_ptr_type libfile_ptr;          /* library containing unit           */
libunit_ptr_type *temp_ptr;            /* used to remove unit pointer       */
#ifdef LIBWRITE
int unit_record;                       /* file position of old unit record  */
unit_header *header_ptr;               /* used to caste data record as a    */
                                       /* unit header                       */
unsigned hash_code;                    /* hash code for unit name           */
libfree_ptr_type libfree_ptr;          /* list of deleted unit headers      */
                                       /* from library unit list            */
int t1,t2,t3;                          /* temporary file pointers           */
int i;                                 /* temporary looping variable        */
#endif

   /* make sure all streams are closed */

   while (libunit_ptr->lu_libstr_list != NULL)
      close_libstr(SETL_SYSTEM libunit_ptr->lu_libstr_list);

   /* remove the unit pointer from the library file's list of open units */

   libfile_ptr = libunit_ptr->lu_libfile_ptr;

   for (temp_ptr = &(libfile_ptr->lf_libunit_list);
        *temp_ptr != NULL && *temp_ptr != libunit_ptr;
        temp_ptr = &((*temp_ptr)->lu_next));

#ifdef TRAPS

   if (*temp_ptr == NULL)
      trap(__FILE__,__LINE__,msg_bad_unit_close);

#endif

   *temp_ptr = (*temp_ptr)->lu_next;

   /* input units are easy -- just free the unit node and return */

   if (!(libunit_ptr->lu_is_output)) {

      free_libunit(libunit_ptr);
      return;

   }

#ifdef LIBWRITE

   /*
    *  Output units.
    */

   /* look for the unit in the library */

   hash_code = hashpjw(libunit_ptr->lu_header.uh_name);
   unit_record = (libfile_ptr->lf_header)->lh_hash_tab[hash_code];

   while (unit_record != -1) {

      get_lib_rec(SETL_SYSTEM libfile_ptr,unit_record,(char *)&lib_data);
      header_ptr = (unit_header *)lib_data.ld_data;
      if (strcmp(header_ptr->uh_name,libunit_ptr->lu_header.uh_name) == 0)
         break;
      unit_record = lib_data.ld_next;

   }

   /* if we didn't find it, just add it and return */

   if (unit_record == -1) {

      memcpy((void *)lib_data.ld_data,
             (void *)&(libunit_ptr->lu_header),
             sizeof(unit_header));
      lib_data.ld_next = (libfile_ptr->lf_header)->lh_hash_tab[hash_code];
      unit_record = get_free_pos(SETL_SYSTEM libfile_ptr);
      (libfile_ptr->lf_header)->lh_hash_tab[hash_code] = unit_record;
      put_lib_rec(SETL_SYSTEM libfile_ptr,unit_record,(char *)&lib_data);

      return;

   }

   /*
    *  We did find the unit name in the library.  First we free all the
    *  streams in the old unit.
    */

   t3 = lib_data.ld_next;
   for (i = 0; i < LIB_STREAM_COUNT; i++) {

      if (header_ptr->uh_data_head[i] != -1) {

         libfree_ptr = get_libfree(SETL_SYSTEM_VOID);
         libfree_ptr->lf_next = libfile_ptr->lf_libfree_list;
         libfile_ptr->lf_libfree_list = libfree_ptr;
         libfree_ptr->lf_head = header_ptr->uh_data_head[i];
         libfree_ptr->lf_tail = header_ptr->uh_data_tail[i];

      }
   }

   /*
    *  we are going to replace records from the hash table to the
    *  matching unit header
    */

   t2 = (libfile_ptr->lf_header)->lh_hash_tab[hash_code];
   libfree_ptr = get_libfree(SETL_SYSTEM_VOID);
   libfree_ptr->lf_next = libfile_ptr->lf_libfree_list;
   libfile_ptr->lf_libfree_list = libfree_ptr;
   libfree_ptr->lf_head = t2;
   libfree_ptr->lf_tail = unit_record;

   /* copy the search list */

   t1 = get_free_pos(SETL_SYSTEM libfile_ptr);
   memcpy((void *)lib_data.ld_data,
          (void *)&(libunit_ptr->lu_header),
          sizeof(unit_header));
   (libfile_ptr->lf_header)->lh_hash_tab[hash_code] = t1;

   while (t2 != unit_record) {

      lib_data.ld_next = get_free_pos(SETL_SYSTEM libfile_ptr);
      put_lib_rec(SETL_SYSTEM libfile_ptr,t1,(char *)&lib_data);
      t1 = lib_data.ld_next;
      get_lib_rec(SETL_SYSTEM libfile_ptr,t2,(char *)&lib_data);
      t2 = lib_data.ld_next;

   }
   lib_data.ld_next = t3;
   put_lib_rec(SETL_SYSTEM libfile_ptr,t1,(char *)&lib_data);

   /* return the unit record to the free list */

   free_libunit(libunit_ptr);

#endif

}

/*\
 *  \function{copy\_libunit()}
 *
 *  This function copies a unit from one library file to another.  It is
 *  used most often to transfer units from a temporary library to a
 *  permanent library, after a successful compilation.
\*/

#ifdef LIBWRITE

void copy_libunit(
   SETL_SYSTEM_PROTO
   char *unit_name,                    /* unit to be copied                 */
   libfile_ptr_type libfile_in,        /* input file pointer                */
   libfile_ptr_type libfile_out)       /* output file pointer               */

{
libunit_ptr_type libunit_in, libunit_out;
                                       /* input and output units            */
libstr_ptr_type libstr_in, libstr_out; /* input and output stream           */
char buffer[64];                       /* copy buffer                       */
int length;                            /* length of record read             */
int i;                                 /* temporary looping variable        */

   /* open the input and output units */

   libunit_in = open_libunit(SETL_SYSTEM unit_name,libfile_in,LIB_READ_UNIT);

#ifdef TRAPS

   if (libunit_in == NULL)
      trap(__FILE__,__LINE__,msg_bad_unit_copy,
           unit_name);

#endif

   libunit_out = open_libunit(SETL_SYSTEM unit_name,libfile_out,LIB_WRITE_UNIT);

   /* copy each stream */

   for (i = 0; i < LIB_STREAM_COUNT; i++) {

      libstr_in = open_libstr(SETL_SYSTEM libunit_in,i);
      libstr_out = open_libstr(SETL_SYSTEM libunit_out,i);
      while ((length = read_libstr(SETL_SYSTEM libstr_in,buffer,64)) > 0)
         write_libstr(SETL_SYSTEM libstr_out,buffer,length);
      close_libstr(SETL_SYSTEM libstr_in);
      close_libstr(SETL_SYSTEM libstr_out);

   }

   /* that's all -- close the units and return */

   close_libunit(SETL_SYSTEM libunit_in);
   close_libunit(SETL_SYSTEM libunit_out);

}

#endif

/*\
 *  \function{open\_libstr()}
 *
 *  This function opens a library stream.  All we have to do is allocate
 *  a stream node and set it up with an empty buffer.
\*/

libstr_ptr_type open_libstr(
   SETL_SYSTEM_PROTO
   libunit_ptr_type libunit_ptr,       /* unit containing stream            */
   int index)                          /* stream index within unit          */

{
libstr_ptr_type libstr_ptr;            /* returned stream pointer           */

#ifdef TRAPS
#ifdef LIBWRITE

   /*
    *  if the containing unit is opened for output, we can not have
    *  duplicate open streams
    */

   if (libunit_ptr->lu_is_output) {

      for (libstr_ptr = libunit_ptr->lu_libstr_list;
           libstr_ptr != NULL && libstr_ptr->ls_index != index;
           libstr_ptr = libstr_ptr->ls_next);

      if (libstr_ptr != NULL)
         trap(__FILE__,__LINE__,msg_dup_stream_open);

      if (libunit_ptr->lu_header.uh_data_head[index] != -1)
         trap(__FILE__,__LINE__,msg_dup_stream_open);

   }

#endif
#endif

   /* allocate and initialize the stream node */

   libstr_ptr = get_libstr(SETL_SYSTEM_VOID);
   libstr_ptr->ls_libunit_ptr = libunit_ptr;
   libstr_ptr->ls_next = libunit_ptr->lu_libstr_list;
   libunit_ptr->lu_libstr_list = libstr_ptr;

   libstr_ptr->ls_index = index;
   libstr_ptr->ls_buff_cursor = libstr_ptr->ls_buffer + LIB_DATA_SIZE;

   if (libunit_ptr->lu_is_output) {

      libstr_ptr->ls_record_num = -1;
      libstr_ptr->ls_bytes_left = 0;

   }
   else {

      libstr_ptr->ls_record_num = libunit_ptr->lu_header.uh_data_head[index];
      libstr_ptr->ls_bytes_left =
            libunit_ptr->lu_header.uh_data_length[index];

   }

   return libstr_ptr;

}

/*\
 *  \function{close\_libstr()}
 *
 *  This function closes an open library stream.  We flush the buffer and
 *  release the stream node.
\*/

void close_libstr(
   SETL_SYSTEM_PROTO
   libstr_ptr_type libstr_ptr)         /* stream to be closed               */

{
libfile_ptr_type libfile_ptr;          /* file containing stream            */
libunit_ptr_type libunit_ptr;          /* unit containing stream            */
libstr_ptr_type *temp_ptr;             /* used to remove stream from        */
                                       /* unit list                         */

   /* extract the file and unit pointers */

   libunit_ptr = libstr_ptr->ls_libunit_ptr;
   libfile_ptr = libunit_ptr->lu_libfile_ptr;

   /* remove the stream from the unit's stream list */

   for (temp_ptr = &(libunit_ptr->lu_libstr_list);
        *temp_ptr != NULL && *temp_ptr != libstr_ptr;
        temp_ptr = &((*temp_ptr)->ls_next));

#ifdef TRAPS

   if (*temp_ptr == NULL)
      trap(__FILE__,__LINE__,msg_bad_stream_close);

#endif

   *temp_ptr = (*temp_ptr)->ls_next;

#ifdef LIBWRITE

   /* if the stream is opened for output, flush the buffer */

   if (libunit_ptr->lu_is_output && libstr_ptr->ls_record_num != -1) {

      lib_data.ld_next = -1;
      memcpy((void *)lib_data.ld_data,
             libstr_ptr->ls_buffer,
             LIB_DATA_SIZE);
      put_lib_rec(SETL_SYSTEM libfile_ptr,
                  libstr_ptr->ls_record_num,
                  (char *)&lib_data);
      libunit_ptr->lu_header.uh_data_tail[libstr_ptr->ls_index] =
            libstr_ptr->ls_record_num;

   }

#endif

   free_libstr(libstr_ptr);

}

/*\
 *  \function{read\_libstr()}
 *
 *  This function reads a block from a library stream.  The interface is
 *  similar to the C \verb"read()" function.  We just provide buffered
 *  input of stream data.
\*/

unsigned read_libstr(
   SETL_SYSTEM_PROTO
   libstr_ptr_type libstr_ptr,         /* stream to be read                 */
   char *buffer,                       /* input buffer                      */
   unsigned buffer_length)             /* size of input buffer              */

{
libfile_ptr_type libfile_ptr;          /* file containing stream            */
libunit_ptr_type libunit_ptr;          /* unit containing stream            */
unsigned avail_length;                 /* size of record in buffer          */
unsigned return_length;                /* bytes transfered to buffer        */

   /* extract the file and unit pointers */

   libunit_ptr = libstr_ptr->ls_libunit_ptr;
   libfile_ptr = libunit_ptr->lu_libfile_ptr;

#ifdef TRAPS

   if (libunit_ptr->lu_is_output)
      trap(__FILE__,__LINE__,msg_bad_stream_read);

#endif

   /* if we are at the end of stream, return 0 */

   if (libstr_ptr->ls_bytes_left == 0)
      return 0;

   /* we can not return more bytes than we have in the stream */

   if (buffer_length > libstr_ptr->ls_bytes_left)
      buffer_length = (unsigned)(libstr_ptr->ls_bytes_left);

   /* avail_length is the number of bytes we have in the buffer */

   avail_length = min(buffer_length,
      libstr_ptr->ls_buffer + LIB_DATA_SIZE - libstr_ptr->ls_buff_cursor);

   /* copy what we have into the record */

   if (avail_length > 0) {

      memcpy((void *)buffer,
             (void *)(libstr_ptr->ls_buff_cursor),
             avail_length);
      libstr_ptr->ls_buff_cursor += avail_length;
      return_length = avail_length;

   }
   else {
      return_length = 0;
   }

   /* if more data is requested, fill our buffer */

   while (return_length < buffer_length) {

      get_lib_rec(SETL_SYSTEM libfile_ptr,libstr_ptr->ls_record_num,(char *)&lib_data);
      libstr_ptr->ls_record_num = lib_data.ld_next;
      memcpy((void *)libstr_ptr->ls_buffer,
             (void *)lib_data.ld_data,
             LIB_DATA_SIZE);
      libstr_ptr->ls_buff_cursor = libstr_ptr->ls_buffer;

      /* copy from this buffer into the record */

      avail_length = min(buffer_length - return_length,LIB_DATA_SIZE);

      memcpy((void *)(buffer+return_length),
             (void *)libstr_ptr->ls_buffer,
             avail_length);
      libstr_ptr->ls_buff_cursor += avail_length;
      return_length += avail_length;

   }

   libstr_ptr->ls_bytes_left -= return_length;

   return return_length;

}

/*\
 *  \function{write\_libstr()}
 *
 *  This function writes a block to a library stream.  The interface is
 *  similar to the C \verb"write()" function.  We just provide buffered
 *  output of stream data.
\*/

#ifdef LIBWRITE

void write_libstr(
   SETL_SYSTEM_PROTO
   libstr_ptr_type libstr_ptr,         /* stream to be written to           */
   char *buffer,                       /* output buffer                     */
   unsigned buffer_length)             /* size of output buffer             */

{
libfile_ptr_type libfile_ptr;          /* file containing stream            */
libunit_ptr_type libunit_ptr;          /* unit containing stream            */
int avail_length;                      /* space available in buffer         */
int return_length;                     /* bytes transfered from buffer      */

   /* extract the file and unit pointers */

   libunit_ptr = libstr_ptr->ls_libunit_ptr;
   libfile_ptr = libunit_ptr->lu_libfile_ptr;

#ifdef TRAPS

   if (!(libunit_ptr->lu_is_output))
      trap(__FILE__,__LINE__,msg_bad_stream_write);

#endif

   /* avail_length is the space available in our buffer */

   avail_length = min(buffer_length,
      libstr_ptr->ls_buffer + LIB_DATA_SIZE - libstr_ptr->ls_buff_cursor);

   /* copy the record into the space we have */

   if (avail_length > 0) {

      memcpy((void *)(libstr_ptr->ls_buff_cursor),
             (void *)buffer,
             avail_length);
      libstr_ptr->ls_buff_cursor += avail_length;
      return_length = avail_length;

   }
   else {
      return_length = 0;
   }

   /* if more data is requested, flush our buffer */

   while (return_length < buffer_length) {

      lib_data.ld_next = get_free_pos(SETL_SYSTEM libfile_ptr);

      if (libstr_ptr->ls_record_num == -1) {

         libunit_ptr->lu_header.uh_data_head[libstr_ptr->ls_index] =
            lib_data.ld_next;

      }
      else {

         memcpy((void *)lib_data.ld_data,
                libstr_ptr->ls_buffer,
                LIB_DATA_SIZE);
         put_lib_rec(SETL_SYSTEM libfile_ptr,libstr_ptr->ls_record_num,(char *)&lib_data);

      }

      libstr_ptr->ls_buff_cursor = libstr_ptr->ls_buffer;

      /* copy from the record to this buffer */

      avail_length = min(buffer_length - return_length,LIB_DATA_SIZE);

      libstr_ptr->ls_record_num = lib_data.ld_next;
      memcpy((void *)libstr_ptr->ls_buffer,
             (void *)(buffer+return_length),
             avail_length);
      libstr_ptr->ls_buff_cursor += avail_length;
      return_length += avail_length;

   }

   /* update the length of this stream and return */

   libunit_ptr->lu_header.uh_data_length[libstr_ptr->ls_index] +=
      return_length;

}

#endif

/*\
 *  \function{open\_lib\_file()}
 *
 *  This function opens a library file.  First we make sure we haven't
 *  exceeded the maximum number of files we are allowed.  This feature is
 *  primarily useful in MS-DOS, where the operating system restricts the
 *  number of files we can have open at a time.  Then we open the file
 *  and copy the hash table into a library file node.
\*/

static void open_lib_file(
   SETL_SYSTEM_PROTO
   libfile_ptr_type libfile_ptr)       /* file to be opened                 */

{
char fname[PATH_LENGTH + 1];           /* work file name (workaround for    */
                                       /* bug in Microsoft C!)              */
libfile_ptr_type temp_ptr;             /* used to close open files          */

   /*
    *  if we've exceeded the maximum number of open files, look for any
    *  files not in use, and close them
    */

   if (lib_files_open > LIB_MAX_OPEN) {

      for (temp_ptr = liblist_head;
           temp_ptr != NULL;
           temp_ptr = temp_ptr->lf_next) {

         if (temp_ptr->lf_is_open && temp_ptr->lf_libunit_list == NULL)
            close_lib_file(SETL_SYSTEM temp_ptr);

      }
   }

   if (lib_files_open > LIB_MAX_OPEN)
      giveup(SETL_SYSTEM msg_too_many_files);

   /* open the library */

   strcpy(fname,libfile_ptr->lf_fname);

   if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */

#ifdef MACINTOSH	/* Giuseppe 10/15/1999 */
/*\
 * Next Lines permit to resolve an alias (MacOS version of a Link)
 * so that we can have the library file somewhere else in the file system.
\*/	

//SPAM	SolveAliasByPathNames(fname);
	
#endif

      if (libfile_ptr->lf_is_writeable)
         libfile_ptr->lf_stream = fopen(fname,BINARY_RDWR);
        else
         libfile_ptr->lf_stream = fopen(fname,BINARY_RD);
      if (libfile_ptr->lf_stream == NULL)
         giveup(SETL_SYSTEM msg_lib_open_error,libfile_ptr->lf_fname);

   }

   /* allocate space for the header record */

   libfile_ptr->lf_header =
      (lib_header *)malloc(sizeof(lib_header));
   if (libfile_ptr->lf_header == NULL)
      giveup(SETL_SYSTEM msg_malloc_error);

   if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */
      /* position the library at the header */
   
      if (fseek(libfile_ptr->lf_stream,libfile_ptr->lf_header_pos,
                SEEK_SET) != 0)
         giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);

      /* read the header */
   
      if (fread((void *)libfile_ptr->lf_header,
                sizeof(lib_header),
                (size_t)1,
                libfile_ptr->lf_stream) != (size_t)1)
         giveup(SETL_SYSTEM msg_lib_read_error,libfile_ptr->lf_fname);

   } else {
     
      memcpy((void *)libfile_ptr->lf_header,
             (void *)mem_lib,
             sizeof(lib_header));

   }

   /* make sure we have a correct library */

   if (strcmp((libfile_ptr->lf_header)->lh_id,LIB_ID) != 0)
      giveup(SETL_SYSTEM msg_bad_library,libfile_ptr->lf_fname);

   /* initialize the library file information */

   libfile_ptr->lf_libfree_list = NULL;
   libfile_ptr->lf_is_open = YES;
   libfile_ptr->lf_next_free = -1;

   if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */
      lib_files_open++;
   }

}

/*\
 *  \function{close\_lib\_file()}
 *
 *  This function closes a library file.  First we write the disk's hash
 *  table.  Then we string together the deleted records, and finally we
 *  place them on the free list.  This sequence of operations is crucial,
 *  if the file is to remain correct even if the system crashes during
 *  this function's execution.
\*/

void close_lib_file(
   SETL_SYSTEM_PROTO
   libfile_ptr_type libfile_ptr)       /* file to be closed                 */

{
#ifdef LIBWRITE
long fseek_pos;                        /* used in converting pointers to    */
                                       /* file positions, and vice versa    */
libfree_ptr_type delete_ptr;           /* temporary free list pointer       */
#endif

#ifdef TRAPS

   /* make sure the file is really open */

   if (!(libfile_ptr->lf_is_open))
      trap(__FILE__,__LINE__,msg_bad_file_close);

#endif

   /* close any open units (there shouldn't be any) */

   while (libfile_ptr->lf_libunit_list != NULL)
      close_libunit(SETL_SYSTEM libfile_ptr->lf_libunit_list);

#ifdef LIBWRITE

   /* if the file was opened for output, update the directory */

   if (libfile_ptr->lf_is_writeable) {

      if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */
   
         /* position the library at the header */
   
         if (fseek(libfile_ptr->lf_stream,libfile_ptr->lf_header_pos,
                   SEEK_SET) != 0)
            giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);

         /* write the header */
   
         if (fwrite((void *)libfile_ptr->lf_header,
                    sizeof(lib_header),
                    (size_t)1,
                    libfile_ptr->lf_stream) != (size_t)1)
            giveup(SETL_SYSTEM msg_lib_write_error,libfile_ptr->lf_fname);

      } else  {
	
           to_memcpy(0,
                  (void *)libfile_ptr->lf_header,
                   sizeof(lib_header));
  

      }

      /* string together the free records on the library's free list */

      while (libfile_ptr->lf_libfree_list != NULL) {

         /* fseek to the position of the tail */

         fseek_pos = (long)((libfile_ptr->lf_libfree_list)->lf_tail) *
                     (long)LIB_BLOCK_SIZE +
                     libfile_ptr->lf_header_pos;


         if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */

            if (fseek(libfile_ptr->lf_stream,fseek_pos,SEEK_SET) != 0)
               giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);
   
            /* make the next pointer point to the current free list */
   
            if (fwrite((void *)&((libfile_ptr->lf_header)->lh_free_list),
                       sizeof(unsigned),
                       (size_t)1,
                       libfile_ptr->lf_stream) != (size_t)1)
               giveup(SETL_SYSTEM msg_lib_write_error,libfile_ptr->lf_fname);

         } else { 

             to_memcpy(fseek_pos,
                    (void *)&((libfile_ptr->lf_header)->lh_free_list),
                    sizeof(unsigned));


         }

         /* point the free list pointer to the start of the chain */

         (libfile_ptr->lf_header)->lh_free_list =
            (libfile_ptr->lf_libfree_list)->lf_head;

         /* set up for the next list */

         delete_ptr = libfile_ptr->lf_libfree_list;
         libfile_ptr->lf_libfree_list =
            (libfile_ptr->lf_libfree_list)->lf_next;
         free_libfree(delete_ptr);

      }

      if (libfile_ptr->lf_is_writeable) {

         if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */

            /* position the library at the header */
   
            if (fseek(libfile_ptr->lf_stream,libfile_ptr->lf_header_pos,
                      SEEK_SET) != 0)
               giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);
   
            /* write the header */
      
            if (fwrite((void *)libfile_ptr->lf_header,
                       sizeof(lib_header),
                       (size_t)1,
                       libfile_ptr->lf_stream) != (size_t)1)
               giveup(SETL_SYSTEM msg_lib_write_error,libfile_ptr->lf_fname);

          } else {
   
              to_memcpy(0,
                     (void *)libfile_ptr->lf_header,
                      sizeof(lib_header));
          }
       }

   }

#endif

   /* close the file and update the table information */

   if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */
      fclose(libfile_ptr->lf_stream);
      lib_files_open--;
   }
   free((void *)libfile_ptr->lf_header);
   libfile_ptr->lf_libfree_list = NULL;
   libfile_ptr->lf_is_open = NO;
   libfile_ptr->lf_next_free = -1;

}

/*\
 *  \function{get\_free\_pos()}
 *
 *  This function allocates a free block in the library file.  We handle
 *  free space a little strangely.  Basically, we are making a crude
 *  attempt to preserve the integrity of the library in the event of a
 *  system crash.  Here's the general idea.
 *
 *  First we check whether the free list is empty (pointer == -1).  If so,
 *  this is easy -- we just allocate a block at the end of the file.  We
 *  keep a pointer to the end of the file, since we only return pointers
 *  to free lists, but don't write anything in them.  If we didn't keep a
 *  pointer, and the caller didn't write something in its block before we
 *  are called again, we would return the same pointer.
 *
 *  If the list is not empty, we read the pointer to the following free
 *  block into the free list head.  Then we write it immediately back
 *  to disk.  That way, if a system crash occurs, the only damage which
 *  was done is that there will be junk records in the file which are not
 *  reachable -- not a great disaster.  The junk records are not reachable
 *  because we don't write the hash table until the file is closed.
\*/

#ifdef LIBWRITE

static unsigned get_free_pos(
   SETL_SYSTEM_PROTO
   libfile_ptr_type libfile_ptr)       /* library for which we would like   */
                                       /* to find a free record             */

{
unsigned free_pos;                     /* the free position we find         */
long fseek_pos;                        /* used in converting pointers to    */
                                       /* file positions, and vice versa    */

   /* if the free list is empty, allocate a block at the end of the file */

   if ((libfile_ptr->lf_header)->lh_free_list == -1) {

      /* if the next free position is at the end of file, find EOF */

      if (libfile_ptr->lf_next_free == -1) {

         if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */

            if (fseek(libfile_ptr->lf_stream,0L,SEEK_END) != 0)
               giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);
            fseek_pos = ftell(libfile_ptr->lf_stream);
            if (fseek_pos == -1)
               giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);

         } else {
            fseek_pos = dl_count;
         }
         fseek_pos -= libfile_ptr->lf_header_pos;
         libfile_ptr->lf_next_free =
            (unsigned)(fseek_pos / (long)LIB_BLOCK_SIZE);

      }

      free_pos = libfile_ptr->lf_next_free;
      (libfile_ptr->lf_next_free)++;

#ifdef MPWC

      /*
       *  The fseek() function in MPW C suffers from serious
       *  brain-damage.  It will not seek to the end of file!  We write
       *  junk here, because we're going to return a position past the
       *  end of file.
       */

      if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */
         if (fseek(libfile_ptr->lf_stream,0L,SEEK_END) != 0)
            giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);
         if (fwrite((void *)(libfile_ptr->lf_header),
                    (size_t)LIB_BLOCK_SIZE,
                    (size_t)1,
                    libfile_ptr->lf_stream) != (size_t)1)
            giveup(SETL_SYSTEM msg_lib_write_error,libfile_ptr->lf_fname);
      }

#endif

   }
   else {

      /*
       * get the first free block, read it's link to the next one,
       * and store it in the header
       */

      free_pos = (libfile_ptr->lf_header)->lh_free_list;
      fseek_pos = (long)free_pos * (long)LIB_BLOCK_SIZE +
                  libfile_ptr->lf_header_pos;

      if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */
         if (fseek(libfile_ptr->lf_stream,fseek_pos,SEEK_SET) != 0)
            giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);

         /* read the link to the next free record */

         if (fread((void *)&((libfile_ptr->lf_header)->lh_free_list),
                   sizeof(unsigned),
                   (size_t)1,
                   libfile_ptr->lf_stream) != (size_t)1)
            giveup(SETL_SYSTEM msg_lib_read_error,libfile_ptr->lf_fname);
      } else { 

              memcpy((void *)&((libfile_ptr->lf_header)->lh_free_list),
                     (void *)(mem_lib+fseek_pos),
                      sizeof(unsigned));
   
      }
            

      /* position library at the free list in the header record */

      if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */
         if (fseek(libfile_ptr->lf_stream,
                   libfile_ptr->lf_header_pos,SEEK_SET) != 0)
            giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);

         /* reset the free list pointer */

         if (fwrite((void *)&((libfile_ptr->lf_header)->lh_free_list),
                    sizeof(unsigned),
                    (size_t)1,
                    libfile_ptr->lf_stream) != (size_t)1)
            giveup(SETL_SYSTEM msg_lib_write_error,libfile_ptr->lf_fname);
       } else { 

             to_memcpy(libfile_ptr->lf_header_pos,
                    (void *)&((libfile_ptr->lf_header)->lh_free_list),
                    sizeof(unsigned));

       }

   }

   return free_pos;

}

#endif

/*\
 *  \function{get\_lib\_rec()}
 *
 *  This function reads a buffer from a library file.   We translate a
 *  record pointer to a file position, \verb"fseek()" to that position,
 *  and read the record.
\*/

static void get_lib_rec(
   SETL_SYSTEM_PROTO
   libfile_ptr_type libfile_ptr,       /* library file pointer              */
   unsigned record_number,             /* logical record number             */
   char *buffer)                       /* record buffer                     */

{
long fseek_pos;                        /* used in converting pointers to    */
                                       /* file positions, and vice versa    */

      /* fseek to the position of the record */

      fseek_pos = (long)record_number * (long)LIB_BLOCK_SIZE +
                  libfile_ptr->lf_header_pos;

   if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */
      if (fseek(libfile_ptr->lf_stream,fseek_pos,SEEK_SET) != 0)
         giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);

      /* read the record */
   
      if (fread((void *)buffer,
                LIB_BLOCK_SIZE,
                (size_t)1,
                libfile_ptr->lf_stream) != (size_t)1)
         giveup(SETL_SYSTEM msg_lib_read_error,libfile_ptr->lf_fname);
   } else {

      memcpy((void *)buffer,
             (void *)(mem_lib+fseek_pos),
             (LIB_BLOCK_SIZE));

   }

}

/*\
 *  \function{put\_lib\_rec()}
 *
 *  This function writes a buffer to a library file.  We translate a
 *  record pointer to a file position, \verb"fseek()" to that position,
 *  and write the record.
\*/

#ifdef LIBWRITE

static void put_lib_rec(
   SETL_SYSTEM_PROTO
   libfile_ptr_type libfile_ptr,       /* library file pointer              */
   unsigned record_number,             /* logical record number             */
   char *buffer)                       /* record buffer                     */

{
long fseek_pos;                        /* used in converting pointers to    */
                                       /* file positions, and vice versa    */

   /* fseek to the position of the record */

   fseek_pos = (long)record_number * (long)LIB_BLOCK_SIZE +
               libfile_ptr->lf_header_pos;

   if (libfile_ptr->lf_mem_lib==0) { /* Library on disk... */
      if (fseek(libfile_ptr->lf_stream,fseek_pos,SEEK_SET) != 0)
         giveup(SETL_SYSTEM msg_lib_fseek_error,libfile_ptr->lf_fname);

      /* write the record */

      if (fwrite((void *)buffer,
                 (size_t)LIB_BLOCK_SIZE,
                 (size_t)1,
                 libfile_ptr->lf_stream) != (size_t)1)
         giveup(SETL_SYSTEM msg_lib_write_error,libfile_ptr->lf_fname);
   } else {

      to_memcpy(fseek_pos,
             (void *)buffer,
             (LIB_BLOCK_SIZE));

   }

}

#endif

/*\
 *  \function{hashpjw()}
 *
 *  This function is an implementation of a hash code function due to
 *  P.~J.~Weinberger taken from \cite{dragon}.  According to that text,
 *  this hash function performs very well for a wide variety of strings.
 *
 *  We have not copied the code exactly, although we compute the same
 *  function.  The function in the text assumes \verb"unsigned" will be
 *  32 bits long.  We have used \verb"#defines" to define constants
 *  which will be equivalent to theirs if we are on a 32 bit machine, but
 *  will be scaled up or down on other machines.  This should make the
 *  function run faster on 16 bit machines (we are counting on the C
 *  compiler to do constant folding, otherwise this might be slow).
\*/

static unsigned hashpjw(
   char *s)                            /* string to be hashed               */

{
unsigned hash_code;                    /* hash code eventually returned     */
unsigned top_four;                     /* top four bits of hash code        */
char *p;                               /* temporary looping variable        */
#define MASK (0x0f << (sizeof(unsigned) * 8 - 4))
                                       /* bit string with four high order   */
                                       /* bits of integer on, others off    */
#define SHIFT (sizeof(unsigned) * 8 - 8);
                                       /* shift distance                    */

   hash_code = 0;
   for (p = s; *p; p++) {

      hash_code = (hash_code << 4) + *p;
      if (top_four = hash_code & MASK) {
         hash_code ^= top_four >> SHIFT;
         hash_code ^= top_four;
      }

   }

   return hash_code % LIB_HASH_SIZE;

}

