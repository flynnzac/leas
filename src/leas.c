/* 
   Leas - the Little Extensible Accounting System - for keeping accounts in order and studying past spending habits
   Copyright Zach Flynn <zlflynn@gmail.com>

   This program is free software: you can redistribute it and/or
   modify it under the terms of version 3 of the GNU General Public
   License as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   <https://www.gnu.org/licenses/>.
*/


/* The function definitions are in a header to facilitate testing */
#include "leas.h"

/* Exit Leas */
void
leas_exit ()
{
  char* quit;

  if (leas_prompt_exit)
    {
      quit = readline("Save file? (yes/no) ");
      while ((strcmp(quit,"yes") != 0) &&
             (strcmp(quit, "no") != 0))
        {
          free(quit);
          quit = readline("Save file? (yes/no) ");
        }
	  
      if (strcmp(quit, "yes")==0)
        {
          (void) leas_write(leas_cur_file);
        }
    }
  delete_book(&leas_book);
}

/* Dummy event for signals */
int
dummy_event ()
{
  return 0;
}

/* What to do on interrupt */
void
interrupt_handler (int status)
{
  rl_replace_line("",0);
  if (leas_prompton==PROMPT_SELECT) leas_prompton = PROMPT_COMMAND;
  rl_done = 1;
}

/* Load standard Leas functions */
void
leas_standard_func (char* file)
{
  char* command = copy_string("load ");
  append_to_string(&command, file, "\"");
  (void) exec_string_safe(command);
  free(command);
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

  struct option leas_options[] =
    {
     {"version", no_argument, 0, 'v'},
     {"no-rc", no_argument, 0, 'n'},
     {"load", required_argument, 0, 'l'},
     {"file", required_argument, 0, 'f'},
     {"no-prompt-on-quit", no_argument, 0, 's'},
     {"help", no_argument, 0, 'h'}
    };

  int option_index = 0;

  time(&curtime_time);
  leas_curtime = localtime(&curtime_time);

  leas_prompt_exit = 1;
  leas_select_tsct_num = 19;

  scm_with_guile(&register_guile_functions, NULL);
  leas_standard_func(LEAS_SCM_INSTALL);
  leas_cur_file = scm_from_locale_string("_.leas");

  rl_event_hook = dummy_event;
  signal(SIGINT,interrupt_handler);
  
  while ((k = getopt_long(argc, argv, "f:l:snvh", leas_options,
			  &option_index)) != -1)
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
          leas_cur_file = scm_from_locale_string(optarg);
          fname = scm_to_locale_string(leas_cur_file);

          if (access(fname, R_OK) != -1)
            {
              free(fname);
              fname = scm_to_locale_string(leas_cur_file);
              read_in (fname);
              if (leas_book.n_account > 0)
                leas_cur_acct = scm_from_locale_string
                  (leas_book.accounts[0].name);
            }
          free(fname);
          break;
        case 's':
          leas_prompt_exit = 0;
          break;
        case 'v':
          printf("%s\n", PACKAGE_STRING);
          printf("Copyright (C) 2020 Zach Flynn.\n");
          printf("License GPLv3: GNU GPL version 3 <https://gnu.org/licenses/gpl.html>\n");
          printf("This is free software: you are free to change and redistribute it.\n");
          printf("There is NO WARRANTY, to the extent permitted by law.\n");
          exit(0);
        case 'h':
          printf("Usage: leas [-f FILE] [-l FILE] [-s] [-n] [-v] [-h] [COMMAND]\n");
          printf("Leas %s - the Little Extensible Accounting System\n",
                 PACKAGE_VERSION);
          printf("\n");
          printf("--file,-f FILE  Load Leas save file.\n");
          printf("--load,-l FILE  Load Scheme code at start up.\n");
          printf("--no-prompt-on-quit,-s  Do not prompt to save file on exit.\n");
          printf("--no-rc, -n  Do not load ~/.leasrc.scm.\n");
	  printf("--help,-h Print this help message.\n");
	  printf("--version,-v Print the version of Leas and exit.\n");
          printf("COMMAND Execute Leas command on startup and exit.\n");
	  exit(0);
          break;
        case '?':
          if (optopt=='l')
            fprintf(stderr, "Option -l requires an argument.\n");
          else if (optopt=='f')
            fprintf(stderr, "Option -f requires an argument.\n");
          else
            fprintf(stderr, "Unknown option, -%c.\n", optopt);
	  
          exit(1);
          break;
          break;
        default:
          abort();
          break;
        }
    }

  if (load)
    {
      char* home = getenv("HOME");
      char* leasrc;

      leasrc = copy_string(home);
      append_to_string(&leasrc, "/.leasrc.scm", "");
      
      if (access(leasrc, R_OK) != -1)
        scm_c_primitive_load(leasrc);

      free(leasrc);
    }

  if (optind < argc)
    {
      for (i=optind; i < argc; i++)
        ret = scm_c_catch(SCM_BOOL_T,
                          exec_string_safe_history,
                          argv[i],
                          handle_error,
                          argv[i],
                          NULL,
                          NULL);
      leas_exit();
      return 0;
    }


  if (leas_book.n_account == 0)
    {
      /* create cash account if no other account */
      leas_aa(scm_from_locale_string("Cash"),
              scm_from_locale_string("asset"),
              scm_from_double(0.0));
    }
  
  leas_prompton = PROMPT_COMMAND;

  while (leas_prompton != PROMPT_OFF)
    {
      ret = scm_c_catch(SCM_BOOL_T,
                        exec_string_safe,
                        "leas/prompt",
                        handle_error,
                        "leas/prompt",
                        NULL,
                        NULL);

      prompt = scm_to_locale_string(ret);
      command = readline(prompt);
      
      ret = scm_c_catch(SCM_BOOL_T,
                        exec_string_safe_history,
                        command,
                        handle_error,
                        command,
                        NULL,
                        NULL);
      
      free(prompt); free(command);
    }
  scm_gc();
  leas_exit();
  return 0;
}

