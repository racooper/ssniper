/*

Copyright © 2007, The Board of Trustees of the University of Illinois.

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include "extension_test.h"

struct node *ext_skip_list = NULL;
extern char LOGBUF[LOG_BUF_SZ];

// ----------------------------------------------------------------------
// find_last() -- return a pointer to the last occurrence of the 'key' 
// char in the string provided.  This is accomplished by iterating back-
// wards from the end of the string.  The last char and the first char
// will be skipped (which reduces the general usability of this
// function, admittedly).
// ----------------------------------------------------------------------

char * find_last(char * str, char key)
{
     char *cursor;
     for(cursor = str + strlen(str) - 2; cursor > str; cursor--)
	  if(*cursor == key)
	       return cursor + 1;
     return NULL;
}


// ----------------------------------------------------------------------
// push_skip_ext() -- add to the list of strings that mark file
// extensions to be skipped.  This just makes it look prettier than
// managing the list manually (and abstracts it in case I change the
// way it works).
// ----------------------------------------------------------------------

void push_skip_ext(char * desc)
{
     char * buf;
     buf = (char *)malloc(strlen(desc));
     strncpy(buf, desc, strlen(desc)+1);
     snprintf(LOGBUF, LOG_BUF_SZ, "skipping files with extension '%s'", buf);
     slog(LOGBUF, LOG_INF);
     ext_skip_list = append_list(ext_skip_list, buf);
}


// ----------------------------------------------------------------------
// skip_extension_p() -- Return a pointer to the extension identified
// if the extension is on the "skip list".  If not, return NULL.
// ----------------------------------------------------------------------

char * skip_extension_p(char *filename)
{
     struct node *cursor;
     const char * extension;

     extension = find_last(filename, '.');
     if(extension)
     {
	  for(cursor = ext_skip_list; cursor != NULL; cursor = cursor->next)
	  {
	       if(strcmp(extension, (char *)cursor->data) == 0)
	       {
		    return (char *)cursor->data;
	       }
	  }
     }
     return NULL;
}

