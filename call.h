/* function to call a function with options specified interactively */

void
append_to_command (char** base, const char* add, const char* surround)
{
  int sz = strlen(*base)+strlen(add)+2*strlen(surround);
  *base = realloc(*base, sizeof(char)*(sz+1));
  strcat(*base, surround);
  strcat(*base, add);
  strcat(*base, surround);
}

SCM
bal_call (SCM func, SCM options)
{
  int i,j,k;
  SCM pair;
  SCM name;

  arg_type type;

  char* tmp_str;
  char* name_c = NULL;
  char* opt;

  time_t curtime;
  struct tm* curtime_info;

  int len = scm_to_int (scm_length (options));
  char* year;
  char* month;
  char* day;

  char* func_c = scm_to_locale_string (func);
  char* command = copy_string(func_c);
  append_to_command(&command, " ", "");

  free(func_c);
  bal_prompton = 2;

  for (i=0; i < len; i++)
    {
      if (bal_prompton != 2)
        break;

      pair = scm_list_ref (options, scm_from_int(i));

      name = scm_car(pair);

      name_c = scm_to_locale_string (name);
      name_c = realloc (name_c, sizeof(char)*(strlen(name_c)+4));
      strcat(name_c, ": ");
      
      type = type_from_string(scm_cdr(pair));

      switch (type)
        {
        case STRING:
          opt = readline(name_c);
          append_to_command(&command, opt, "\"");
          break;
        case ACCOUNT:
          opt = bal_select_account(name_c);
          if (opt != NULL && strcmp(opt,"") && anyalpha(opt) == 0)
            {
              k = atoi(opt);
              append_to_command(&command,
                                bal_book.accounts[k].name,
                                "\"");
            }
	  else bal_prompton = 1;
          break;
        case CURRENT_ACCOUNT:
          opt = scm_to_locale_string (bal_cur_acct);
          append_to_command(&command, opt, "\"");
          break;
        case TYPE:
          k = bal_select_account_type(name_c);
	  if (k >= 0)
	    {
	      tmp_str = account_type_to_string(k);
	      append_to_command(&command, tmp_str, "\"");
	      free(tmp_str);
	    } else bal_prompton = 1;
          break;
        case TRANSACTION:
          printf("%s\n", name_c);
          opt = bal_select_account("Account: ");
          if (opt!=NULL && strcmp(opt,"")!=0)
            {
              if (anyalpha(opt) == 0)
                {
                  k = atoi(opt);
                  free(opt);

                  j = bal_select_transaction (&bal_book.accounts[k]);

                  if (j >= 0)
                    {
		      tmp_str = malloc(sizeof(char)*(strlen("(cons  )") +
						     digits(k)+digits(j)+1));
                      sprintf(tmp_str, "(cons %d %d)", k, j);
                      append_to_command(&command, tmp_str, "");
                      free(tmp_str);
                    }
                }
            } else bal_prompton = 1;
          break;
        case DAY:
          time(&curtime);
          curtime_info = localtime(&curtime);

          printf("%s\n", name_c);

          bal_select_day (curtime_info, &year, &month, &day);
          command = realloc(command,
                            sizeof(char)*(strlen(command)+4+2+2+15));

          
          sprintf(command, "%s(list %d %d %d)", command,
                  strcmp(day,"")==0 ? curtime_info->tm_mday : atoi(day),
                  strcmp(month,"")==0 ? (curtime_info->tm_mon+1) : atoi(month),
                  strcmp(year,"")==0 ? (curtime_info->tm_year+1900) : atoi(year));
          break;
        default:
          opt = readline(name_c);
          append_to_command(&command, opt, "");
          break;
        }

      if (type != DAY) free(opt);
      free(name_c);
      
      if (i != (len-1))
        append_to_command(&command, " ", "");

    }

  if (bal_prompton == 2)
    {
      SCM ret = scm_c_catch (SCM_BOOL_T,
                             exec_string_safe,
                             command,
                             handle_error,
                             command,
                             NULL,
                             NULL);
      return ret;
    }

  free(command);
  bal_prompton = 1;

  return SCM_UNDEFINED;
}
