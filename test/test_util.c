#include "../src/leas.h"
#include <assert.h>
/* Test of all Convenience/Manipulation/Utility Functions */

int
main (int argc, char** argv)
{
  /* pow_int */
  assert(pow_int(2,3) == 8);
  assert(pow_int(3,2) == 9);
  printf("pow_int ... SUCCESS\n");

  /* remove_ext */
  char* test = copy_string("hello.ext");
  test = remove_ext(test);
  assert(strcmp(test, "hello") == 0);
  free(test);

  test = copy_string(".ext");
  test = remove_ext(test);
  assert(strcmp(test,"")==0);
  free(test);
  
  test = copy_string("hello");
  test = remove_ext(test);
  assert(strcmp(test,"hello")==0);
  free(test);
  
  printf("remove_ext ... SUCCESS\n");
  /* digits */
  assert(digits(1)==1);
  assert(digits(19)==2);
  assert(digits(12435) == 5);
  assert(digits(0)==1);
  assert(digits(-12)==3);

  /* anyalpha */
  assert(anyalpha("34286396")==0);
  assert(anyalpha("34t15th043")==1);
  assert(anyalpha("34x")==1);
  assert(anyalpha("x34")==1);

  printf("anyalpha ... SUCCESS\n");

  /* copy_string_insert_int */
  test = copy_string_insert_int("hello %d", 100);
  assert(strcmp(test, "hello 100")==0);
  free(test);

  test = copy_string_insert_int("%d hello", 100);
  assert(strcmp(test, "100 hello")==0);
  free(test);

  test = copy_string_insert_int("hello", 100);
  assert(strcmp(test, "hello")==0);
  free(test);

  printf("copy_string_insert_int ... SUCCESS\n");
  /* append_to_string */
  test = copy_string("hello ");
  append_to_string(&test, "San Diego", ",");
  assert(strcmp(test, "hello ,San Diego,")==0);
  
  printf("append_to_string ... SUCCESS\n");
  return 0;

}
