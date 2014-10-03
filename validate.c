/*

Copyright © 2007, The Board of Trustees of the University of Illinois.

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include "validate.h"

#define MAX_AREA   999

extern char LOGBUF[LOG_BUF_SZ];

// ----------------------------------------------------------------------
// This is an array holding a list of max group codes for each area
// code.  The index of the array is the area code.
// ----------------------------------------------------------------------

int max[1000];

int group_order[101];

char *ssn_code_file;

// ----------------------------------------------------------------------
// group_sets() -- This is just a functional abstraction from the
// init_groups() function below.  It generates a sequence of numbers,
// counting by twos.
// ----------------------------------------------------------------------

int group_sets(int from, int to, int c)
{
     int x;
     for(x=from; x<=to; x+=2, c++)
     {
          group_order[x] = c;
     }
     return c;
}


FILE * open_code_file(char *filename)
{
     FILE *infile;

     snprintf(LOGBUF, LOG_BUF_SZ, "Reading SSN max group codes from %s", 
	      filename);
     slog(LOGBUF, LOG_SCR);

     if((infile = fopen(filename, "r")) == NULL)
     {
	  snprintf(LOGBUF, LOG_BUF_SZ, "Failed reading from %s", 
		   filename);
	  slog(LOGBUF, LOG_SCR);
	  return NULL;
     }
     return infile;
}

// ----------------------------------------------------------------------
// init_groups() -- set up the data structures (namely an array that
// defines the group-code order of assignment).  This speeds things 
// up when validating SSNs.
// ----------------------------------------------------------------------

void init_groups()
{
     int c = 1, x, idex, val;
     FILE *infile;

     for(x=0; x<=MAX_AREA; x++) max[x] = 0;

     if((infile = open_code_file(ssn_code_file)) == NULL)
     {
	  if((infile = open_code_file("./ssn_codes.txt")) == NULL)
	  {
	       printf("ERROR: can't open SSN max group code file\n");
	       printf("you can override the compiled-in default location with the -r switch\n");
	       printf("\nThis location is defined in the config file (normally ssniper.conf)\n");
	       printf("with the 'codes' directive\n");
	       exit(2);
	  }	  
     }

     while(fscanf(infile, "%d %d\n", &idex, &val) != EOF)
     {
	  max[idex] = val;
     }

     fclose(infile);

     c = group_sets(1, 9, c);
     c = group_sets(10, 98, c);
     c = group_sets(2, 8, c);
     c = group_sets(11, 99, c);
}


// ----------------------------------------------------------------------
// valid_ssn -- return true if the provided SSN is potentially valid.
// there may be more rules to add to this later, as it could probably
// be more complex.
// ----------------------------------------------------------------------

int valid_ssn(int area, int group, int serial)
{
     int max_group = max[area];
     int max_index = group_order[max_group];
     int qry_index = group_order[group];
     int ret = 0;

     //     printf("# SSN Validation: %03d-%02d-%04d\n", area, group, serial);
     if(area <= MAX_AREA)
     {
	  if(qry_index <= max_index)
	       ret = 1;
	  
	  if(area == 0 || group == 0 || area == 666)
	       ret = 0;
     }

     //     printf("# Result: %d\n", ret);
     return ret;
}
