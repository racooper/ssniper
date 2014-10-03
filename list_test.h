/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/
#ifndef LIST_TEST_H
#define LIST_TEST_H

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "logging.h"
#include "get_line.h"

int list_skip_location(char *location);
void push_list_skip_loc(char *location);
void read_skip_list(char *filename);
int max_int(int a, int b);


#endif
