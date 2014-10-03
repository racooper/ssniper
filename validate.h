/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#ifndef VALIDATE_H
#define VALIDATE_H

#include <stdio.h>
#include "config.h"
#include "logging.h"

int group_sets(int from, int to, int c);
void init_groups();
int valid_ssn(int area, int group, int serial);
FILE * open_code_file(char *filename);

#endif
