#include "../src/leas.h"
#include <assert.h>
/* Test of all transaction time and sorting functions */

int
main (int argc, char** argv)
{
  scm_with_guile(&register_guile_functions, NULL);
  
  /* find_account_location_in_book */

  leas_book.accounts = malloc(sizeof(account));
  leas_book.n_account = 1;
  leas_book.accounts[0].name = copy_string("Salary");

  assert(find_account_location_in_book(&leas_book,
                                       "Salary")==0);
  assert(find_account_location_in_book(&leas_book,
                                       "Income")==-1);


  printf("find_account_location_in_book ... SUCCESS\n");

  delete_book(&leas_book);

  return 0;

}
