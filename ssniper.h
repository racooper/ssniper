/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#ifndef SSN_WEB_H
#define SSN_WEB_H

#include "config.h"

#include <sys/types.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>

#include "constants.h"
#include "validate.h"
#include "list.h"
#include "magic_test.h"
#include "extension_test.h"
#include "scan_file.h"
#include "logging.h"
#include "list_test.h"

int       error;
int       cwd_size;


// ----------------------------------------------------------------------
// dirinfo struct holds a list of files and subdirectories in it.  One
// can run out of file handles if one is not careful in recursively
// descending a filesystem.  So, when a directory entry is read, its
// contents are stored in these linked lists, and the handle is
// closed.
// ----------------------------------------------------------------------

struct dirinfo
{
     struct node *files;
     struct node *subdirs;
};


// ----------------------------------------------------------------------
// Function listing
// ----------------------------------------------------------------------

char * get_cwd();
struct dirinfo * read_directory(char *dirname);
void free_directory(struct dirinfo *info);
struct ssn_results traverse_directory(char *dirname, struct ssn_results results);
int main (int argc, char **argv);
void show_result(char *filename, struct ssn_results results);
void process_dir_entry(struct dirent *entry, char *cwd, struct dirinfo *info, char *newnode);
void process_file_entry(struct dirent *entry, char *cwd, struct dirinfo *info, char *newnode);
void show_summary_stats(struct ssn_results results);
void blank_results(struct ssn_results * results);
void banner();
void configuration_notifications();
int process_args(int argc, char **argv);
void usage();
struct ssn_results initiate_scan(char *filename, struct ssn_results results);
int skip_any(char *cwd, char *filename);


#endif
