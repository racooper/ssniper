/*

Copyright © 2007, The Board of Trustees of the University of Illinois

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#ifndef SSN_FSM_H
#define SSN_FSM_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "constants.h"
#include "validate.h"

int fsm_initialize_machine();
void push(char c, int state_from, int state_to, int index);
struct ssn_results fsm_match_buffer(char *buffer, int len, struct ssn_results results);
struct ssn_results tally_ssn(char *area, char *group, char *serial, struct ssn_results results);

#endif
