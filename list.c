/*

Copyright © 2007, The Board of Trustees of the University of Illinois.

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include "list.h"


// ----------------------------------------------------------------------
// append_list() -- This will append the provided data as a new node
// at the beginning of the list.
// ----------------------------------------------------------------------

struct node * append_list(struct node *list, void * data)
{
     struct node *temp;

     temp = (struct node *)malloc(sizeof(struct node));

     temp->data = data;
     temp->next = NULL;

     if(list != NULL)
     {
	  temp->next = list;
     }

     return temp;
}


// ----------------------------------------------------------------------
// destroy_list() -- This will free each node on the list and the base
// pointer provided as 'list'.
// ----------------------------------------------------------------------

void destroy_list(struct node *list)
{
     if(list != NULL)
     {
	  destroy_list(list->next);
	  free(list->data);
	  free(list);
     }
}


// ----------------------------------------------------------------------
// print_string_list() -- for debugging, assume that every node's data
// is a (char *) and print them as the list is traversed.
// ----------------------------------------------------------------------

void print_string_list(struct node *list)
{
     struct node *cursor;
     for(cursor = list; cursor != NULL; cursor = cursor->next)
     {
	  printf("%s ", (char *)cursor->data);
     }
     printf("\n");
}


// ----------------------------------------------------------------------
// string_submatch_p() -- assuming that the data payload is a char *, 
// check to see if any of the nodes' data includes the provided string
// as a substring.  Return the payload of the matchin node or NULL.
// ----------------------------------------------------------------------

int string_submatch_p(char * match, struct node *list)
{
     struct node *cursor;
     for(cursor = list; cursor != NULL; cursor = cursor->next)
     {
	  if(strstr(match, (char *)cursor->data) == NULL)
	  {
	       // then we found one where it's a substring
	       return 1;
	  }
     }
     return 0;
}
