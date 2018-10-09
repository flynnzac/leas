#ifndef BAL_CALL_WITH_OPTS_H
#define BAL_CALL_WITH_OPTS_H

SCM
bal_call_with_opts (SCM func, SCM options)
{

  int i,n,j,k;
  SCM pair;
  SCM name, type;

  char* name_c = NULL;
  char* type_c = NULL;
  char* opt;

  time_t curtime;
  struct tm* curtime_info;

  tsct* t;
  
  int len = scm_to_int (scm_length (options));
  char** year = malloc(sizeof(char*));
  char** month = malloc(sizeof(char*));
  char** day = malloc(sizeof(char*));

  char* func_c = scm_to_locale_string (func);
  char* command = malloc((strlen(func_c)+3)*sizeof(char));

  strcpy(command, func_c);
  strcat(command, " ");

  free(func_c);

  for (i=0; i < len; i++)
    {
      pair = scm_list_ref (options, scm_from_int(i));

      name = scm_car(pair);
      type = scm_cdr(pair);

      name_c = scm_to_locale_string (name);
      name_c = realloc (name_c, sizeof(char)*(strlen(name_c)+4));
      strcat(name_c, ": ");
      
      type_c = scm_to_locale_string (type);

      if (strcmp(type_c, "string")==0)
        {
          opt = readline(name_c);
          command = realloc (command,
                             (strlen(command)+strlen(opt)+5)*sizeof(char));
          strcat(command, "\"");
          strcat(command, opt);
          strcat(command, "\"");
        }
      else if (strcmp(type_c, "account")==0)
        {
          opt = bal_select_account();
          if (strcmp(opt,"")==0)
            {
              k = 0;
            }
          else
            {
              k = atoi(opt);
            }
          
          command = realloc (command,
                             (strlen(command)+strlen(bal_book.accounts[k].name)+5)*sizeof(char));
          strcat(command, "\"");
          strcat(command, bal_book.accounts[k].name);
          strcat(command, "\"");
        }
      else if (strcmp(type_c, "default_account")==0)
        {
          opt = scm_to_locale_string (bal_cur_acct);
          command = realloc(command,
                            (strlen(command) + strlen(opt)+5)*sizeof(char));
          strcat(command, "\"");
          strcat(command, opt);
          strcat(command, "\"");

        }
      else if (strcmp(type_c, "type")==0)
        {
          printf("%d: Expense\n", EXPENSE);
          printf("%d: Income\n", INCOME);
          printf("%d: Asset\n", ASSET);
          printf("%d: Liability\n", LIABILITY);
          opt = readline (name_c);
          if (strcmp(opt,"")==0)
            {
              k = -1;
            }
          else
            {
              k = atoi(opt);
            }

          switch (k)
            {
            case EXPENSE:
              command = realloc (command,
                                 (strlen(command)+strlen("expense")+5)*sizeof(char));
              strcat(command, "\"expense\"");
              break;
            case INCOME:
              command = realloc (command,
                                 (strlen(command)+strlen("income")+5)*sizeof(char));
              strcat(command, "\"income\"");
              break;
            case ASSET:
              command = realloc (command,
                                 (strlen(command)+strlen("asset")+5)*sizeof(char));
              strcat(command, "\"asset\"");
              break;
            case LIABILITY:
              command = realloc (command,
                                 (strlen(command)+strlen("liability")+5)*sizeof(char));
              strcat(command, "\"liability\"");
              break;
            default:
              command = realloc (command,
                                 (strlen(command)+strlen("liability")+5)*sizeof(char));
              strcat(command, "\"none\"");
              break;
            }

        }
      else if (strcmp(type_c, "currency")==0)
        {
          opt = readline(name_c);
          if (strcmp(opt,"")==0)
            {
              command = realloc(command,
                                (strlen(command)+3)*sizeof(char));
              strcat(command, "1");
            }
          else
            {
              command = realloc(command,
                                (strlen(command)+strlen(opt)+3)*sizeof(char));
              strcat(command, opt);
            }
        }
      else if (strcmp(type_c, "transaction")==0)
        {
          printf("%s\n", name_c);
          opt = bal_select_account();

          if (opt==NULL) return SCM_UNDEFINED;

          k = atoi(opt);
          
          free(opt);

          j = bal_select_transaction (&bal_book.accounts[k]);

          opt = malloc(sizeof(char)*(((k % 10) + 1) + 1 +
                                     ((j % 10) + 1) + 1));

          sprintf(opt, "(cons %d %d)", j, k);
          
          command = realloc(command,
                            (strlen(command)+
                             strlen(opt)+3)*sizeof(char));

          strcat(command, opt);
          
        }
      else if (strcmp(type_c, "day")==0)
        {

          time(&curtime);
          curtime_info = localtime(&curtime);

          printf("%s\n", name_c);

          bal_select_day (curtime_info, year, month, day);

          command = realloc(command,
                            sizeof(char)*(strlen(command)+4+2+2+10));

          
          curtime_info->tm_year = strcmp(*year,"")==0 ? (curtime_info->tm_year+1900) : atoi(*year);
          curtime_info->tm_mon = strcmp(*month,"")==0 ? (curtime_info->tm_mon+1) : atoi(*month);
          curtime_info->tm_mday = strcmp(*day,"")==0 ? curtime_info->tm_mday : atoi(*day);

          sprintf(command, "%s\"%d-%d-%d\"", command,
                  curtime_info->tm_year,
                  curtime_info->tm_mon,
                  curtime_info->tm_mday);

          
          
        }
      else
        {
          opt = readline(name_c);
          command = realloc (command,
                             (strlen(command)+strlen(opt)+5)*sizeof(char));
          strcat(command, opt);
        }


      if (strcmp(type_c,"day") != 0)
        {
          free(opt);
        }

      free(name_c);
      free(type_c);
      
      if (i != (len-1))
        {
          command = realloc(command,
                            (strlen(command)+2)*sizeof(char));
          strcat(command, " ");
        }

    }

  printf("Command: %s\n", command);
  SCM ret = scm_c_catch (SCM_BOOL_T,
                         exec_string_safe,
                         command,
                         handle_error,
                         command,
                         NULL,
                         NULL);
  free(year);
  free(month);
  free(day);

  free(command);
  return ret;
}



#endif
