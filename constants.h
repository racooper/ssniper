/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "config.h"

// global constants (mostly from ssniper.h)
#define DEBUG           0
#define BUFFLEN     32768
#define BUFFOFF       -15
#define NOTYPE       0x00
#define DIRTYPE      0x01
#define FILETYPE     0x02
#define MAXMATCHES     10
#define NUMEXPRS        2
#define NOTE_INTERVAL  10

// constants for output / logging (logging.h)
#define LOG_RES 0
#define LOG_DEB 1
#define LOG_INF 2
#define LOG_SCR 3
#define LOG_BUF_SZ 4096

// constants for file scanning (scan_file.h)
#define SF_SKIP        -1
#define SF_STANDARD     0
#define SF_GZIP         1
#define SF_BZIP         2
#define NUMEXPRS        2

#define USAGE_ERROR     1

// constants for config file processing (config_file.h)
#define CONFIG_BUFFER 256
#define CF_COMMENT -1
#define CF_BLANK -2

// ----------------------------------------------------------------------
// ssn_results struct is intended to hold values as a file is scanned.
// As false positive detection gets more complex, this is a good place
// to keep track of things.
// ----------------------------------------------------------------------

struct ssn_results {
     int positives;
     int delim;
     int nondelim;
     int falses;
     int skip;
     int total;
     int scanned;
     int dirs;
     long long bytes;
};

#endif
