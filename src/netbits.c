/* > netbits.c
 * Miscellaneous primitive network routines for Survex
 * Copyright (C) 1992-2001 Olly Betts
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if 0
# define DEBUG_INVALID 1
#endif

#include "debug.h"
#include "cavern.h"
#include "filename.h"
#include "message.h"
#include "netbits.h"
#include "datain.h" /* for compile_error */
#include "validate.h" /* for compile_error */

#define THRESHOLD (REAL_EPSILON * 1000) /* 100 was too small */

node *stn_iter = NULL; /* for FOR_EACH_STN */

#ifdef NO_COVARIANCES
static void check_var(/*const*/ var *v) {
   int bad = 0;
   int i;

   for (i = 0; i < 3; i++) {
      char buf[32];
      sprintf(buf, "%6.3f", v[i]);
      if (strstr(buf, "NaN") || strstr(buf, "nan"))
	 printf("*** NaN!!!\n"), bad = 1;
   }
   if (bad) print_var(v);
   return;
}
#else
#define V(A,B) ((*v)[A][B])
static void check_var(/*const*/ var *v) {
   int bad = 0;
   int ok = 0;
   int i, j;
#if DEBUG_INVALID
   real det = 0.0;
#endif

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
	 char buf[32];
	 sprintf(buf, "%6.3f", V(i, j));
	 if (strstr(buf, "NaN") || strstr(buf, "nan"))
	    printf("*** NaN!!!\n"), bad = 1, ok = 1;
	 if (V(i, j) != 0.0) ok = 1;
      }
   }
   if (!ok) return; /* ignore all-zero matrices */

#if DEBUG_INVALID
   for (i = 0; i < 3; i++) {
      det += V(i, 0) * (V((i + 1) % 3, 1) * V((i + 2) % 3, 2) -
			V((i + 1) % 3, 2) * V((i + 2) % 3, 1));
   }

   if (fabs(det) < THRESHOLD)
      printf("*** Singular!!!\n"), bad = 1;
#endif

#if 0
   /* don't check this - it isn't always the case! */
   if (fabs(V(0,1) - V(1,0)) > THRESHOLD ||
       fabs(V(0,2) - V(2,0)) > THRESHOLD ||
       fabs(V(1,2) - V(2,1)) > THRESHOLD)
      printf("*** Not symmetric!!!\n"), bad = 1;
   if (V(0,0) <= 0.0 || V(1,1) <= 0.0 || V(2,2) <= 0.0)
      printf("*** Not positive definite (diag <= 0)!!!\n"), bad = 1;
   if (sqrd(V(0,1)) >= V(0,0)*V(1,1) || sqrd(V(0,2)) >= V(0,0)*V(2,2) ||
       sqrd(V(1,0)) >= V(0,0)*V(1,1) || sqrd(V(2,0)) >= V(0,0)*V(2,2) ||
       sqrd(V(1,2)) >= V(2,2)*V(1,1) || sqrd(V(2,1)) >= V(2,2)*V(1,1))
      printf("*** Not positive definite (off diag^2 >= diag product)!!!\n"), bad = 1;
#endif
   if (bad) print_var(*v);
}

#define SN(V,A,B) ((*(V))[(A)==(B)?(A):2+(A)+(B)])
#define S(A,B) SN(v,A,B)

static void check_svar(/*const*/ svar *v) {
   int bad = 0;
   int ok = 0;
   int i;
#if DEBUG_INVALID
   real det = 0.0;
#endif

   for (i = 0; i < 6; i++) {
      char buf[32];
      sprintf(buf, "%6.3f", (*v)[i]);
      if (strstr(buf, "NaN") || strstr(buf, "nan"))
	 printf("*** NaN!!!\n"), bad = 1, ok = 1;
      if ((*v)[i] != 0.0) ok = 1;
   }
   if (!ok) return; /* ignore all-zero matrices */

#if DEBUG_INVALID
   for (i = 0; i < 3; i++) {
      det += S(i, 0) * (S((i + 1) % 3, 1) * S((i + 2) % 3, 2) -
			S((i + 1) % 3, 2) * S((i + 2) % 3, 1));
   }

   if (fabs(det) < THRESHOLD)
      printf("*** Singular!!!\n"), bad = 1;
#endif

#if 0
   /* don't check this - it isn't always the case! */
   if ((*v)[0] <= 0.0 || (*v)[1] <= 0.0 || (*v)[2] <= 0.0)
      printf("*** Not positive definite (diag <= 0)!!!\n"), bad = 1;
   if (sqrd((*v)[3]) >= (*v)[0]*(*v)[1] ||
       sqrd((*v)[4]) >= (*v)[0]*(*v)[2] ||
       sqrd((*v)[5]) >= (*v)[1]*(*v)[2])
      printf("*** Not positive definite (off diag^2 >= diag product)!!!\n"), bad = 1;
#endif
   if (bad) print_svar(*v);
}
#endif

static void check_d(/*const*/ delta *d) {
   int bad = 0;
   int i;

   for (i = 0; i < 3; i++) {
      char buf[32];
      sprintf(buf, "%6.3f", (*d)[i]);
      if (strstr(buf, "NaN") || strstr(buf, "nan"))
	 printf("*** NaN!!!\n"), bad = 1;
   }

   if (bad) printf("(%4.2f,%4.2f,%4.2f)\n", (*d)[0], (*d)[1], (*d)[2]);
}

/* insert at head of double-linked list */
void
add_stn_to_list(node **list, node *stn) {
   ASSERT(list);
   ASSERT(stn);
   ASSERT(stn_iter != stn); /* if it does, we're still on a list... */
#if 0
   printf("add_stn_to_list(%p, [%p] ", list, stn);
   if (stn->name) print_prefix(stn->name);
   printf(")\n");
#endif
   stn->next = *list;
   stn->prev = NULL;
   if (*list) (*list)->prev = stn;
   *list = stn;
}

/* remove from double-linked list */
void
remove_stn_from_list(node **list, node *stn) {
   ASSERT(list);
   ASSERT(stn);
#if 0
   printf("remove_stn_from_list(%p, [%p] ", list, stn);
   if (stn->name) print_prefix(stn->name);
   printf(")\n");
#endif
#if DEBUG_INVALID
     {
	/* check station is actually in this list */
	node *stn_to_remove_is_in_list = *list;
	validate();
	while (stn_to_remove_is_in_list != stn) {
	   ASSERT(stn_to_remove_is_in_list);
	   stn_to_remove_is_in_list = stn_to_remove_is_in_list->next;
	}
     }
#endif
   /* adjust the iterator if it points to the element we're deleting */
   if (stn_iter == stn) stn_iter = stn_iter->next;
   /* need a special case if we're removing the list head */
   if (stn->prev == NULL) {
      *list = stn->next;
      if (*list) (*list)->prev = NULL;
   } else {
      stn->prev->next = stn->next;
      if (stn->next) stn->next->prev = stn->prev;
   }
}

/* Create (uses osmalloc) a forward leg containing the data in leg, or
 * the reversed data in the reverse of leg, if leg doesn't hold data
 */
linkfor *
copy_link(linkfor *leg)
{
   linkfor *legOut;
   int d;
   legOut = osnew(linkfor);
   if (data_here(leg)) {
      for (d = 2; d >= 0; d--) legOut->d[d] = leg->d[d];
   } else {
      leg = reverse_leg(leg);
      ASSERT(data_here(leg));
      for (d = 2; d >= 0; d--) legOut->d[d] = -leg->d[d];
   }
#if 1
# ifndef NO_COVARIANCES
   check_svar(&(leg->v));
     {
	int i;
	for (i = 0; i < 6; i++) legOut->v[i] = leg->v[i];
     }
# else
   for (d = 2; d >= 0; d--) legOut->v[d] = leg->v[d];
# endif
#else
   memcpy(legOut->v, leg->v, sizeof(svar));
#endif
   return legOut;
}

/* Adds to the forward leg `leg', the data in leg2, or the reversed data
 * in the reverse of leg2, if leg2 doesn't hold data
 */
linkfor *
addto_link(linkfor *leg, const linkfor *leg2)
{
   if (data_here(leg2)) {
      adddd(&leg->d, &leg->d, &((linkfor *)leg2)->d);
   } else {
      leg2 = reverse_leg(leg2);
      ASSERT(data_here(leg2));
      subdd(&leg->d, &leg->d, &((linkfor *)leg2)->d);
   }
   addss(&leg->v, &leg->v, &((linkfor *)leg2)->v);
   return leg;
}

static void
addleg_(node *fr, node *to,
	real dx, real dy, real dz,
	real vx, real vy, real vz,
#ifndef NO_COVARIANCES
	real cyz, real czx, real cxy,
#endif
	int leg_flags)
{
   int i, j;
   linkfor *leg, *leg2;
   int shape;
   /* we have been asked to add a leg with the same node at both ends
    * - this should be trapped by the caller */
   ASSERT(fr->name != to->name);

   leg = osnew(linkfor);
   leg2 = (linkfor*)osnew(linkrev);

   i = freeleg(&fr);
   j = freeleg(&to);

   leg->l.to = to;
   leg2->l.to = fr;
   leg->d[0] = dx;
   leg->d[1] = dy;
   leg->d[2] = dz;
#ifndef NO_COVARIANCES
   leg->v[0] = vx;
   leg->v[1] = vy;
   leg->v[2] = vz;
   leg->v[3] = cxy;
   leg->v[4] = czx;
   leg->v[5] = cyz;
   check_svar(&(leg->v));
#else
   leg->v[0] = vx;
   leg->v[1] = vy;
   leg->v[2] = vz;
#endif
   leg2->l.reverse = i;
   leg->l.reverse = j | FLAG_DATAHERE | leg_flags;

   leg->l.flags = pcs->flags;

   fr->leg[i] = leg;
   to->leg[j] = leg2;

   shape = fr->name->shape + 1;
   if (shape < 1) shape = 1 - shape;
   fr->name->shape = shape;

   shape = to->name->shape + 1;
   if (shape < 1) shape = 1 - shape;
   to->name->shape = shape;
}

/* Add a leg between names *fr_name and *to_name
 * If either is a three node, then it is split into two
 * and the data structure adjusted as necessary.
 */
void
addlegbyname(prefix *fr_name, prefix *to_name, bool fToFirst,
	     real dx, real dy, real dz,
	     real vx, real vy, real vz
#ifndef NO_COVARIANCES
	     , real cyz, real czx, real cxy
#endif
	     )
{
   node *to, *fr;
   if (to_name == fr_name) {
      compile_error(/*Survey leg with same station (`%s') at both ends - typing error?*/50,
		    sprint_prefix(to_name));
      return;
   }   
   if (fToFirst) {
      to = StnFromPfx(to_name);
      fr = StnFromPfx(fr_name);
   } else {
      fr = StnFromPfx(fr_name);
      to = StnFromPfx(to_name);
   }
   cLegs++; /* increment count (first as compiler may do tail recursion) */
   addleg_(fr, to, dx, dy, dz, vx, vy, vz,
#ifndef NO_COVARIANCES
	   cyz, czx, cxy,
#endif
	   0);
}

/* helper function for replace_pfx */
static void
replace_pfx_(node *stn, node *from, pos *pos_replace, pos *pos_with)
{
   int d;
   stn->name->pos = pos_with;
   for (d = 0; d < 3; d++) {
      linkfor *leg = stn->leg[d];
      node *to;
      if (!leg) break;
      to = leg->l.to;
      if (to == from) continue;

      if (fZeros(data_here(leg) ? &leg->v : &reverse_leg(leg)->v))
	 replace_pfx_(to, stn, pos_replace, pos_with);
   }
}

/* We used to iterate over the whole station list (inefficient) - now we
 * just look at any neighbouring nodes to see if they are equated */
static void
replace_pfx(const prefix *pfx_replace, const prefix *pfx_with)
{
   pos *pos_replace;
   ASSERT(pfx_replace);
   ASSERT(pfx_with);
   pos_replace = pfx_replace->pos;
   ASSERT(pos_replace != pfx_with->pos);

   replace_pfx_(pfx_replace->stn, NULL, pos_replace, pfx_with->pos);

#if DEBUG_INVALID
   {
      node *stn;
      FOR_EACH_STN(stn, stnlist) {
	 ASSERT(stn->name->pos != pos_replace);
      }
   }
#endif

   /* free the (now-unused) old pos */
   osfree(pos_replace);
}

/* Add an equating leg between existing stations *fr and *to (whose names are
 * name1 and name2).
 */
void
process_equate(prefix *name1, prefix *name2)
{
   if (name1 == name2) {
      /* catch something like *equate "fred fred" */
      compile_warning(/*Station `%s' equated to itself*/13,
		      sprint_prefix(name1));
      return;
   }

   /* equate nodes if not already equated */
   if (name1->pos != name2->pos) {
      node *stn1, *stn2;
      stn1 = StnFromPfx(name1);
      stn2 = StnFromPfx(name2);

      if (pfx_fixed(name1)) {
	 if (pfx_fixed(name2)) {
	    /* both are fixed, but let them off iff their coordinates match */
	    char *s = osstrdup(sprint_prefix(name1));
	    int d;
	    for (d = 2; d >= 0; d--) {
	       if (name1->pos->p[d] != name2->pos->p[d]) {
		  compile_error(/*Tried to equate two non-equal fixed stations: `%s' and `%s'*/52,
				s, sprint_prefix(name2));
		  osfree(s);
		  return;
	       }
	    }
	    compile_warning(/*Equating two equal fixed points: `%s' and `%s'*/53,
			    s, sprint_prefix(name2));
	    osfree(s);
	 }
	 
	 /* name1 is fixed, so replace all refs to name2's pos with name1's */
	 replace_pfx(name2, name1);
      } else {
	 /* name1 isn't fixed, so replace all refs to its pos with name2's */
	 replace_pfx(name1, name2);
      }

      /* count equates as legs for now... */
      cLegs++;
      addleg_(stn1, stn2,
	      (real)0.0, (real)0.0, (real)0.0,
	      (real)0.0, (real)0.0, (real)0.0,
#ifndef NO_COVARIANCES
	      (real)0.0, (real)0.0, (real)0.0,
#endif
	      FLAG_FAKE);
   }
}

/* Add a 'fake' leg (not counted) between existing stations *fr and *to
 * (which *must* be different)
 * If either node is a three node, then it is split into two
 * and the data structure adjusted as necessary
 */
void
addfakeleg(node *fr, node *to,
	   real dx, real dy, real dz,
	   real vx, real vy, real vz
#ifndef NO_COVARIANCES
	   , real cyz, real czx, real cxy
#endif
	   )
{
   addleg_(fr, to, dx, dy, dz, vx, vy, vz,
#ifndef NO_COVARIANCES
	   cyz, czx, cxy,
#endif
	   FLAG_FAKE);
}

char
freeleg(node **stnptr)
{
   node *stn, *oldstn;
   linkfor *leg, *leg2;
#ifndef NO_COVARIANCES
   int i;
#endif

   stn = *stnptr;

   if (stn->leg[0] == NULL) return 0; /* leg[0] unused */
   if (stn->leg[1] == NULL) return 1; /* leg[1] unused */
   if (stn->leg[2] == NULL) return 2; /* leg[2] unused */

   /* All legs used, so split node in two */
   oldstn = stn;
   stn = osnew(node);
   leg = osnew(linkfor);
   leg2 = (linkfor*)osnew(linkrev);

   *stnptr = stn;

   add_stn_to_list(&stnlist, stn);
   stn->name = oldstn->name;

   leg->l.to = stn;
   leg->d[0] = leg->d[1] = leg->d[2] = (real)0.0;

#ifndef NO_COVARIANCES
   for (i = 0; i < 6; i++) leg->v[i] = (real)0.0;
#else
   leg->v[0] = leg->v[1] = leg->v[2] = (real)0.0;
#endif
   leg->l.reverse = 1 | FLAG_DATAHERE | FLAG_FAKE;
   leg->l.flags = pcs->flags;

   leg2->l.to = oldstn;
   leg2->l.reverse = 0;

   stn->leg[0] = oldstn->leg[0];
   /* correct reverse leg */
   reverse_leg(stn->leg[0])->l.to = stn;
   stn->leg[1] = leg2;

   oldstn->leg[0] = leg;

   stn->leg[2] = NULL; /* needed as stn->leg[dirn]==NULL indicates unused */

   return(2); /* leg[2] unused */
}

node *
StnFromPfx(prefix *name)
{
   node *stn;
   if (name->stn != NULL) return (name->stn);
   stn = osnew(node);
   stn->name = name;
   if (name->pos == NULL) {
      name->pos = osnew(pos);
#ifdef NEW3DFORMAT
      name->pos->id = 0;
#endif
      unfix(stn);
   }
   stn->leg[0] = stn->leg[1] = stn->leg[2] = NULL;
   add_stn_to_list(&stnlist, stn);
   name->stn = stn;
   cStns++;
   return stn;
}

extern void
fprint_prefix(FILE *fh, const prefix *ptr)
{
   ASSERT(ptr);
   if (ptr->up != NULL) {
      fprint_prefix(fh, ptr->up);
      if (ptr->up->up != NULL) fputc('.', fh);
      fputs(ptr->ident, fh);
   }
}

static char *buffer = NULL;
static OSSIZE_T buffer_len = 256;

static OSSIZE_T
sprint_prefix_(const prefix *ptr)
{
   OSSIZE_T len = 1;
   if (ptr->up != NULL) {
      len = sprint_prefix_(ptr->up) + strlen(ptr->ident);
      if (ptr->up->up != NULL) len++;
      if (len > buffer_len) {
	 buffer = osrealloc(buffer, len);
	 buffer_len = len;
      }
      if (ptr->up->up != NULL) strcat(buffer, ".");
      strcat(buffer, ptr->ident);
   }
   return len;
}

extern char *
sprint_prefix(const prefix *ptr)
{
   ASSERT(ptr);
   if (!buffer) buffer = osmalloc(buffer_len);
   *buffer = '\0';
   sprint_prefix_(ptr);
   return buffer;
}

/* r = ab ; r,a,b are variance matrices */
void
mulvv(var *r, /*const*/ var *a, /*const*/ var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * (*b)[0];
   (*r)[1] = (*a)[1] * (*b)[1];
   (*r)[2] = (*a)[2] * (*b)[2];
#else
   int i, j, k;
   real tot;

   ASSERT((/*const*/ var *)r != a);
   ASSERT((/*const*/ var *)r != b);

   check_var(a);
   check_var(b);

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
	 tot = 0;
	 for (k = 0; k < 3; k++) {
	    tot += (*a)[i][k] * (*b)[k][j];
	 }
	 (*r)[i][j] = tot;
      }
   }
   check_var(r);
#endif
}

/* r = ab ; r,a,b are variance matrices */
void
mulss(var *r, /*const*/ svar *a, /*const*/ svar *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * (*b)[0];
   (*r)[1] = (*a)[1] * (*b)[1];
   (*r)[2] = (*a)[2] * (*b)[2];
#else
   int i, j, k;
   real tot;

#if 0
   ASSERT((/*const*/ var *)r != a);
   ASSERT((/*const*/ var *)r != b);
#endif

   check_svar(a);
   check_svar(b);

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
	 tot = 0;
	 for (k = 0; k < 3; k++) {
	    tot += SN(a,i,k) * SN(b,k,j);
	 }
	 (*r)[i][j] = tot;
      }
   }
   check_var(r);
#endif
}

/* r = ab ; r,a,b are variance matrices */
void
smulvs(svar *r, /*const*/ var *a, /*const*/ svar *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * (*b)[0];
   (*r)[1] = (*a)[1] * (*b)[1];
   (*r)[2] = (*a)[2] * (*b)[2];
#else
   int i, j, k;
   real tot;

#if 0
   ASSERT((/*const*/ var *)r != a);
#endif
   ASSERT((/*const*/ svar *)r != b);

   check_var(a);
   check_svar(b);

   (*r)[3]=(*r)[4]=(*r)[5]=-999;
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
	 tot = 0;
	 for (k = 0; k < 3; k++) {
	    tot += (*a)[i][k] * SN(b,k,j);
	 }
	 if (i <= j)
	    SN(r,i,j) = tot;
	 else if (fabs(SN(r,j,i) - tot) > THRESHOLD) {
	    printf("not sym - %d,%d = %f, %d,%d was %f\n",
		   i,j,tot,j,i,SN(r,j,i));
	    BUG("smulvs didn't produce a sym mx\n");
	 }
      }
   }
   check_svar(r);
#endif
}

/* r = ab ; r,b delta vectors; a variance matrix */
void
mulvd(delta *r, /*const*/ var *a, /*const*/ delta *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * (*b)[0];
   (*r)[1] = (*a)[1] * (*b)[1];
   (*r)[2] = (*a)[2] * (*b)[2];
#else
   int i, k;
   real tot;

   ASSERT((/*const*/ delta*)r != b);
   check_var(a);
   check_d(b);

   for (i = 0; i < 3; i++) {
      tot = 0;
      for (k = 0; k < 3; k++) tot += (*a)[i][k] * (*b)[k];
      (*r)[i] = tot;
   }
   check_d(r);
#endif
}

/* r = vb ; r,b delta vectors; a variance matrix */
void
mulsd(delta *r, /*const*/ svar *v, /*const*/ delta *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*v)[0] * (*b)[0];
   (*r)[1] = (*v)[1] * (*b)[1];
   (*r)[2] = (*v)[2] * (*b)[2];
#else
   int i, j;
   real tot;

   ASSERT((/*const*/ delta*)r != b);
   check_svar(v);
   check_d(b);

   for (i = 0; i < 3; i++) {
      tot = 0;
      for (j = 0; j < 3; j++) tot += S(i,j) * (*b)[j];
      (*r)[i] = tot;
   }
   check_d(r);
#endif
}

/* r = ca ; r,a delta vectors; c real scaling factor  */
void
muldc(delta *r, /*const*/ delta *a, real c) {
   check_d(a);
   (*r)[0] = (*a)[0] * c;
   (*r)[1] = (*a)[1] * c;
   (*r)[2] = (*a)[2] * c;
   check_d(r);
}

/* r = ca ; r,a variance matrices; c real scaling factor  */
void
mulvc(var *r, /*const*/ var *a, real c)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * c;
   (*r)[1] = (*a)[1] * c;
   (*r)[2] = (*a)[2] * c;
#else
   int i, j;

   check_var(a);
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) (*r)[i][j] = (*a)[i][j] * c;
   }
   check_var(r);
#endif
}

/* r = ca ; r,a variance matrices; c real scaling factor  */
void
mulsc(svar *r, /*const*/ svar *a, real c)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] * c;
   (*r)[1] = (*a)[1] * c;
   (*r)[2] = (*a)[2] * c;
#else
   int i;

   check_svar(a);
   for (i = 0; i < 6; i++) (*r)[i] = (*a)[i] * c;
   check_svar(r);
#endif
}

/* r = a + b ; r,a,b delta vectors */
void
adddd(delta *r, /*const*/ delta *a, /*const*/ delta *b)
{
   check_d(a);
   check_d(b);
   (*r)[0] = (*a)[0] + (*b)[0];
   (*r)[1] = (*a)[1] + (*b)[1];
   (*r)[2] = (*a)[2] + (*b)[2];
   check_d(r);
}

/* r = a - b ; r,a,b delta vectors */
void
subdd(delta *r, /*const*/ delta *a, /*const*/ delta *b) {
   check_d(a);
   check_d(b);
   (*r)[0] = (*a)[0] - (*b)[0];
   (*r)[1] = (*a)[1] - (*b)[1];
   (*r)[2] = (*a)[2] - (*b)[2];
   check_d(r);
}

/* r = a + b ; r,a,b variance matrices */
void
addvv(var *r, /*const*/ var *a, /*const*/ var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] + (*b)[0];
   (*r)[1] = (*a)[1] + (*b)[1];
   (*r)[2] = (*a)[2] + (*b)[2];
#else
   int i, j;

   check_var(a);
   check_var(b);
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) (*r)[i][j] = (*a)[i][j] + (*b)[i][j];
   }
   check_var(r);
#endif
}

/* r = a + b ; r,a,b variance matrices */
void
addss(svar *r, /*const*/ svar *a, /*const*/ svar *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] + (*b)[0];
   (*r)[1] = (*a)[1] + (*b)[1];
   (*r)[2] = (*a)[2] + (*b)[2];
#else
   int i;

   check_svar(a);
   check_svar(b);
   for (i = 0; i < 6; i++) (*r)[i] = (*a)[i] + (*b)[i];
   check_svar(r);
#endif
}

/* r = a - b ; r,a,b variance matrices */
void
subvv(var *r, /*const*/ var *a, /*const*/ var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] - (*b)[0];
   (*r)[1] = (*a)[1] - (*b)[1];
   (*r)[2] = (*a)[2] - (*b)[2];
#else
   int i, j;

   check_var(a);
   check_var(b);
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) (*r)[i][j] = (*a)[i][j] - (*b)[i][j];
   }
   check_var(r);
#endif
}

/* r = a - b ; r,a,b variance matrices */
void
subss(svar *r, /*const*/ svar *a, /*const*/ svar *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] - (*b)[0];
   (*r)[1] = (*a)[1] - (*b)[1];
   (*r)[2] = (*a)[2] - (*b)[2];
#else
   int i;

   check_svar(a);
   check_svar(b);
   for (i = 0; i < 6; i++) (*r)[i] = (*a)[i] - (*b)[i];
   check_svar(r);
#endif
}

/* inv = v^-1 ; inv,v variance matrices */
#ifdef NO_COVARIANCES
extern int
invert_var(var *inv, /*const*/ var *v)
{
   int i;
   for (i = 0; i < 3; i++) {
      if ((*v)[i] < THRESHOLD) return 0; /* matrix is singular */
      (*inv)[i] = 1.0 / (*v)[i];
   }
   return 1;
}
#else
extern int
invert_var(var *inv, /*const*/ var *v)
{
   int i, j;
   real det = 0;

   ASSERT((/*const*/ var *)inv != v);

   check_var(v);
   for (i = 0; i < 3; i++) {
      det += V(i, 0) * (V((i + 1) % 3, 1) * V((i + 2) % 3, 2) -
			V((i + 1) % 3, 2) * V((i + 2) % 3, 1));
   }

   if (fabs(det) < THRESHOLD) {
      /* printf("det=%.20f\n", det); */
      return 0; /* matrix is singular */
   }

   det = 1 / det;

#define B(I,J) ((*v)[(J)%3][(I)%3])
   for (i = 0; i < 3; i++) {
      for (j = 0; j < 3; j++) {
         (*inv)[i][j] = det * (B(i+1,j+1)*B(i+2,j+2) - B(i+2,j+1)*B(i+1,j+2));
      }
   }
#undef B

     { /* check that original * inverse = identity matrix */
	var p;
	real d = 0;
	mulvv(&p, v, inv);
	for (i = 0; i < 3; i++) {
	   for (j = 0; j < 3; j++) d += fabs(p[i][j] - (real)(i==j));
	}
	if (d > THRESHOLD) {
	   printf("original * inverse=\n");
	   print_var(*v);
	   printf("*\n");
	   print_var(*inv);
	   printf("=\n");
	   print_var(p);
	   BUG("matrix didn't invert");
	}
	check_var(inv);
     }

   return 1;
}
#endif

/* inv = v^-1 ; inv,v variance matrices */
#ifndef NO_COVARIANCES
extern int
invert_svar(svar *inv, /*const*/ svar *v)
{
   int i;
   real det, a, b, c, d, e, f, bcff, efcd, dfbe;

#if 0
   ASSERT((/*const*/ var *)inv != v);
#endif

   check_svar(v);
   /* a d e
    * d b f
    * e f c
    */
   a = (*v)[0], b = (*v)[1], c = (*v)[2];
   d = (*v)[3], e = (*v)[4], f = (*v)[5];
   bcff = b * c - f * f;
   efcd = e * f - c * d;
   dfbe = d * f - b * e;
   det = a * bcff + d * efcd + e * dfbe;

   if (fabs(det) < THRESHOLD) {
      /* printf("det=%.20f\n", det); */
      return 0; /* matrix is singular */
   }

   det = 1 / det;

   (*inv)[0] = det * bcff;
   (*inv)[1] = det * (c * a - e * e);
   (*inv)[2] = det * (a * b - d * d);
   (*inv)[3] = det * efcd;
   (*inv)[4] = det * dfbe;
   (*inv)[5] = det * (e * d - a * f);

     { /* check that original * inverse = identity matrix */
	var p;
	real D = 0;
	mulss(&p, v, inv);
	for (i = 0; i < 3; i++) {
	   int j;
	   for (j = 0; j < 3; j++) D += fabs(p[i][j] - (real)(i==j));
	}
	if (D > THRESHOLD) {
	   printf("original * inverse=\n");
	   print_svar(*v);
	   printf("*\n");
	   print_svar(*inv);
	   printf("=\n");
	   print_var(p);
	   BUG("matrix didn't invert");
	}
	check_svar(inv);
     }
   return 1;
}
#endif

/* r = (b^-1)a ; r,a delta vectors; b variance matrix */
void
divdv(delta *r, /*const*/ delta *a, /*const*/ var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] / (*b)[0];
   (*r)[1] = (*a)[1] / (*b)[1];
   (*r)[2] = (*a)[2] / (*b)[2];
#else
   var b_inv;
   if (!invert_var(&b_inv, b)) {
      print_var(*b);
      BUG("covariance matrix is singular");
   }
   mulvd(r, &b_inv, a);
#endif
}

/* r = (b^-1)a ; r,a delta vectors; b variance matrix */
#ifndef NO_COVARIANCES
void
divds(delta *r, /*const*/ delta *a, /*const*/ svar *b)
{
   svar b_inv;
   if (!invert_svar(&b_inv, b)) {
      print_svar(*b);
      BUG("covariance matrix is singular");
   }
   mulsd(r, &b_inv, a);
}
#endif

/* f = a(b^-1) ; r,a,b variance matrices */
void
divvv(var *r, /*const*/ var *a, /*const*/ var *b)
{
#ifdef NO_COVARIANCES
   /* variance-only version */
   (*r)[0] = (*a)[0] / (*b)[0];
   (*r)[1] = (*a)[1] / (*b)[1];
   (*r)[2] = (*a)[2] / (*b)[2];
#else
   var b_inv;
   check_var(a);
   check_var(b);
   if (!invert_var(&b_inv, b)) {
      print_var(*b);
      BUG("covariance matrix is singular");
   }
   mulvv(r, a, &b_inv);
   check_var(r);
#endif
}

bool
fZeros(/*const*/ svar *v) {
#ifdef NO_COVARIANCES
   /* variance-only version */
   return ((*v)[0] == 0.0 && (*v)[1] == 0.0 && (*v)[2] == 0.0);
#else
   int i;

   check_svar(v);
   for (i = 0; i < 6; i++) if ((*v)[i] != 0.0) return fFalse;

   return fTrue;
#endif
}
