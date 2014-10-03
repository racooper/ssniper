/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#ifndef SCAN_FILE_H
#define SCAN_FILE_H

#include "config.h"

#include <sys/types.h>
#include <regex.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#ifndef DISABLE_BZIP
#include <bzlib.h>
#endif

#ifndef DISABLE_ZLIB
#include <zlib.h>
#endif

#include "ssniper.h"
#include "constants.h"
#include "ssn_fsm.h"
#include "logging.h"

struct ssn_results match_buffer(char * buffer, int buff_len, struct ssn_results results);
struct ssn_results  scan_file(char *filename, int type, struct ssn_results results);
void * open_file(char * filename, int type);
int read_block(void * handle, char * buffer, int len, int type);
void close_file(void * handle, int type);
FILE * open_read(char *filename);
int scan_block(FILE *infile, int type, struct ssn_results * results);
void update_summary_stats(struct ssn_results spec, struct ssn_results *gen);
void report_progress(struct ssn_results results);

#ifndef DISABLE_BZIP
void * open_bzip_file(char *filename);
#endif

#endif
