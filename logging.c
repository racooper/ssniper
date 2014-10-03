/*

Copyright © 2007, The Board of Trustees of the University of Illinois.

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include "logging.h"

extern int quiet;

FILE *results_log;
FILE *debug_log;
FILE *info_log;

char LOGBUF[LOG_BUF_SZ];

void log_init(char * name)
{
     char *r, *i, *d;
     int len;

     if(name == NULL)
     {
	  name = "ssniper";
     }
     
     len = strlen(name);

     r = (char *)malloc(len + 13);
     i = (char *)malloc(len + 10);
     d = (char *)malloc(len + 11);

     snprintf(r, len + 13, "%s_results.log", name);
     snprintf(i, len + 10, "%s_info.log", name);
     snprintf(d, len + 11, "%s_debug.log", name);

     if(!quiet) printf("Opening %s for scan results\n", r);
     if((results_log = fopen(r, "w")) == NULL)
     {
	  printf("ERROR opening results file %s\n", r);
	  printf("Do you have permission and does the location exist?\n");
	  exit(2);
     }

     if(!quiet) printf("Opening %s for scan progress / info\n", i);
     if((info_log = fopen(i, "w")) == NULL)
     {
	  printf("ERROR opening results file %s\n", r);
	  printf("Do you have permission and does the location exist?\n");
	  exit(2);
     }

     if(!quiet) printf("Opening %s for debug info / errors\n", d);
     if((debug_log = fopen(d, "w")) == NULL)
     {
	  printf("ERROR opening results file %s\n", r);
	  printf("Do you have permission and does the location exist?\n");
	  exit(2);
     }
}

void slog(char *msg, int level)
{
     switch(level)
     {
     case LOG_RES:
	  fprintf(results_log, "%s\n", msg);
	  fflush(results_log);
	  break;
     case LOG_INF:
	  fprintf(info_log, "%s\n", msg);
	  fflush(info_log);
	  break;
     case LOG_DEB:
	  fprintf(debug_log, "%s\n", msg);
	  break;
     case LOG_SCR:
	  fprintf(info_log, "%s\n", msg);
	  fflush(info_log);
	  if(!quiet) 
	  {
	       printf("%s\n", msg);
	       fflush(stdout);
	  }
	  break;
     }
}


// ----------------------------------------------------------------------
// print_timestamp() -- Print a # (comment char) out, then a timestamp,
// then the provided string.  This is for annotating result output in
// a reliable, filterable way.
// ----------------------------------------------------------------------

void print_timestamp(char *text)
{
     char time_str[512];
     time_t result;

     // shamelessly modified from `man localtime`

     result = time(NULL);
     strftime(time_str, 512, "%Y-%m-%d %T", localtime(&result));
     snprintf(LOGBUF, LOG_BUF_SZ, "%s %s", time_str, text);
     slog(LOGBUF, LOG_SCR);
}

