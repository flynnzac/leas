#include "../src/leas.h"
#include <assert.h>
/* Test of all transaction time and sorting functions */

int
main (int argc, char** argv)
{
  scm_with_guile(&register_guile_functions, NULL);
  
  /* check_account_trans */
  account* a = malloc(sizeof(account));
  a->ob = 0.0;
  a->n_tsct = 3;
  a->tscts = malloc(sizeof(tsct)*3);
  a->tscts[0].amount = 112.30;
  a->tscts[1].amount = 30.23;
  a->tscts[2].amount = 10.71;


  leas_curtime = malloc(sizeof(struct tm));
  leas_curtime->tm_year = 120;
  leas_curtime->tm_mon = 0;
  leas_curtime->tm_mday = 16;

  a->tscts[0].year = 2020;
  a->tscts[0].month = 1;
  a->tscts[0].day = 15;

  a->tscts[1].year = 2020;
  a->tscts[1].month = 1;
  a->tscts[1].day = 15;

  a->tscts[2].year = 2020;
  a->tscts[2].month = 1;
  a->tscts[2].day = 17;
  
  SCM total = total_transactions(a);
  double total_c = scm_to_double(scm_cdr(total));
  double total_past_c = scm_to_double(scm_car(total));


  printf("%f\n%f\n", total_c, total_past_c);
  assert(total_c == 153.24);
  assert(total_past_c == 142.53);

  printf("total_transactions ... SUCCESS\n");

  return 0;

}
