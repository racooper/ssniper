/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include "get_line.h"

// ----------------------------------------------------------------------
// get_line() -- This is a portable function for reading a line.  Turns
// out there are some vagaries with GNU libraries, so I decided to play
// it safe here.
// ----------------------------------------------------------------------

int get_line(char *line, FILE *infile, int max)
{
     int x, done, c;
     for(x=0, done=0; ! done; x++)
     {
          c = fgetc(infile);
          switch(c)
          {
          case EOF:
	       done = 1;
               break;
          case '\n':
               done = 1;
               break;
          case '\r':
               done = 1;
               break;
          default:
               line[x] = (char)c;
               break;
          }
     }
     line[x-1] = (char)0x00;
     return x;
}

