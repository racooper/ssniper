/*

Copyright © 2007, The Board of Trustees of the University of Illinois.

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include "ssn_fsm.h"


// The state machine keeps track of the SSNs it finds and state meta
// information in the 'states' array.  The fields in the elements of
// this array are as follows:
//
// 0:   State active before consuming next char or not (1/0)
// 1-9: Accumulated SSN digits
// 10:  Value of first separator (or 0 if none)
// 11:  Value of second separator (or 0 if none)


// Believe me, macro-izing these makes things a lot faster.  The
// "digit_state" and "separator_state" macros are just to keep the
// code looking nicer (and prevent typos).  The state machine is
// actually fairly simple when you think about it.

#define digit(c)       (c >= '0' && c <= '9')
#define nondigit(c)    (!digit(c) && \
                        !(c <= 'z' && c >= 'a') && \
                        !(c <= 'Z' && c >= 'A'))
#define separator(c)   (c == ' ' || c == '-' || c == '.')
#define activate(c)    states[c][0] = (char)1
// #define deactivate(c)  states[c][0] = (char)0; states[c][12] = 0
#define deactivate(c)  states[c][0] = (char)0; states[c][10] = 0; states[c][11] = 0


// digit_state args: 
//  (1) state to move from
//  (2) destination state
//  (3) index of SSN digit to record

#define digit_state(f,t,d) \
     if(states[f][0])      \
     {                     \
          if(digit(c))     \
          {                      \
               push(c, f, t, d); \
               activate(t);      \
          }                      \
          deactivate(f);         \
     }


// separator_state args:
//  (1) state to move from
//  (2) state to go to on separator
//  (3) state to go to on digit
//  (4) index of SSN digit to record (when digit)
//  (5) index of SSN separator to record (when separator)

#define separator_state(f, t1, t2, d, s) \
     if(states[f][0])                 \
     {                                \
          if(digit(c))                \
          {                                     \
               push(c, f, t2, d);               \
               activate(t2);                    \
          }                                     \
          else if(separator(c))                 \
          {                                     \
               push(c, f, t1, s);               \
               activate(t1);                    \
          }                                     \
          deactivate(f);                        \
     }


char *states[13];


// ----------------------------------------------------------------------
// fsm_free_machine() -- we don't want to preserve state machine state 
// between files, so the calling code should free the machine state
// before scanning a new file.  Each state's buffer is malloc'd, so we
// just free them here.
// ----------------------------------------------------------------------

void fsm_free_machine()
{
     int x;
     for(x=0; x<13; x++)
     {
	  free(states[x]);
     }
}


// ----------------------------------------------------------------------
// fsm_initialize_machine() -- Because we want to keep track of the SSN
// digits accumulated through the state machine, we have a buffer
// attached to each state.  This buffer contains space for storing SSN
// digits, state activity, and some other tracking info.  One 
// optimization is to swap pointers around instead of copying memory
// regions -- so we want to dynamically allocate these buffers here.
// ----------------------------------------------------------------------

int fsm_initialize_machine()
{
     int x;
     for(x=0; x<13; x++)
     {
          states[x] = (char *)malloc(13);
          memset(states[x], 0, 13);
     }
     states[0][0] = 1;
     states[1][0] = 1;
}


// ----------------------------------------------------------------------
// push() -- This takes care of the work of processing a state.  When the
// appropriate tests have been completed (testing for accept characters,
// for example), we want to move from one state to another, and possibly
// keep a digit char for the detected SSN.  
// ----------------------------------------------------------------------

void push(char c, int state_from, int state_to, int index)
{
     char *temp;

	 states[state_from][index] = c;

     temp = states[state_to];
     temp[11] = 0;
     temp[10] = 0;
     states[state_to] = states[state_from];
     states[state_from] = temp;
}


// ----------------------------------------------------------------------
// represent_ssn() -- to keep from dirtying up other code, we want to
// take a textual representation of an SSN and split it into its three
// values.  This is intended to provide three strings which can be
// converted to ints if needed.
// ----------------------------------------------------------------------

void represent_ssn(char *area, char *group, char *serial, char *ssn)
{
     strncpy(area, ssn, 3);
     strncpy(group, ssn+3, 2);
     strncpy(serial, ssn+5, 4);
     area[3] = (char) 0;
     group[2] = (char) 0;
     serial[4] = (char) 0;
}


// ----------------------------------------------------------------------
// fsm_match_buffer() -- This is the workhorse function for the state 
// machine.  It uses the macros defined above to expand out the logic
// for each state.  It's a messy function (especially when macro
// expanded!), but should generally not need to be changed from now on.
// In the future, I should probably break out the "exit" state (12)
// into another function.
// ----------------------------------------------------------------------

struct ssn_results fsm_match_buffer(char *buffer, int len, 
				    struct ssn_results results)
{
     int i, j, total = 0;
     char area[4], group[3], serial[5];
     char *x, c;

     for(x=buffer; x < (buffer + len); x++)
     {
          c = *x;

          // state 12 is special
          if(states[12][0])
          {
               if(nondigit(c))
               {
		    results = tally_ssn(area, group, serial, results);
               }
               deactivate(12);
          }

          // detect the serial number group
          digit_state(11, 12, 9);
          digit_state(10, 11, 8);
          digit_state(9, 10, 7);
          digit_state(8, 9, 6);

          separator_state(7, 8, 9, 6, 11);

          // detect the group code
          digit_state(6, 7, 5);
          digit_state(5, 6, 4);

          separator_state(4, 5, 6, 4, 10);

          // detect the area code
          digit_state(3, 4, 3);
          digit_state(2, 3, 2);
          digit_state(1, 2, 1);

          if(states[0][0] && nondigit(c)) activate(1);
     }
     return results;
}


// ----------------------------------------------------------------------
// tally_ssn() -- This function will take a detected SSN and adjust the
// results struct accordingly.  SSNs are validated against max area/group
// codes published by the Social Security Administration (SSA).
// ----------------------------------------------------------------------

struct ssn_results tally_ssn(char *area, char *group, char *serial, 
			     struct ssn_results results)
{
     represent_ssn(area, group, serial, &states[12][1]);
     if(states[12][10] && states[12][11])
     {
	  if(valid_ssn(atoi(area), atoi(group), atoi(serial)) && 
	     states[12][10]==states[12][11])
	  {
	       results.delim++;
	       results.positives++;
	  }
	  else results.falses++;
     }
     else if(states[12][10] == 0 && states[12][11] == 0) 
     {
	  if(valid_ssn(atoi(area), atoi(group), atoi(serial)))
	  {
	       results.nondelim++;
	       results.positives++;
	  }
	  else results.falses++;
     }
     else results.falses++;
   
     return results;
}
