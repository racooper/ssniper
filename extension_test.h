/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#ifndef EXTENSION_TEST_H
#define EXTENSION_TEST_H

#include <stdio.h>
#include <string.h>

#include "list.h"
#include "logging.h"

char * skip_extension_p(char *filename);
void push_skip_ext(char * desc);
char * find_last(char * str, char key);


#endif
