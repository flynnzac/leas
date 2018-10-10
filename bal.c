#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <regex.h>
#include <libguile.h>
#include <math.h>
#include <time.h>

#include "bal_types.h"

/* global vars */
struct book bal_book;
SCM bal_cur_acct;
int bal_prompton;
SCM bal_cur_file;


#include "bal_utility.h"
#include "bal_select.h"
#include "bal_read.h"
#include "bal_modify.h"
#include "bal_write.h"

#include "bal_interface.h"
#include "bal_scm.h"

void
bal_exit (int exit_code)
{
  (void) bal_write (bal_cur_file);
  delete_book (&bal_book);
  exit (exit_code);
}


int
main (int argc, char** argv)
{
  SCM ret;
  const char* prompt = ":> ";
  char* command;
  int i;
  int k;

  scm_with_guile (&register_guile_functions, NULL);
  bal_standard_func();

  bal_cur_file = scm_from_locale_string ("_");
  
  while ((k = getopt(argc, argv, "f:")) != -1)
    {
      switch (k)
        {
        case 'l':
          scm_c_primitive_load(optarg);
          break;
        case 'f':
          bal_cur_file = scm_from_locale_string (optarg);
          break;
        case '?':
          if (optopt=='l')
            {
              fprintf (stderr, "Option -l requires an argument.\n");
              exit(1);
            }
          else if (optopt=='f')
            {
              fprintf (stderr, "Option -f requires an argument.\n");
              exit(1);
            }
          else
            {
              fprintf (stderr, "Unknown option, -%c.\n", optopt);
              exit(1);
            }
          break;
        default:
          abort();
          break;
        }
    }

  if (optind < argc)
    {
      for (i=optind; i < argc; i++)
        {
          ret = scm_c_catch (SCM_BOOL_T,
                             exec_string_safe_history,
                             argv[i],
                             handle_error,
                             argv[i],
                             NULL,
                             NULL);
        }

      bal_exit(0);
    }


  bal_prompton = 1;
  bal_cur_acct = SCM_UNDEFINED;
  
  while (bal_prompton)
    {
      command = readline(prompt);
      
      ret = scm_c_catch (SCM_BOOL_T,
                         exec_string_safe_history,
                         command,
                         handle_error,
                         command,
                         NULL,
                         NULL);
      free(command);
    }

  bal_exit(0);
  return 0;
}


                        
  
