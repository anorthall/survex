/* > diffpos.c */
/* (Originally quick and dirty) program to compare two SURVEX .pos files */
/* Copyright (C) 1994,1996,1998,1999 Olly Betts */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* size of line buffer */
#define BUFLEN 256

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define sqrd(X) ((X)*(X))

/* macro to just convert argument to a string */
#define STRING(X) _STRING(X)
#define _STRING(X) #X

/* very small value for comparing floating point numbers with */
/* (ought to use a real epsilon value) */
#define EPSILON 0.00001

/* default threshold is 1cm */
#define DFLT_MAX_THRESHOLD 0.01

static int diff_pos(FILE *fh1, FILE *fh2, double threshold);
static int read_line(FILE *fh, double *px, double *py, double *pz, char *id);

int
main(int argc, char *argv[])
{
   double threshold = DFLT_MAX_THRESHOLD;
   char *fnm1, *fnm2;
   FILE *fh1, *fh2;
   if (argc != 3) {
      char *p;
      if (argc == 4) threshold = strtod(argv[3], &p);
      if (argc != 4 || *p) {
	 /* FIXME put these in the messages file */
         /* complain if not 4 args, or threshold didn't parse cleanly */
         printf("Syntax: %s <pos file> <pos file> [<threshold>]\n", argv[0]);
         printf(" where <threshold> is the max. permitted change along "
                "any axis in metres\n"
	        " (default <threshold> is "STRING(DFLT_MAX_THRESHOLD)"m)\n");
         exit(1);
      }
   }
   fnm1 = argv[1];
   fnm2 = argv[2];
   fh1 = fopen(fnm1, "rb");
   if (!fh1) {
      printf("Can't open file '%s'\n", fnm1);
      exit(1);
   }
   fh2 = fopen(fnm2, "rb");
   if (!fh2) {
      printf("Can't open file '%s'\n", fnm2);
      exit(1);
   }
   return diff_pos(fh1, fh2, threshold);
}

typedef enum { Eof, Haveline, Needline } state;
   
int
diff_pos(FILE *fh1, FILE *fh2, double threshold)
{
   state pos1 = Needline, pos2 = Needline;
   int result = 0;
   
   while (1) {
      double x1, y1, z1, x2, y2, z2;
      char id1[BUFLEN], id2[BUFLEN];
      
      if (pos1 == Needline) {
	 pos1 = Haveline;
         if (!read_line(fh1, &x1, &y1, &z1, id1)) pos1 = Eof;
      }
      
      if (pos2 == Needline) {
	 pos2 = Haveline;
         if (!read_line(fh2, &x2, &y2, &z2, id2)) pos2 = Eof;
      }
      
      if (pos1 == Eof) {
	 if (pos2 == Eof) break;
	 result = 1;
	 printf("Added: %s (at end of file)\n", id2);
	 pos2 = Needline;	 
      } else if (pos2 == Eof) {
	 result = 1;
	 printf("Deleted: %s (at end of file)\n", id1);
	 pos1 = Needline;
      } else {
	 int cmp = strcmp(id1, id2);
	 if (cmp == 0) {
	    if (fabs(x1 - x2) - threshold > EPSILON ||
		fabs(y1 - y2) - threshold > EPSILON ||
		fabs(z1 - z2) - threshold > EPSILON) {
	       result = 1;
	       printf("Moved by (%3.2f,%3.2f,%3.2f): %s\n",
		      x1 - x2, y1 - y2, z1 - z2, id1);
	    }
	    pos1 = pos2 = Needline;
	 } else {
	    result = 1;
	    if (cmp < 0) {
	       printf("Deleted: %s\n", id1);
	       pos1 = Needline;
	    } else {
	       printf("Added: %s\n", id2);
	       pos2 = Needline;
	    }
	 }
      }
   }
   return result;
}

static int
read_line(FILE *fh, double *px, double *py, double *pz, char *id)
{
   char buf[BUFLEN];
   while (1) {
      if (!fgets(buf, BUFLEN, fh)) return 0;
      if (sscanf(buf, "(%lf,%lf,%lf )%s", px, py, pz, id) == 4) break;
      printf("Ignoring line: %s\n", buf);
   }
   return 1;
}
