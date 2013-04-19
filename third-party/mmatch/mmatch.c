/*
 * IRC - Internet Relay Chat, common/match.c
 * Copyright (C) 1990 Jarkko Oikarinen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: Match.cpp,v 1.2 2005/08/15 10:08:50 shroud23 Exp $
 */

#include <ctype.h>
#include "mmatch.h"

#define ToLower tolower

/*
 * mmatch()
 *
 * Written by Run (carlo@runaway.xs4all.nl), 25-10-96
 *
 *
 * From: Carlo Wood <carlo@runaway.xs4all.nl>
 * Message-Id: <199609021026.MAA02393@runaway.xs4all.nl>
 * Subject: [C-Com] Analysis for `mmatch' (was: gline4 problem)
 * To: coder-com@mail.undernet.org (coder committee)
 * Date: Mon, 2 Sep 1996 12:26:01 +0200 (MET DST)
 *
 * We need a new function `mmatch(const char *old_mask, const char *new_mask)'
 * which returns `true' likewise the current `match' (start with copying it),
 * but which treats '*' and '?' in `new_mask' differently (not "\*" and "\?" !)
 * as follows:  a '*' in `new_mask' does not match a '?' in `old_mask' and
 * a '?' in `new_mask' does not match a '\?' in `old_mask'.
 * And ofcourse... a '*' in `new_mask' does not match a '\*' in `old_mask'...
 * And last but not least, '\?' and '\*' in `new_mask' now become one character.
 */

#if 0
int mmatch(const char *old_mask, const char *new_mask)
{
  const char *m = old_mask;
  const char *n = new_mask;
  const char *ma = m;
  const char *na = n;
  int wild = 0;
  int mq = 0, nq = 0;

  while (1)
  {
    if (*m == '*')
    {
      while (*m == '*')
        m++;
      wild = 1;
      ma = m;
      na = n;
    }

    if (!*m)
    {
      if (!*n)
        return 0;
      for (m--; (m > old_mask) && (*m == '?'); m--)
        ;
      if ((*m == '*') && (m > old_mask) && (m[-1] != '\\'))
        return 0;
      if (!wild)
        return 1;
      m = ma;

      /* Added to `mmatch' : Because '\?' and '\*' now is one character: */
      if ((*na == '\\') && ((na[1] == '*') || (na[1] == '?')))
        ++na;

      n = ++na;
    }
    else if (!*n)
    {
      while (*m == '*')
        m++;
      return (*m != 0);
    }
    if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?')))
    {
      m++;
      mq = 1;
    }
    else
      mq = 0;

    /* Added to `mmatch' : Because '\?' and '\*' now is one character: */
    if ((*n == '\\') && ((n[1] == '*') || (n[1] == '?')))
    {
      n++;
      nq = 1;
    }
    else
      nq = 0;

/*
 * This `if' has been changed compared to match() to do the following:
 * Match when:
 *   old (m)         new (n)         boolean expression
 *    *               any             (*m == '*' && !mq) ||
 *    ?               any except '*'  (*m == '?' && !mq && (*n != '*' || nq)) ||
 * any except * or ?  same as m       (!((*m == '*' || *m == '?') && !mq) &&
 *                                      ToLower(*m) == ToLower(*n) &&
 *                                        !((mq && !nq) || (!mq && nq)))
 *
 * Here `any' also includes \* and \? !
 *
 * After reworking the boolean expressions, we get:
 * (Optimized to use boolean shortcircuits, with most frequently occuring
 *  cases upfront (which took 2 hours!)).
 */
    if ((*m == '*' && !mq) ||
        ((!mq || nq) && ToLower(*m) == ToLower(*n)) ||
        (*m == '?' && !mq && (*n != '*' || nq)))
    {
      if (*m)
        m++;
      if (*n)
        n++;
    }
    else
    {
      if (!wild)
        return 1;
      m = ma;

      /* Added to `mmatch' : Because '\?' and '\*' now is one character: */
      if ((*na == '\\') && ((na[1] == '*') || (na[1] == '?')))
        ++na;

      n = ++na;
    }
  }
}
#endif

/*
 * Compare if a given string (name) matches the given
 * mask (which can contain wild cards: '*' - match any
 * number of chars, '?' - match any single character.
 *
 * return  0, if match
 *         1, if no match
 */

/*
 * match
 *
 * Rewritten by Andrea Cocito (Nemesi), November 1998.
 *
 */

/****************** Nemesi's match() ***************/

int match(const char *mask, const char *str)
{
  const char *m = mask, *s = str;
  char ch;
  const char *bm, *bs;          /* Will be reg anyway on a decent CPU/compiler */

  /* Process the "head" of the mask, if any */
  while ((ch = *m++) && (ch != '*'))
    switch (ch)
    {
      case '\\':
        if (*m == '?' || *m == '*')
          ch = *m++;
      default:
        if (ToLower(*s) != ToLower(ch))
          return 1;
      case '?':
        if (!*s++)
          return 1;
    };
  if (!ch)
    return *s;

  /* We got a star: quickly find if/where we match the next char */
got_star:
  bm = m;                       /* Next try rollback here */
  while ((ch = *m++))
    switch (ch)
    {
      case '?':
        if (!*s++)
          return 1;
      case '*':
        bm = m;
        continue;               /* while */
      case '\\':
        if (*m == '?' || *m == '*')
          ch = *m++;
      default:
        goto break_while;       /* C is structured ? */
    };
break_while:
  if (!ch)
    return 0;                   /* mask ends with '*', we got it */
  ch = ToLower(ch);
  if (!*s)			/* String is already empty, don't continue */
    return 1;			/* This fixes the #quakenet access denied bug */
  while (ToLower(*s++) != ch)
    if (!*s)
      return 1;
  bs = s;                       /* Next try start from here */

  /* Check the rest of the "chunk" */
  while ((ch = *m++))
  {
    switch (ch)
    {
      case '*':
        goto got_star;
      case '\\':
        if (*m == '?' || *m == '*')
          ch = *m++;
      default:
        if (ToLower(*s) != ToLower(ch))
        {
          /* If we've run out of string, give up */
          if (!*bs)
            return 1;
          m = bm;
          s = bs;
          goto got_star;
        };
      case '?':
        if (!*s++)
          return 1;
    };
  };
  if (*s)
  {
    m = bm;
    s = bs;
    goto got_star;
  };
  return 0;
}

/*
 * collapse()
 * Collapse a pattern string into minimal components.
 * This particular version is "in place", so that it changes the pattern
 * which is to be reduced to a "minimal" size.
 *
 * (C) Carlo Wood - 6 Oct 1998
 * Speedup rewrite by Andrea Cocito, December 1998.
 * Note that this new optimized alghoritm can *only* work in place.
 */

#if 0
char *collapse(char *pattern)
{
  int star = 0;
  char *m = pattern;
  char *b;

  if (m)
  {
    do
    {
      if ((*m == '*') && ((m[1] == '*') || (m[1] == '?')))
      {
        b = m;
        do
        {
          if (*m == '*')
            star = 1;
          else
          {
            if (star && (*m != '?'))
            {
              *b++ = '*';
              star = 0;
            };
            *b++ = *m;
            if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?')))
              *b++ = *++m;
          };
        }
        while (*m++);
        break;
      }
      else
      {
        if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?')))
          m++;
      };
    }
    while (*m++);
  };
  return pattern;
}
#endif
