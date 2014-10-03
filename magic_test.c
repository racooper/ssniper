/*

Copyright © 2007, The Board of Trustees of the University of Illinois.

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include <magic.h>
#include <stdio.h>
#include <string.h>

#include "magic_test.h"
#include "list.h"

extern char LOGBUF[LOG_BUF_SZ];

char * magic_filepath;
magic_t MAGIC = NULL;
struct node *skip_list = NULL;


// ----------------------------------------------------------------------
// push_skip_type() -- add to the list of strings that mark files to be
// skipped.  This just makes it look prettier than managing the list 
// manually (and abstracts it in case I change the way it works).
// ----------------------------------------------------------------------

void push_skip_type(char * desc)
{
     char * buf;
     buf = (char *)malloc(strlen(desc));
     strncpy(buf, desc, strlen(desc)+1);
     snprintf(LOGBUF, LOG_BUF_SZ, "skipping files matching '%s'", buf);
     slog(LOGBUF, LOG_INF);
     skip_list = append_list(skip_list, buf);
}


// ----------------------------------------------------------------------
// magic_init() -- Set up the environment to be checking magic numbers on
// files.  This includes setting up the list of 'keyphrases' in the magic
// description that will flag a file to be skipped (i.e., ISO 9660 imgs)
// ----------------------------------------------------------------------

void magic_init()
{
     MAGIC = magic_open(0);
     snprintf(LOGBUF, LOG_BUF_SZ, "loading magic file from %s", magic_filepath);
     slog(LOGBUF, LOG_INF);
     magic_load(MAGIC, (const char *)magic_filepath);
}


// ----------------------------------------------------------------------
// skip_file_p() -- Given a filename, check its magic description and 
// return true if it is a type that should be skipped in the SSN scan.
// This will let us skip pictures, movies, disk images, etc.
// ----------------------------------------------------------------------

char * skip_file_p(char *filename)
{
     struct node *cursor;
     const char * magic;

     magic = magic_file(MAGIC, filename);
     if(magic)
     {
	  for(cursor = skip_list; cursor != NULL; cursor = cursor->next)
	  {
	       if(strstr(magic, (char *)cursor->data) != 0)
	       {
		    return ((char *)cursor->data);
	       }
	  }
     }
     return NULL;
}

int zip_file_p(char *filename)
{

}
