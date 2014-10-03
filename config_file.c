/*

Copyright © 2007, The Board of Trustees of the University of Illinois.

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include "config_file.h"

extern struct node *root_list;

extern int quiet;
extern char LOGBUF[LOG_BUF_SZ];
extern int override_magic_file;
extern long long file_byte_cap;
extern char *ssn_code_file;
extern int override_code_file;
extern char *email_address;

#ifndef DISABLE_LIBMAGIC
extern char * magic_filepath;
#endif

// ----------------------------------------------------------------------
// config_readline() -- read a line from the config file and parse it 
// into the structure defined in config_file.h.  This abstracts away
// the scanning of the variables, and lets the rest of the program
// treat them as abstract, parsed data.
// ----------------------------------------------------------------------

struct config_line * config_readline(FILE * infile)
{
     char buffer[CONFIG_BUFFER];
     int num, len;
     struct config_line *line;
     line = (struct config_line *)malloc(sizeof(struct config_line));

     num = get_line(buffer, infile, CONFIG_BUFFER);
     if(num < 1)
     {
	  return NULL;
     }

     if(buffer[0] == '#')
     {
	  return (struct config_line *)CF_COMMENT;
     }

     line->directive = (char *)malloc(num + 1);
     line->value = (char *)malloc(num + 1);

     num = sscanf(buffer, "%s \"%[^\"]", line->directive, line->value);

     if(num == -1)
     {
	  if(feof(infile))
	  {
	       return NULL;
	  }
	  return (struct config_line *)CF_BLANK;
     }

     if(num != 2)
     {
	  printf("Error reading config file on line:\n");
	  printf("  %s", buffer);
	  exit(4);
     }
     return line;
}


// ----------------------------------------------------------------------
// process_config() -- read in the attributes defined in the config file
// and conduct the appropriate activities.  This should be run before 
// anything that sets up modules that require configuration from the 
// config file!
// ----------------------------------------------------------------------

int process_config(char * filename)
{
     FILE *infile;
     char *buffer;
     struct config_line *line;

     snprintf(LOGBUF, LOG_BUF_SZ, "Loading config file from %s", filename);
     slog(LOGBUF, LOG_SCR);

     if((infile = fopen(filename, "r")) == NULL)
     {
	  if(!quiet)
	  {
	       printf("ERROR: cannot open config file\n");
	       printf("Expected to find it at %s\n", filename);
	       printf("You can override the compiled-in default with the -c flag\n");
	  }
	  return 0;
     }

     while(line = config_readline(infile))
     {
	  if((int)line != CF_COMMENT && (int)line != CF_BLANK)
	  {
	       if(strncmp(line->directive, "skip", 5) == 0)
	       {
		    #ifndef DISABLE_LIBMAGIC
		    push_skip_type(line->value);
		    #endif
	       }
	       if(strncmp(line->directive, "skipext", 8) == 0)
	       {
		    push_skip_ext(line->value);
	       }
	       if(strncmp(line->directive, "bytecap", 8) == 0)
	       {
		    file_byte_cap = atoi(line->value);
	       }
               #ifndef DISABLE_LIBMAGIC
	       if(strncmp(line->directive, "magic", 6) == 0 && !override_magic_file)
	       {
		    magic_filepath = (char *)malloc(strlen(line->value) + 1);
		    strncpy(magic_filepath, line->value, strlen(line->value)+1);
	       }
               #endif
	       if(strncmp(line->directive, "codes", 6) == 0 && !override_code_file)
	       {
		    ssn_code_file = (char *)malloc(strlen(line->value) + 1);
		    strncpy(ssn_code_file, line->value, strlen(line->value)+1);
	       }
	       if(strncmp(line->directive, "root", 5) == 0)
	       {
		    buffer = (char *)malloc(strlen(line->value) + 1);
		    strncpy(buffer, line->value, strlen(line->value) + 1);
		    root_list = append_list(root_list, buffer);
	       }
	       if(strncmp(line->directive, "email", 6) == 0)
	       {
		    email_address = (char *)malloc(strlen(line->value) + 1);
		    strncpy(email_address, line->value, strlen(line->value) + 1);
	       }
	       config_kill_line(line);
	  }
     }
     return 1;
}


// ----------------------------------------------------------------------
// config_kill_line() -- This function will free a configuration file
// line struct, since the strings in it are malloc'd.  
// ----------------------------------------------------------------------

void config_kill_line(struct config_line *line)
{
     free(line->directive);
     free(line->value);
     free(line);
}
