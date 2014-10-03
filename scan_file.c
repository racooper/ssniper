/*

Copyright © 2007, The Board of Trustees of the University of Illinois.

This file is part of SSNiper.  

SSNiper is open source software, released under the University of
Illinois/NCSA Open Source License.  You should have received a copy of
this license in a file with the SSNiper distribution.

*/

#include "scan_file.h"

extern char LOGBUF[LOG_BUF_SZ];
extern long long total_bytes;
extern time_t timer;

#ifndef DISABLE_BZIP
FILE * BZ_infile = NULL;
#endif

int fsm_initialized = 0;
long long file_byte_cap = 0;

long long byte_tracker = 0;

// ----------------------------------------------------------------------
// open_file() -- this facilitates functionally abstracting the scan_file
// function.  This lets us open a file "generically".  It's done based on
// the provided "type" (defined in scan_file.h), and passed around as 
// void pointers.  Anything that uses pointers for handles can be crammed
// in here thusly.
// ----------------------------------------------------------------------

void * open_file(char * filename, int type)
{
     #ifndef DISABLE_BZIP
     BZFILE *bzfile;
     #endif

     switch(type)
     {
     case SF_STANDARD:
	  return (void *)(open_read(filename));

     #ifndef DISABLE_ZLIB
     case SF_GZIP:
	  return (void *)(gzopen(filename, "rb"));
     #endif

     #ifndef DISABLE_BZIP
     case SF_BZIP:
	  return open_bzip_file(filename);
     #endif
     }
     return NULL;
}


// ----------------------------------------------------------------------
// open_file() -- this facilitates functionally abstracting the scan_file
// function.  This function will read a block from the handle (void *
// cast based on the type var) of length len into buffer.
// ----------------------------------------------------------------------

int read_block(void * handle, char * buffer, int len, int type)
{
     int bzerror;

     switch(type)
     {
     case SF_STANDARD:
	  return fread(buffer, sizeof(char), len, (FILE *)handle);

     #ifndef DISABLE_ZLIB
     case SF_GZIP:
	  return gzread((gzFile)handle, buffer, len);
     #endif

     #ifndef DISABLE_BZIP
     case SF_BZIP:
	  return BZ2_bzRead(&bzerror, (BZFILE *)handle, buffer, len);
     #endif
     }
}


// ----------------------------------------------------------------------
// seek_file() -- this facilitates functionally abstracting the scan_file
// function.  Where possible, we like to seek back on these files in case
// SSNs straddle block boundaries.
// ----------------------------------------------------------------------

void seek_file(void * handle, int delta, int type)
{
     switch(type)
     {
     case SF_STANDARD:
	  fseek((FILE *)handle, delta, SEEK_CUR);
	  break;
     }
     return;
}


// ----------------------------------------------------------------------
// seek_file() -- this facilitates functionally abstracting the scan_file
// function.  This will close the file handle provided (cast based on the
// value of the 'type' variable).
// ----------------------------------------------------------------------

void close_file(void * handle, int type)
{
     int bzerror;

     switch(type)
     {
     case SF_STANDARD:
	  fclose((FILE *)handle);
	  break;

     #ifndef DISABLE_ZLIB
     case SF_GZIP:
	  gzclose((gzFile)handle);
	  break;
     #endif

     #ifndef DISABLE_BZIP
     case SF_BZIP:
	  BZ2_bzReadClose(&bzerror, (BZFILE *)handle);
	  fclose(BZ_infile);
	  break;
     #endif
     }
}


// ----------------------------------------------------------------------
// report_progress() -- Periodically, this function will print a message
// to the user and the info log about how the scan is going.  This is
// summary information so that people know something's happening.
// ----------------------------------------------------------------------

void report_progress(struct ssn_results results)
{
     char    printbuf[1024];
     time_t  local_timer;
     int     rate;

     local_timer = time(NULL);

     if(local_timer > (timer + NOTE_INTERVAL))
     {
	  rate = (int)((double)(total_bytes - byte_tracker) / 
		       (double)(local_timer - timer) / 1024);
	  
	  snprintf(printbuf, 1024, "Progress: % 7d files, % 12lld bytes (% 6d kB/s)", 
		   results.scanned, total_bytes, rate);
	  print_timestamp(printbuf);
	  byte_tracker = total_bytes;
	  timer = local_timer;
     }
}


// ----------------------------------------------------------------------
// scan_file() -- this is a big, messy function that takes a filename
// in the current directory, opens it, reads it block by block, tallying
// SSNs discovered.
// ----------------------------------------------------------------------

struct ssn_results scan_file(char *filename, int type, struct ssn_results results)
{
     void               *infile = NULL;
     int                 done = 0, x=0, ratio, i;
     struct ssn_results  temp_results;
     char               *cwd;

     report_progress(results);

     blank_results(&temp_results);

     if(!fsm_initialized) fsm_initialized = 1;
     else fsm_free_machine();

     // fsm is in ssn_fsm.c, and is a finite state machine for scanning
     // blocks for SSNs.  It must be initialized for every file.

     fsm_initialize_machine();
     infile = open_file(filename, type);

     if(infile != NULL)
     {
	  while(!done)
	  {
	       done = scan_block(infile, type, &temp_results);
	       if(file_byte_cap && temp_results.bytes >= file_byte_cap)
	       {
		    cwd = get_cwd();
		    snprintf(LOGBUF, LOG_BUF_SZ, 
			     "skipped %s/%s -- byte cap reached (%lld/%lld)", 
			     cwd, filename, temp_results.bytes, file_byte_cap);
		    slog(LOGBUF, LOG_DEB);
		    free(cwd);
		    break;
	       }
	  }

	  switch(done)
	  {
	  case 2:
	       cwd = get_cwd();
	       snprintf(LOGBUF, LOG_BUF_SZ, "Error reading block on file %s/%s", 
		      cwd, filename);
	       slog(LOGBUF, LOG_DEB);
	       free(cwd);
	       break;
	  }

	  close_file(infile, type);

	  if(temp_results.positives > 0)
	  {
	       show_result(filename, temp_results);
	       update_summary_stats(temp_results, &results);
	  }
     }
     return results;
}


// ----------------------------------------------------------------------
// scan_block() -- read in a block from the provided file handle and 
// scan it.  The file type must be passed as well, as it's needed by
// the abstracted "read_block" function.  *results is there so we can
// record SSN hits for the calling function.
// ----------------------------------------------------------------------

int scan_block(FILE *infile, int type, struct ssn_results * results)
{
     char                buffer[BUFFLEN];
     int bytes_read, done = 0;

     switch(bytes_read = read_block(infile, buffer, BUFFLEN, type))
     {
     case -1:
	  printf("Error: %d\n", errno);
	  printf("  Message: %s\n", strerror(errno));
	  done = 2;
	  break;
     case 0:
	  done = 1;
	  break;
     default:
	  *results = fsm_match_buffer(buffer, bytes_read, *results);
	  results->bytes += bytes_read;
	  total_bytes += bytes_read;
	  if(bytes_read == BUFFLEN)
	  {
	       seek_file(infile, BUFFOFF, type);
	  }
     }
     return done;
}


// ----------------------------------------------------------------------
// open_read() -- open a file with read access.  I just put it here
// because the error checking is ugly and distracts me from surrounding
// code (a normal problem with C, as far as I'm concerned).
// ----------------------------------------------------------------------

FILE * open_read(char *filename)
{
     FILE *infile;
     infile = fopen(filename, "r");
     if((void *)infile == NULL)
     {
	  // if I had anything to do on error, this is where it
	  // would go!
     }

     return infile;
}


// ----------------------------------------------------------------------
// update_summary_stats() -- We keep running totals for final reporting.
// this function will take SSN scan results from an individual file,
// and update the "global" SSN results struct (provided by caller).
// ----------------------------------------------------------------------

void update_summary_stats(struct ssn_results spec, struct ssn_results *gen)
{
     if(spec.positives) gen->positives++;
     if(spec.falses)    gen->falses++;
     if(spec.delim)     gen->delim++;
     if(spec.nondelim)  gen->nondelim++;
}


// ----------------------------------------------------------------------
// open_bzip_file() -- open a bzip file and return the appropriate
// handle.  There is a global variable for holding the necessary "slush"
// filehandle.  This is an unfortunate side effect... node this makes us
// not thread safe!
// ----------------------------------------------------------------------

#ifndef DISABLE_BZIP
void * open_bzip_file(char *filename)
{
     int bzerror;
     BZFILE *bzfile;

     BZ_infile = open_read(filename);
     if(BZ_infile)
     {
	  bzfile = BZ2_bzReadOpen(&bzerror, BZ_infile, 0, 0, (void *)NULL, 0);
	  if(bzerror != BZ_OK)
	  {
	       fclose(BZ_infile);
	  }
	  else
	  {
	       return (void *)bzfile;
	  }
     }
     return NULL;
}
#endif

