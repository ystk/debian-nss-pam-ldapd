/*
   test_expr.c - simple tests for the expr module
   This file is part of the nss-pam-ldapd library.

   Copyright (C) 2009 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

/* we include expr.c because we want to test the static methods */
#include "common/expr.c"

#ifndef __ASSERT_FUNCTION
#define __ASSERT_FUNCTION ""
#endif /* not __ASSERT_FUNCTION */

#define assertstreq(str1,str2) \
  (assertstreq_impl(str1,str2,"strcmp(" __STRING(str1) "," __STRING(str2) ")==0", \
                    __FILE__, __LINE__, __ASSERT_FUNCTION))

/* for Solaris: */
#define __assert_fail(assertion,file,line,function) __assert(assertion,file,line)

/* method for determening string equalness */
static void assertstreq_impl(const char *str1,const char *str2,
                             const char *assertion,const char *file,
                             int line,const char *function)
{
  if (strcmp(str1,str2)!=0)
    __assert_fail(assertion,file,line,function);
}

static void test_parse_name(void)
{
  char buffer[20];
  int i;
  i=0;
  assert(parse_name("fooBar",&i,buffer,sizeof(buffer))!=NULL);
  assert(i==6);
  i=0;
  assert(parse_name("nameThatWillNotFitInBuffer",&i,buffer,sizeof(buffer))==NULL);
  i=0;
  assert(parse_name("foo Bar",&i,buffer,sizeof(buffer))!=NULL);
  assert(i==3);
  assertstreq(buffer,"foo");
}

static const char *expanderfn(const char *name,void UNUSED(*expander_attr))
{
  if (strcmp(name,"empty")==0)
    return "";
  else
    return "foobar";
}

static void test_expr_parse(void)
{
  char buffer[1024];
  assert(expr_parse("$test1",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"foobar");
  assert(expr_parse("$empty",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"");
  assert(expr_parse("${test1}\\$",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"foobar$");
  assert(expr_parse("${test1:-default}",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"foobar");
  assert(expr_parse("${empty:-default}",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"default");
  assert(expr_parse("${test1:+setset}",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"setset");
  assert(expr_parse("${empty:+setset}",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"");
  assert(expr_parse("${empty:-$test1}",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"foobar");
  assert(expr_parse("a/$test1/b",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"a/foobar/b");
  assert(expr_parse("a/$empty/b",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"a//b");
  assert(expr_parse("a${test1}b",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"afoobarb");
  assert(expr_parse("a${test1}b${test2:+${test3:-d$test4}e}c",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"afoobarbfoobarec");
  assert(expr_parse("a${test1}b${test2:+${empty:-d$test4}e}c",buffer,sizeof(buffer),expanderfn,NULL)!=NULL);
  assertstreq(buffer,"afoobarbdfoobarec");
}

static void test_buffer_overflow(void)
{
  char buffer[10];
  assert(expr_parse("$test1$empty$test1",buffer,sizeof(buffer),expanderfn,NULL)==NULL);
  assert(expr_parse("long test value",buffer,sizeof(buffer),expanderfn,NULL)==NULL);
  assert(expr_parse("${test1:-long test value}",buffer,sizeof(buffer),expanderfn,NULL)==NULL);
}

static void test_expr_vars(void)
{
  SET *set;
  /* simple test */
  set=set_new();
  assert(expr_vars("$a",set)!=NULL);
  assert(set_contains(set,"a"));
  assert(!set_contains(set,"$a"));
  set_free(set);
  /* more elaborate test */
  set=set_new();
  assert(expr_vars("\"${gecos:-$cn}\"",set)!=NULL);
  assert(set_contains(set,"gecos"));
  assert(set_contains(set,"cn"));
  set_free(set);
  /* more elaborate test */
  set=set_new();
  assert(expr_vars("\"${homeDirectory:-/home/$uidNumber/$uid}\"",set)!=NULL);
  assert(set_contains(set,"homeDirectory"));
  assert(set_contains(set,"uidNumber"));
  assert(set_contains(set,"uid"));
  set_free(set);
}

/* the main program... */
int main(int UNUSED(argc),char UNUSED(*argv[]))
{
  test_parse_name();
  test_expr_parse();
  test_buffer_overflow();
  test_expr_vars();
  return EXIT_SUCCESS;
}
