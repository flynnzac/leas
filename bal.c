/* 
   bal - for keeping accounts in order and studying past spending habits
   Copyright Zach Flynn <zlflynn@gmail.com>
*/

/* 
   This program is free software: you can redistribute it and/or modify
   it under the terms of version 3 of the GNU General Public License as published by
   the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#define BAL_VERSION "0.2.0-dev"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <regex.h>
#include <libguile.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <readline/readline.h>
#include <csv.h>
#include <ctype.h>
#include <string.h>
#include <libgen.h>
#include <signal.h>

#include "types.h"

/* global vars */

static struct book bal_book;
static SCM bal_cur_acct;
static int bal_prompton;
static SCM bal_cur_file;
static int bal_prompt_exit;
static int bal_select_tsct_num;
static struct tm* bal_curtime;

#include "utility.h"
#include "select.h"
#include "btar.h"
#include "call.h"
#include "interface.h"
#include "scm.h"

void
bal_exit (int exit_code)
{
  int c;

  if (bal_prompt_exit)
    {
      printf("Save file? (y/n) ");
      c = getchar();

      if (c=='y')
        {
          (void) bal_write (bal_cur_file);
        }
    }
  delete_book (&bal_book);
  exit (exit_code);
}

int
dummy_event ()
{
}

void
handler (int status)
{
  rl_replace_line("",0);
  if (bal_prompton==2)
    bal_prompton = 1;
  rl_done = 1;
}

int
main (int argc, char** argv)
{
  SCM ret;
  char* prompt;
  char* command;
  char* fname;
  int i;
  int k;
  time_t curtime_time;
  int load = 1;

  time(&curtime_time);
  bal_curtime = localtime(&curtime_time);

  bal_prompt_exit = 1;
  bal_select_tsct_num = 19;
  
  scm_with_guile (&register_guile_functions, NULL);
  bal_standard_func();
  bal_cur_acct = SCM_UNDEFINED;
  bal_cur_file = scm_from_locale_string ("_");

  rl_event_hook = dummy_event;
  signal(SIGINT,handler);
  
  while ((k = getopt(argc, argv, "f:l:sn")) != -1)
    {
      switch (k)
        {
        case 'n':
          load = 0;
          break;
        case 'l':
          scm_c_primitive_load(optarg);
          break;
        case 'f':
          {
            bal_cur_file = scm_from_locale_string (optarg);
            fname = scm_to_locale_string (bal_cur_file);
            fname = realloc(fname, sizeof(char)*(strlen(fname)+
                                                 strlen(".btar")
                                                 +1));
            strcat(fname, ".btar");
  
            if (access(fname, R_OK) != -1)
              {
                free(fname);
                fname = scm_to_locale_string(bal_cur_file);
                read_in (fname);
                if (bal_book.n_account > 0)
		  bal_cur_acct = scm_from_locale_string
		    (bal_book.accounts[0].name);

		free(fname);
              }
            else
              {
                free(fname);
              }
          }
          break;
        case 's':
          bal_prompt_exit = 0;
          break;
        case '?':
          if (optopt=='l')
	    fprintf (stderr, "Option -l requires an argument.\n");
          else if (optopt=='f')
	    fprintf (stderr, "Option -f requires an argument.\n");
          else
	    fprintf (stderr, "Unknown option, -%c.\n", optopt);
	  
	  exit(1);
          break;
        default:
          abort();
          break;
        }
    }

  if (load)
    {
      char* home = getenv("HOME");
      char* balrc;
  
      balrc = malloc(sizeof(char)*(strlen(home)+
                                   strlen("/.balrc.scm")+
                                   2));
      strcpy(balrc, home);
      strcat(balrc, "/.balrc.scm");
      
      if (access(balrc, R_OK) != -1)
	scm_c_primitive_load(balrc);

      free(balrc);
    }

  if (optind < argc)
    {
      for (i=optind; i < argc; i++)
	ret = scm_c_catch (SCM_BOOL_T,
			   exec_string_safe_history,
			   argv[i],
			   handle_error,
			   argv[i],
			   NULL,
			   NULL);
      bal_exit(0);
    }


  bal_prompton = 1;

  while (bal_prompton)
    {
      ret = scm_c_catch (SCM_BOOL_T,
                         exec_string_safe,
                         "bal/prompt",
                         handle_error,
                         "bal/prompt",
                         NULL,
                         NULL);

      prompt = scm_to_locale_string (ret);
      command = readline(prompt);
      
      ret = scm_c_catch (SCM_BOOL_T,
                         exec_string_safe_history,
                         command,
                         handle_error,
                         command,
                         NULL,
                         NULL);
      free(prompt);
      free(command);
    }

  bal_exit(0);
  return 0;
}
