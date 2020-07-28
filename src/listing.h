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
 *  \packagespec{listing}
\*/

#ifndef LISTING_LOADED

/* public function declarations */

#if UNIX_VARARGS
void error_message(SETL_SYSTEM_PROTO_VOID);
                                       /* log error message                 */
void warning_message(SETL_SYSTEM_PROTO_VOID);                
                                       /* log warning message               */
void info_message(SETL_SYSTEM_PROTO_VOID);                   
                                       /* log info message                  */
#else
void error_message(SETL_SYSTEM_PROTO struct file_pos_item *, char *, ...);
                                       /* log error message                 */
void warning_message(SETL_SYSTEM_PROTO struct file_pos_item *, char *, ...);
                                       /* log warning message               */
void info_message(SETL_SYSTEM_PROTO struct file_pos_item *, char *, ...);
                                       /* log info message                  */
#endif
void print_errors(SETL_SYSTEM_PROTO_VOID);  
                                       /* print error list                  */
void print_listing(SETL_SYSTEM_PROTO_VOID);            
                                       /* print source listing              */
void generate_markup(SETL_SYSTEM_PROTO_VOID);    
                                       /* insert errors in source file      */
void free_err_table(SETL_SYSTEM_PROTO_VOID);

#ifdef PLUGIN
int setl_num_errors();
char * setl_err_string(int i);
#endif


#define LISTING_LOADED 1
#endif
