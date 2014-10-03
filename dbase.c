/*

Copyright © 2007, The Board of Trustees of the University of Illinois.

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include "dbase.h"
#include <stdlib.h>
#include <stdio.h>

extern char *database_file;

/* These are some SQL queries defined for the functions below.  I
 * wanted them in defines like this because they make the code look
 * really messy. */

#define SQL_CREATE_DB         "create table results (path, delim, nondelim, total, falses, ratio, first, last, false_positive, notified)"
#define SQL_CHECK_DB          "select * from sqlite_master where type='table' and name='results'"
#define SQL_CHECK_PATH        "select * from results where path like '%q'"
#define SQL_UPDATE_TIMESTAMP  "update results set last = datetime('now') where path = '%q'"
#define SQL_INSERT_RESULT     "insert into results values ('%s', %d, %d, %d, %d, %f, datetime('now'), datetime('now'), 0, 0);"
#define SQL_FIND_NEW          "select path from results where false_positive = 0"
#define SQL_MARK_SENT         "update results set notified = 1 where path = '%q'"
#define SQL_MARK_FALSE_P      "update results set false_positive = 1 where path = '%q'"

/* This is used as the beginning of the email notification subject
 * header.  It will be followed by the hostname (see code below) */

char *subject = "SSNiper notification from";

sqlite3 *db;
char buffer[4096];

// ----------------------------------------------------------------------
// init_database() -- This is intended to be called once by the main
// program (you can find the call in ssniper.c).  It will set up the
// database for use in databased scanning mode.  The DB is an sqlite3
// local DB with one table.  This function will also create a fresh DB
// if the file is not already initialized.
// ----------------------------------------------------------------------

void init_database()
{
     int x; 
     int numrows;
     int numcols;
     char *errmsg;
     char **queryRes;
     
     x = sqlite3_open(database_file, &db);
     if(x != SQLITE_OK)
     {
	  fprintf(stderr, "Error opening database file %s\n", database_file);
	  exit(2);
     }
     
     sqlite3_get_table(db, SQL_CHECK_DB, 
		       &queryRes, &numrows, &numcols, &errmsg);
     if(numrows != 1)
     {
	  sqlite3_exec(db, SQL_CREATE_DB, 0, 0, 0);
     }
}


// ----------------------------------------------------------------------
// db_process_result() -- This function should be called with the scan
// results of each individual file that has met the criteria for being
// reported.  This is determined in the ssniper.c file.  This function
// should be considered an alternative to the standard output methods,
// and will put results in the database file.
// ----------------------------------------------------------------------

void db_process_result(char *path, struct ssn_results results)
{
     char **queryRes;
     char *errmsg;
     double ratio;
     int numrows;
     int numcols;
     int total;
     char *pathcpy;

     total = results.positives + results.falses;
     ratio = ((double)results.positives) / ((double)total);

     sqlite3_snprintf(4096, buffer, SQL_CHECK_PATH, path);
     sqlite3_get_table(db, buffer, &queryRes, &numrows, &numcols, &errmsg);
     
     if(numrows > 0)
     {
	  sqlite3_snprintf(4096, buffer, SQL_UPDATE_TIMESTAMP, path);
	  sqlite3_get_table(db, buffer, &queryRes, &numrows, &numcols, &errmsg);
     }
     else
     {
	  snprintf(buffer, 4096, SQL_INSERT_RESULT, path, results.delim, 
		   results.nondelim, total, results.falses, ratio);
	  sqlite3_get_table(db, buffer, &queryRes, &numrows, &numcols, &errmsg);
     }

}


// ----------------------------------------------------------------------
// db_notifications() -- get a list of results in the DB that need to be
// reported, and formulate an email to the provided email address.  A 
// result is identified as worthy of notification if it is not flagged in
// the database as a false positive.  A flag is set for the result's
// record in the DB when it has been included in a notification at least
// once.
// ----------------------------------------------------------------------

void db_notifications(char *address)
{
     char **res;
     char *errmsg;
     int numrows;
     int numcols;
     int i;
     FILE *email;
     char hostname[1024];

     sqlite3_get_table(db, SQL_FIND_NEW, &res, &numrows, &numcols, &errmsg);
     
     if(numrows > 0)
     {
	  gethostname(hostname, 1024);
	  snprintf(buffer, 4096, "mail -s '%s %s' %s", subject, hostname, address);
	  email = popen(buffer, "w");
	  
	  fprintf(email, "SSNiper found strings that look like SSNs in the following files:\n\n");
	  for(i = 1; i <= numrows; i++)
	  {
	       fprintf(email, "%s\n", res[i]);
	       sqlite3_snprintf(4096, buffer, SQL_MARK_SENT, res[i]);
	       sqlite3_exec(db, buffer, 0, 0, 0);
	  }
	  
	  fprintf(email, "\nAs far as I know, my hostname is %s -- check the headers to confirm\n",
		  hostname);
	  pclose(email);
     }
}

// ----------------------------------------------------------------------
// db_review -- review the "positives" found in the database.  This will
// interactively allow the user to mark false positives so that subse-
// quent runs will not result in annoying emails.
// ----------------------------------------------------------------------

void db_review()
{
    char **res;
    char *errmsg;
    int numrows;
    int numcols;
    char input;
    int i;

    printf("DBG: Running query\n");
    sqlite3_get_table(db, SQL_FIND_NEW, &res, &numrows, &numcols, &errmsg);

    printf("DBG: Iterating through %d results\n", numrows);
    for(i=1; i<numrows; i++)
    {
	printf("\nPath:   %s\n", res[i]);
	printf("Action (f:false+, t:true+, s:skip): ");
	fflush(stdout);

	system("stty raw -echo");
	input = getchar();
	system("stty -raw echo");

	switch(input)
	{
	case 'f':
	    printf("Marking in DB...\n");
	    sqlite3_snprintf(4096, buffer, SQL_MARK_FALSE_P, res[i]);
	    sqlite3_exec(db, buffer, 0, 0, 0);
	    break;
	case 't':
	    printf("NOTE: deal with the file to remove it from results\n");
	    break;
	case 's': 
	    printf("Skipping...\n");
	    break;
	case 3:
	    return;
	default:
	    printf("Invalid response -- continuing...\n");
	}
    }
}

// ----------------------------------------------------------------------
// close_database() -- Kind of obvious.  Run it at the end of the
// program's execution.
// ----------------------------------------------------------------------

void close_database()
{
     sqlite3_close(db);
}
