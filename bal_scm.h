#ifndef BAL_SCM_H
#define BAL_SCM_H

/* contains standard interface scheme functions */

#define QUOTE(...) #__VA_ARGS__

void
bal_standard_func ()
{
  scm_c_eval_string ("");
  scm_c_eval_string
    (QUOTE(
           (define bal/number-to-quick-list 10)
           (define aa
            (lambda ()
             (bal/call-with-opts "bal/aa"
              (list
               (cons "Account" "string")
               (cons "Type" "type")
               (cons "Opening Balance" "real")
               (cons "Currency [1]" "currency")))))
           (define at
            (lambda ()
             (bal/call-with-opts "bal/at"
              (list
               (cons "Account" "default_account")
               (cons "Amount" "real")
               (cons "Description" "string")
               (cons "Day" "day")))))

           (define lastn
            (lambda ()
             (let ((tscts (bal/call-with-opts "bal/get-transactions"
                           (list
                            (cons "Account" "default_account")
                            (cons "How many?" "integer")))))
              (map-in-order
               (lambda (x)
                (display
                 (string-append
                  (number->string (list-ref x 2))
                  "-"
                  (number->string (list-ref x 3))
                  "-"
                  (number->string (list-ref x 4))
                  " "
                  (list-ref x 0)
                  " "
                  (number->string (list-ref x 1))
                  "\n")))
               tscts))
             (display "\n")))


           (define print-tscts
            (lambda (k)
             (map-in-order
              (lambda (x)
               (display
                (string-append
                 (number->string (list-ref x 2))
                 "-"
                 (number->string (list-ref x 3))
                 "-"
                 (number->string (list-ref x 4))
                 " "
                 (list-ref x 0)
                 " "
                 (number->string (list-ref x 1))
                 "\n")))
              k)))



           (define et
            (lambda ()
             (bal/call-with-opts "bal/et"
              (list
               (cons "Transaction" "transaction")))))

           (define lt
            (lambda ()
             (print-tscts (bal/get-transactions (bal/get-current-account) bal/number-to-quick-list))))


      
           (define ea
            (lambda ()
             (bal/call-with-opts "bal/ea"
              (list
               (cons "Account" "account")
               (cons "New account name" "string")))))

           (define da
            (lambda ()
             (bal/call-with-opts "bal/da"
              (list
               (cons "Account" "account")))))

           (define dt
            (lambda ()
             (bal/call-with-opts "bal/dt"
              (list
               (cons "Transaction" "transaction")))))

           (define la
            (lambda ()
             (let ((accts (bal/total-all-accounts)))
              (map-in-order
               (lambda (x)
                (display
                 (string-append
                  (car x)
                  " "
                  (number->string (cdr x))
                  "\n")))
               accts))))

           (define bt
            (lambda ()
             (let ((accts (bal/total-by-account-type)))
              (map-in-order
               (lambda (x)
                (display
                 (string-append
                  (car x)
                  " "
                  (number->string (cdr x))
                  "\n")))
               accts))))


           (define re
            (lambda ()
             (print-tscts
              (bal/call-with-opts "bal/get-transactions-by-regex"
               (list
                (cons "Account" "default_account")
                (cons "Regular Expression" "string"))))))


           (define sa
            (lambda ()
             (bal/call-with-opts "bal/set-account"
              (list
               (cons "Account" "account")))))

           (define ca
            (lambda ()
             (display
              (string-append (bal/get-current-account) "\n"))))
           
           (define w
            (lambda* (#:optional x)
             (if x
              (bal/write x)
              (bal/write (bal/get-current-file)))))
           ));
      
}

#endif
