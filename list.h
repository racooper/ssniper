/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#ifndef LIST_H
#define LIST_H

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ----------------------------------------------------------------------
// This is a generic node structure for a simply linked list.  This is
// not intended to be very flashy, since it serves a very simple purpose
// in the main program.
// ----------------------------------------------------------------------

struct node
{  
     void * data;
     struct node *next;
};

struct node * append_list(struct node *list, void * data);
void destroy_list(struct node *list);
void print_string_list(struct node *list);
int string_submatch_p(char *match, struct node *list);

#endif
