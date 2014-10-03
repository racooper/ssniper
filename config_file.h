/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"
#include "magic_test.h"
#include "extension_test.h"
#include "logging.h"
#include "get_line.h"

struct config_line {
     char * directive;
     char * value;
};

struct config_line * config_readline(FILE * infile);
void config_kill_line(struct config_line *line);
int process_config(char * filename);

#endif
