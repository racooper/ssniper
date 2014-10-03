/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#ifndef MAGIC_TEST_H
#define MAGIC_TEST_H

#include "config.h"
#include "logging.h"

void push_skip_type(char * desc);
char * skip_file_p(char *filename);
void magic_init();

#endif
