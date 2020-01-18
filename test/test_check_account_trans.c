#include "../src/leas.h"
#include <assert.h>
/* Test of all transaction time and sorting functions */

int
main (int argc, char** argv)
{
  scm_with_guile(&register_guile_functions, NULL);
  
  /* check_account_trans */

  leas_book.accounts = malloc(sizeof(account));
  leas_book.accounts[0].n_tsct = 2;
  leas_book.n_account = 1;

  assert(check_account_trans(0, 1));
  assert(!check_account_trans(0, 2));
  assert(!check_account_trans(1, 2));


  printf("check_account_trans ... SUCCESS\n");
  
  free(leas_book.accounts);
  return 0;

}
