#include "../src/leas.h"
#include <assert.h>
/* Test of Conversion Functions */

int
main (int argc, char** argv)
{
  scm_with_guile(&register_guile_functions, NULL);
  /* arg_type_from_string */
  SCM scm_type = scm_from_locale_string("day");
  assert(arg_type_from_string(scm_type) == DAY);
  scm_type = scm_from_locale_string("hello");
  assert(arg_type_from_string(scm_type) == OTHER);
  scm_type = scm_from_locale_string("account");
  assert(arg_type_from_string(scm_type) == ACCOUNT);
  printf("arg_type_from_string ... SUCCESS\n");

  /* account_type_to_string */
  char* test_str = account_type_to_string(EXPENSE);
  assert(strcmp(test_str, "expense") == 0);
  free(test_str);

  test_str = account_type_to_string(LIABILITY);
  assert(strcmp(test_str, "liability")==0);
  free(test_str);

  test_str = account_type_to_string(1024);
  assert(strcmp(test_str, "none")==0);
  free(test_str);
  
  printf("account_type_to_string ... SUCCESS\n");
  
  /* account_type_from_string */
  account_type at_test = account_type_from_string("expense");
  assert(at_test == EXPENSE);
  at_test = account_type_from_string("liability");
  assert(at_test == LIABILITY);
  at_test = account_type_from_string("#4t3qq");
  assert(at_test == -1);

  printf("account_type_from_string ... SUCCESS\n");
  
  return 0;

}
