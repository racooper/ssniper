/*
University of Illinois/NCSA
Open Source License

Copyright © 2007, The Board of Trustees of the University of Illinois.
All rights reserved.

Developed by:

    Technology Services Group
    Department of Computer Science

    University of Illinois at Urbana-Champaign

    http://agora.cs.uiuc.edu/display/tsg

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal with the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimers.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimers in the documentation and/or other materials provided
      with the distribution.

    * Neither the names of Technology Services Group, Department of
      Computer Science; University of Illinois at Urbana-Champaign; nor
      the names of its contributors may be used to endorse or promote
      products derived from this Software without specific prior
      written permission.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
*/

// ----------------------------------------------------------------------
// ssniper.c -- This is a program that scans the current directory and
// all subdirectories for SSNs.  This is intended to be lightweight and
// fast, and does not yet have robust features for stripping out false
// positives or understanding filetypes.  It will scan binary files,
// though, so embedded strings with SSNs should be identified.
//
// SSNs are validated according to rudimentary area/group-code sequences
// as defined by the SSA.  The code for this checking is in the
// associated file validate.c.  
//
// Author:  Joshua Stone
// Contact: joshs@uiuc.edu
//
// 
// ----------------------------------------------------------------------

#include "ssniper.h"

#define fallback_config_file "./ssniper.conf"

char *database_file;

extern char LOGBUF[LOG_BUF_SZ];
extern long long file_byte_cap;
extern char *ssn_code_file;
long long total_bytes;

#ifndef DISABLE_LIBMAGIC
extern char *magic_filepath;
#endif

struct node *root_list = NULL;

// These are flags that govern overriding of configuration options by
// command line arguments (set in process_args(), used elsewhere)

int       override_magic_file;
int       override_code_file;
char     *report_basename;
char     *config_filename;
int       skip_gzip_override;
int       skip_bzip_override;
int       quiet;
time_t    timer;
char     *email_address = NULL;
int       review_positives;

// ----------------------------------------------------------------------
// main() -- stripped of all real functionality, main just sets things
// up and sets off the recursive traversal.
// ----------------------------------------------------------------------

int main (int argc, char **argv)
{
     char      errbuff[1024];
     char     *cwd;
     int       x;
     int       stoparg;
     char      printbuf[1024];

     struct ssn_results  results;
     struct node *cursor;

     blank_results(&results);
     
     cwd_size = pathconf(".", _PC_PATH_MAX);

     stoparg = process_args(argc, argv);

#ifndef DISABLE_SQLITE
     if(database_file) init_database();
     if(review_positives)
     {
	 printf("\nReviewing un-marked positives in database:\n\n");
	 db_review();
	 close_database();
	 return 0;
     }

#endif
     
     log_init(report_basename);
     if(!process_config(config_filename))
     {
	  if(!quiet) printf("Attempting fallback config file (%s)\n", fallback_config_file);
	  process_config(fallback_config_file);
     }

     if(stoparg < argc || root_list != NULL)
     {
	  if(!quiet) banner();

	  // initialize groups data structure for SSN validation
	  // (validate.c)
	  init_groups();
	  
#ifndef DISABLE_LIBMAGIC
	  magic_init();
#endif
	  
	  configuration_notifications();
	  
	  slog("\nScan Progress:\n-------------", LOG_SCR);
	  
	  print_timestamp("Starting scan");

#ifndef DISABLE_SQLITE
	  if(database_file)
	  {
#endif
	       for(cursor = root_list; cursor != NULL; cursor = cursor->next)
	       {
		    snprintf(printbuf, 1024, "Scanning configured root dir %s", (char *)cursor->data);
		    print_timestamp(printbuf);
		    results = initiate_scan((char *)cursor->data, results);
	       }
#ifndef DISABLE_SQLITE
	  }
	  else
	  {
#endif
	       
	       for(; stoparg < argc; stoparg++)
	       {
		    snprintf(printbuf, 1024, "Scanning basedir %s", argv[stoparg]);
		    print_timestamp(printbuf);
		    results = initiate_scan(argv[stoparg], results);
	       }
#ifndef DISABLE_SQLITE
	  }
#endif

     }
     else
     {
	  usage();
     }

#ifndef DISABLE_SQLITE
     if(email_address) db_notifications(email_address);
     if(database_file) close_database();
#endif

     print_timestamp("Scan Completed");
     show_summary_stats(results);

     return 0;
}


// ----------------------------------------------------------------------
// initiate_scan() -- Start the scan on a given directory and return the
// accumulated results.
// ----------------------------------------------------------------------

struct ssn_results initiate_scan(char *filename, struct ssn_results results)
{
     char *cwd; 
     int x;

     // initiate scan on first directory
     cwd = get_cwd();
     x = chdir(filename);
     switch(x)
     {
     case -1:
	  printf("ERROR: location '%s' does not exist\n", filename);
	  break;
     default:
	  timer = time(NULL);
	  results = traverse_directory(filename, results);
	  chdir(cwd);
     }
     free(cwd);
     return results;
}

// ----------------------------------------------------------------------
// Here we want to provide useful output to the user.  We also want to
// provide a percentage of valid / invalid SSNs, as this is a useful
// metric for someone to decide if they're real SSN hits or not.
// ----------------------------------------------------------------------

void show_result(char *filename, struct ssn_results results)
{
     char *cwd;
     char *path;
     double positives, total;
     int ratio, x;
     
     cwd = get_cwd();
#ifndef DISABLE_SQLITE
     if(database_file == NULL)
     {
#endif
	  positives = (double)results.positives;
	  total = (double)(results.positives + results.falses);
	  ratio = (int)(positives / total * 100);
	  
	  x = snprintf(LOGBUF, LOG_BUF_SZ, "% 6d (delim) % 6d (non) % 6d (tot) ",
		       results.delim, results.nondelim, results.positives);
	  snprintf(LOGBUF + x, LOG_BUF_SZ - x, " % 6d (false) % 4d (ratio %%) %s/%s",
		   results.falses, ratio, cwd, filename);
	  slog(LOGBUF, LOG_RES);
	  
	  fflush(stdout);
#ifndef DISABLE_SQLITE
     }
     else
     {
	  // record the result in the SQLite3 database file
	  x = strlen(cwd) + strlen(filename) + 2;
	  path = (char *)malloc(x);
	  snprintf(path, x, "%s/%s", cwd, filename); 
	  db_process_result(path, results);
	  free(path);
     }
#endif
	  
     free(cwd);
}


// ----------------------------------------------------------------------
// read_directory -- This opens a directory and pulls information from
// its list of entries.  They are stored in a dirinfo struct (defined
// above).  This is part of an optimization to preserve file handles,
// as a more compact mechanism will exhaust system resources too
// quickly.  So, this snarfs directory info and closes the handle as
// soon as possible.  Note that when directory information is read, it
// must be freed later (see the free_directory() function below).
// ----------------------------------------------------------------------

struct dirinfo * read_directory(char *dirname)
{
     struct dirinfo *info;
     DIR *dirp;
     struct dirent **namelist;
     struct dirent *entry;
     char *temp;
     char *cwd;
     
     int n = 0;

     info = (struct dirinfo *)malloc(sizeof(struct dirinfo));

     info->files = NULL;
     info->subdirs = NULL;

     cwd = get_cwd();

     if((dirp = opendir(cwd)) == NULL)
     {
	  return NULL;
     }
     do
     {
	  if((entry = readdir(dirp)) != NULL)
	  {
	       temp = (char *)malloc(strlen(entry->d_name) + 1);
	       strncpy(temp, entry->d_name, strlen(entry->d_name) + 1);

	       switch(get_mode(entry->d_name))
	       {
	       case DIRTYPE:
		    process_dir_entry(entry, cwd, info, temp);
		    break;
	       case FILETYPE:
		    process_file_entry(entry, cwd, info, temp);
		    break;
	       }
	  }
     } while(entry != NULL);

     free(cwd);
     closedir(dirp);

     return info;
}

// ----------------------------------------------------------------------
// get_mode() -- an abstraction for getting a file's mode -- in case we
// reach a platform that doesn't work the same way, this will be a good
// place to trap OS differences (instead of read_directory() above).
// ----------------------------------------------------------------------

int get_mode(char *filename)
{
     struct stat buffer;
     char *cwd;

     if(lstat(filename, &buffer) != 0)
     {
	  return NOTYPE;
     }
     
     switch(buffer.st_mode & S_IFMT)
     {
     case S_IFREG:
	  return FILETYPE;
     case S_IFDIR:
	  return DIRTYPE;
     case S_IFLNK:
	  cwd = get_cwd();
	  snprintf(LOGBUF, LOG_BUF_SZ, "skipping %s/%s: it's a symbolic link", cwd, filename);
	  slog(LOGBUF, LOG_DEB);
	  free(cwd);
	  return NOTYPE;
     }
     return NOTYPE;
}

// ----------------------------------------------------------------------
// These two functions detail how files and directories are handled in
// the directory entry.  There are special cases that we want to skip,
// since they often hold weird files (such as in /proc, where they may
// look like a normal file, but are actually hooks into kernel
// functionality).
// ----------------------------------------------------------------------

void process_file_entry(struct dirent *entry, char *cwd, struct dirinfo *info, char *newnode)
{
     info->files = append_list(info->files, (void *)newnode);
}

void process_dir_entry(struct dirent *entry, char *cwd, struct dirinfo *info, char *newnode)
{
     if(strcmp(entry->d_name, ".") != 0 &&
	strcmp(entry->d_name, "..") != 0)
     {
	  if(strcmp(cwd, "/") == 0 && 
	     (strcmp(entry->d_name, "proc") == 0 || 
	      strcmp(entry->d_name, "sys") == 0 ||
	      strcmp(entry->d_name, "dev") == 0))
	  {
	       // fprintf(stderr, "Skipping directory: %s/%s\n", cwd, entry->d_name);
	  }
	  else
	  {
	       info->subdirs = append_list(info->subdirs, (void *)newnode);
	  }
     }
     else
     {
	  free(newnode);
     }
}


// ----------------------------------------------------------------------
// The dirinfo struct that is created and populated by the above
// read_directory() function must be freed, otherwise we have a memory
// leak.  This function accomplishes that.
// ----------------------------------------------------------------------

void free_directory(struct dirinfo *info)
{
     destroy_list(info->files);
     destroy_list(info->subdirs);
     free(info);
}



int skip_any(char *cwd, char *filename)
{
     int ret = 0;
     int full_len;
     char *ext;
     char *full;

#ifndef DISABLE_LIBMAGIC
     char *magic;
#endif

     ext = skip_extension_p(filename);

     // has to be +2 because of null termination and added slash char
     full_len = strlen(cwd) + strlen(filename) + 2;
     full = (char *)malloc(full_len);
     snprintf(full, full_len + 1, "%s/%s", cwd, filename);

#ifndef DISABLE_LIBMAGIC
     magic = skip_file_p(filename);
#endif

     if(list_skip_location(full))
     {
	  ret = SF_SKIP;
     }
     else
     {

#ifndef DISABLE_LIBMAGIC
	  if(magic == NULL)
	  {
#endif

	       if(ext == NULL) 
               {
                    free(full);
		    return SF_STANDARD;
               }
               else ret = SF_SKIP;

#ifndef DISABLE_LIBMAGIC
	  }
	  else
	  {
#ifndef DISABLE_ZLIB
	       if(strncmp(magic, "gzip", strlen(magic)) == 0 && !skip_gzip_override)
               {
                    free(full);
                    return SF_GZIP;
               }
#endif
	  
#ifndef DISABLE_BZIP
	       if(strncmp(magic, "bzip", strlen(magic)) == 0 && !skip_bzip_override)
               {
                    free(full);
		    return SF_BZIP;
               }
               ret = SF_SKIP;
#endif
	  }
#endif
     }

     if(ret == SF_SKIP)
     {
          #ifndef DISABLE_LIBMAGIC
	  snprintf(LOGBUF, LOG_BUF_SZ, "Skipped %s/%s m:'%s' e:'%s'",
  		   cwd, filename, magic, ext);
          #else
	  snprintf(LOGBUF, LOG_BUF_SZ, "Skipped %s/%s e:'%s'",
		   cwd, filename, ext);
          #endif
	  slog(LOGBUF, LOG_DEB);
     }
     free(full);
     return ret;
}
// ----------------------------------------------------------------------
// traverse_directory() -- This function will start at the directory indicated
// by "dirname", and recursively descend the filesystem.  The action for 
// checking standard files for SSNs is hard-coded.  In the future, I might
// want to break that out to a callback, so this will become a more generic
// function.
// ----------------------------------------------------------------------

int count = 0;

struct ssn_results traverse_directory(char *dirname, struct ssn_results results)
{
     struct dirinfo *info;
     struct node *cursor;
     int n = 0, x = 0, scanned;
     int scantype;
     char *name;
     char *cwd;
     char printbuf[1024];

     cwd = get_cwd();
     info = read_directory(dirname);

     if(info && (!list_skip_location(cwd)))
     {
	  for(cursor = info->files; cursor != NULL; cursor = cursor->next, n++)
	  {
	       results.total++;
	       scanned = 0;
	       name = (char *)cursor->data;

	       if((scantype = skip_any(cwd, name)) != SF_SKIP)
	       {
		    results = scan_file(name, scantype, results);
                    results.scanned++;
                    scanned = 1;
	       }

	       if(!scanned) results.skip++;
	  }
	  
	  for(cursor = info->subdirs; cursor != NULL; cursor = cursor->next)
	  {
	       results.dirs++;
	       name = (char *)cursor->data;
	       
	       if((x = chdir(name)) == 0)
	       {
		    results = traverse_directory(".", results);
		    
		    chdir(cwd);
	       }
	       else
	       {
		    printf("# error %s on %s/%s\n", strerror(errno), cwd, name);
	       }
	  }
	  free_directory(info);
     }
     free(cwd);
     return results;
}


// ----------------------------------------------------------------------
// get_cwd() -- returns a malloc'ed string containing the current
// working directory.  This should be freed at some point!
// ----------------------------------------------------------------------

char * get_cwd()
{
     char *cwd;
     cwd = (char *)malloc(cwd_size + 1);
     getcwd(cwd, cwd_size);
     return cwd;
}


// ----------------------------------------------------------------------
// blank_results() -- This function will reset an ssn_results struct to
// zero (since, depending on your compiler, the initial values may not
// be 0 to start with).
// ----------------------------------------------------------------------

void blank_results(struct ssn_results * results)
{
     results->positives = 0;
     results->delim = 0;
     results->nondelim = 0;
     results->falses = 0;
     results->skip = 0;
     results->scanned = 0;
     results->total = 0;
     results->dirs = 0;
     results->bytes = 0;
}


// ----------------------------------------------------------------------
// show_summary_stats() -- print out the summary stats about the whole
// scan.  Ideally, discrepancies in these numbers can be indications of
// unanticipated failures.  
// ----------------------------------------------------------------------

void show_summary_stats(struct ssn_results results)
{
     int x;
     
     snprintf(LOGBUF, LOG_BUF_SZ, "\nSummary Stats / Aggregate Scan Results:", 
	      results.total, results.dirs);
     slog(LOGBUF, LOG_SCR);

     snprintf(LOGBUF, LOG_BUF_SZ, "---------------------------------------", 
	      results.total, results.dirs);
     slog(LOGBUF, LOG_SCR);

     snprintf(LOGBUF, LOG_BUF_SZ, "Found %d files and %d subdirectories", 
	      results.total, results.dirs);
     slog(LOGBUF, LOG_SCR);

     snprintf(LOGBUF, LOG_BUF_SZ, "Skipped %d files", results.skip);
     slog(LOGBUF, LOG_SCR);

     snprintf(LOGBUF, LOG_BUF_SZ, "Scanned %d files", results.scanned);
     slog(LOGBUF, LOG_SCR);

     snprintf(LOGBUF, LOG_BUF_SZ, "Identified %d files with possible delimited SSNs", 
	      results.delim);
     slog(LOGBUF, LOG_SCR);

     snprintf(LOGBUF, LOG_BUF_SZ, "Identified %d files with possible non-delimited SSNs", 
	      results.nondelim);
     slog(LOGBUF, LOG_SCR);

     snprintf(LOGBUF, LOG_BUF_SZ, "Identified %d files with false positives", 
	      results.falses);
     slog(LOGBUF, LOG_SCR);

     snprintf(LOGBUF, LOG_BUF_SZ, "Processed %lld total bytes\n", total_bytes);
     slog(LOGBUF, LOG_SCR);

     if(!quiet)
     {
	  printf("----------------------------------------------------------\n\n");
	  printf("DON'T FORGET TO RUN RESULTS THROUGH THE POST-PROCESSOR!\n");
	  printf("\nThis will eliminate lots of false positives and\n");
	  printf("categorize the hits so you can spend your review time\n");
	  printf("wisely.\n");
	  printf("\nYou can do that with the following command:\n\n");
	  printf("    cat %s_results.log | ssniper-report.pl > %s_report.txt\n\n",
		 report_basename, report_basename);
     }
}

// ----------------------------------------------------------------------
// banner() -- the initial banner announcemount about what we're doing,
// where we came from, and how to contact me.
// ----------------------------------------------------------------------

void banner()
{
     int x = 0;
     
     printf("\nSSNiper %s: SSN scanner (maintainer: Joshua Stone, joshs@uiuc.edu)\n", VERSION);
     x = printf("Copyright (C) 2007 The Board of Trustees of the University of Illinois\n");
     for(; x>1; x--) putchar('-');
     printf("\n");
     printf("(License: University of Illinois/NCSA Open Source License)\n\n");
}


// ----------------------------------------------------------------------
// configuration_notifications() -- provide output based on the compile
// time configuration.  This is where we tell people if some important
// module is not compiled in.
// ----------------------------------------------------------------------

void configuration_notifications()
{
     if(!quiet)
     {
#ifdef DISABLE_LIBMAGIC
	  slog("# libmagic not linked at compile, filetypes will not be checked\n", LOG_INF);
#endif
	  
#ifdef DISABLE_ZLIB
	  slog("# zlib not linked at compile, gzipped files will not be searched\n", LOG_INF);
#endif
	  
#ifdef DISABLE_BZIP
	  slog("# bzip2 not linked at compile, bzipped files will not be searched\n", LOG_INF);
#endif
	  
	  if(file_byte_cap)
	  {
	       snprintf(LOGBUF, LOG_BUF_SZ, "Setting file read byte cap to %lld", file_byte_cap);
	       slog(LOGBUF, LOG_SCR);
	  }
     }
}


// ----------------------------------------------------------------------
// process_args() -- take care of whatever we're going to do in response
// to command line arguments.
// ----------------------------------------------------------------------

int process_args(int argc, char **argv)
{
     int c;

     override_magic_file = 0;
     override_code_file = 0;
     report_basename = "ssniper";
     config_filename = SSNIPER_CONF;
     ssn_code_file = SSN_CODE_FILE;
     skip_bzip_override = 0;
     skip_gzip_override = 0;
     total_bytes = 0;
     file_byte_cap = 0;
     quiet = 0;
     database_file = NULL;
     review_positives = 0;

     while((c = getopt(argc, argv, "e:d:zl:bhm:n:c:qr:s:p:")) != -1)
     {
	  switch((char)c)
	  {
	  case 'e':
	       email_address = optarg;
	       break;
	  case 'h':
	       usage();
	       break;
	  case 'm':
               #ifndef DISABLE_LIBMAGIC
	       override_magic_file = 1;
	       magic_filepath = optarg;
               #endif
	       break;
	  case 'n':
	       report_basename = optarg;
	       break;
	  case 'c':
	       config_filename = optarg;
	       break;
	  case 'd':
#ifndef DISABLE_SQLITE	    
	       database_file = optarg;
	       quiet = 1;
#else
	       printf("!!! QUIET DATABASE SCANNING NOT COMPILED IN\n");
	       exit(1);
#endif
	       break;
	  case 'l':
	       file_byte_cap = atoi(optarg);
	       break;
          #ifndef DISABLE_ZLIB
	  case 'z':
	       skip_gzip_override = 1;
	       break;
	  #endif
          #ifndef DISABLE_BZIP
	  case 'b':
	       skip_bzip_override = 1;
	       break;
	  #endif
	  case 'q':
	       quiet = 1;
	       break;
	  case 'r':
	       override_code_file = 1;
	       ssn_code_file = optarg;
	       break;
	  case 's':
	       read_skip_list(optarg);
	       break;
	  case 'p':
#ifndef DISABLE_SQLITE
	      database_file = optarg;
	      review_positives = 1;
#else
	      printf("!!! DATABASE SCANNING NOT COMPILED IN\n");
	      exit(1);
#endif
	      break;
	  case '?':
	  case ':':
	       usage();
	  }
     }

     return optind;
}


// ----------------------------------------------------------------------
// usage() -- This is the documentation function that will show the user
// how to use ssniper if the command line options are wrong.
// ----------------------------------------------------------------------

void usage()
{
     banner();
     printf("\n usage: ssniper -[hmnclzbrsdq] [<dir>*]\n");
     printf("\n    <dir>         Directory to scan\n");
     printf("    -h            This help output\n");
     printf("    -m <magic>    Override location of magic file\n");
     printf("    -n <name>     Name prefix on output files*\n");
     printf("    -c <conf>     Configuration file (default ./ssniper.conf)\n");
     printf("    -l <bytes>    Limit file scanning cap to first <bytes> bytes\n");
     printf("    -z            Skip gzip files (if compiled in)\n");
     printf("    -b            Skip bzip files (if compiled in)\n");
     printf("    -r <file>     Read SSN max group codes from <file>\n");
     printf("    -s <file>     Use <file> as list of absolute paths to skip\n");
     printf("    -d <file>     Use <file> as the result database and run quietly\n");
     printf("    -q            Run quietly (less output)\n");
     printf("    -e <addr>     Email address to notify in DB mode\n");
     printf("    -p <file>     Review false positives in a DB and exit\n");
     printf("\n");
     exit(USAGE_ERROR);
}
