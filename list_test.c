/*

Copyright © 2007, The Board of Trustees of the University of Illinois.

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include "list_test.h"

extern char LOGBUF[LOG_BUF_SZ];

struct node *list_skip_list = NULL;

// ----------------------------------------------------------------------
// max_int() -- I'm sure this exists somewhere, but it's also very quick
// to write, so I just threw it in.  This makes the semantics in
// list_skip_location much clearer.
// ----------------------------------------------------------------------

int max_int(int a, int b)
{
     if(a<b) return b;
     return a;
}


// ----------------------------------------------------------------------
// list_skip_location() -- returns true (1) if the provided location is
// on the skip list, false (0) otherwise.  This is a little picky -- it
// won't tolerate "valid" paths that should match.  E.g., extra slashes:
// if "/etc/passwd" is on the skip list, but "/etc//passwd" is passed in,
// it will not match.  Take special note in other code using this
// function.
// ----------------------------------------------------------------------

int list_skip_location(char *location)
{
     struct node *cursor;
     int len;

     if(list_skip_list)
     {
	  for(cursor = list_skip_list; cursor != NULL; cursor = cursor->next)
	  {
	       len = max_int(strlen(location), strlen((char *)cursor->data));

	       if(strncmp(location, (char *)cursor->data, len) == 0)
	       {
		    snprintf(LOGBUF, LOG_BUF_SZ, "skipping %s : on skip list", location);
		    slog(LOGBUF, LOG_DEB);
		    return 1;
	       }
	  }
     }

     return 0;
}


// ----------------------------------------------------------------------
// push_list_skip_loc() -- add a location to the skip list.  This is just
// a linked list of strings that will be kept around to be checked
// against each file and directory encountered.
// ----------------------------------------------------------------------

void push_list_skip_loc(char *location)
{
     char *buf;
     int len;
     len = strlen(location);
     buf = (char *)malloc(len+1);
     strncpy(buf, location, len + 1);
     list_skip_list = append_list(list_skip_list, buf);
}


// ----------------------------------------------------------------------
// read_skip_list() -- given a filename, read in the skip list.  It
// should be one path per line.  The line is read explicitly, so spaces
// or other whitespace characters will be considered as part of the file-
// name.  Hopefully, you won't have newlines in your filenames.
// ----------------------------------------------------------------------

void read_skip_list(char *filename)
{
     char buf[4096];
     FILE *infile;
     
     if((infile = fopen(filename, "r")) == NULL)
     {
	  printf("Error opening skip list (%s) : %s\n",
		 filename, strerror(errno));
	  return;
     }

     while(!feof(infile))
     {
	  get_line(buf, infile, 4096);
	  // we have a line of input, which should just be a 
	  // filesystem location to skip.  These MUST be
	  // absolute paths, since it's too complex to process
	  // relative paths from the initial scan working dir.
	  push_list_skip_loc(buf);
     }
}
