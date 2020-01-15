#include "../src/leas.h"
#include <assert.h>
/* Test of all transaction time and sorting functions */

int
main (int argc, char** argv)
{
  scm_with_guile(&register_guile_functions, NULL);
  
  /* set_tsct_time */
  struct tm test_time;
  test_time.tm_year = 29;
  test_time.tm_mon = 9;
  test_time.tm_mday = 24;

  tsct test_t;

  set_tsct_time(&test_t, &test_time);
  assert(test_t.year == 1929);
  assert(test_t.month == 10);
  assert(test_t.day == 24);

  printf("set_tsct_time ... SUCCESS\n");

  /* set_tsct_time_from_scm */
  SCM list_test = scm_list_3(scm_from_int(12),
			     scm_from_int(12),
			     scm_from_int(1976));
  set_tsct_time_from_scm(&test_t, list_test);
  assert(test_t.year == 1976);
  assert(test_t.month == 12);
  assert(test_t.day == 12);

  tsct tscts[4];

  tscts[0].year = 1929;
  tscts[0].month = 10;
  tscts[0].day = 24;

  tscts[1].year = 1935;
  tscts[1].month = 11;
  tscts[1].day = 1;

  tscts[2].year = 1935;
  tscts[2].month = 10;
  tscts[2].day = 10;

  tscts[3].year = 1929;
  tscts[3].month = 10;
  tscts[3].day = 24;
    
  assert(sort_transactions(&tscts[0], &tscts[3]) == 0);
  assert(sort_transactions(&tscts[0], &tscts[1]) == -1);
  assert(sort_transactions(&tscts[0], &tscts[2]) == -1);
  assert(sort_transactions(&tscts[1], &tscts[2]) == 1);

  printf("sort_transactions ... SUCCESS\n");
  

  return 0;

}
