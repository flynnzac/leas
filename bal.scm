(use-modules (ice-9 format))
(use-modules (srfi srfi-1))

(define bal/number-to-quick-list 20)
(define aa
  (lambda ()
    (bal/call "bal/aa"
              (list
               (cons "Account" "string")
               (cons "Type" "type")
               (cons "Opening Balance" "real")))))


(define at
  (lambda ()
    (bal/call "bal/at"
              (list
               (cons "Account" "current_account")
               (cons "Amount" "real")
               (cons "Description" "string")
               (cons "Day" "day")))))


(define bal/print-tscts
  (lambda (k)
    (if (list? k)
        (let ((len (apply max
                          (map (lambda (x)
                                 (string-length (car x))) k))))
          (map-in-order
           (lambda (x)
             (display
              (string-append
               (number->string (list-ref x 2))
               "-"
               (format #f "~2,'0d" (list-ref x 3))
               "-"
               (format #f "~2,'0d" (list-ref x 4))
               " "
               (format #f (string-append
                           "~"
                           (number->string (+ len 5))
                           "a") (list-ref x 0))
               " "
               (format #f "~10,2f" (list-ref x 1))
               "\n")))
           k)))))

(define ltn
  (lambda ()
    (let ((tscts (bal/call "bal/get-transactions"
                           (list
                            (cons "Account" "current_account")
                            (cons "How many?" "integer")))))
      (bal/print-tscts tscts))))

(define bal/edit-transact
  (lambda (tsct day amount desc)
    (let ((tsct-attr (bal/get-transaction-by-location
                      (car tsct) (cdr tsct))))
      (bal/dt tsct)
      (bal/at (car (bal/get-account-by-location (car tsct)))
              (if (string-null? amount)
                  (list-ref tsct-attr 1)
                  (string->number amount))
              (if (string-null? desc)
                  (list-ref tsct-attr 0)
                  desc)
              day))))

(define et
  (lambda* (#:optional n)
    (if n (bal/set-select-transact-num n))
    (bal/call "bal/edit-transact"
              (list
               (cons "Transaction" "transaction")
               (cons "Day (default is current day)" "day")
               (cons "Amount" "string")
               (cons "Description" "string")))))

(define lt
  (lambda ()
    (let* ((tscts (bal/get-transactions-by-day
                   (bal/get-current-account)
                   (list 0 0 0)
                   (bal/get-current-day)))
           (final-tsct (list-tail tscts
                                  (max (- (length tscts)
                                          bal/number-to-quick-list) 0))))
      (if (and (list? final-tsct)
               (not (null? final-tsct)))
          (bal/print-tscts final-tsct)))))

(define ea
  (lambda ()
    (bal/call "bal/ea"
              (list
               (cons "Account" "account")
               (cons "New account name" "string")
               (cons "Opening balance" "string")))))

(define da
  (lambda ()
    (bal/call "bal/da"
              (list
               (cons "Account" "account")))))

(define dt
  (lambda ()
    (bal/call "bal/dt"
              (list
               (cons "Transaction" "transaction")))))

(define bal/display-account-totals
  (lambda (accts)
    (let ((len
           (apply max
                  (map
                   (lambda (x) (string-length (car x))) accts))))
      (map-in-order
       (lambda (x)
         (display
          (string-append
           (format #f
                   (string-append "~"
                                  (number->string (+ len 5))
                                  "a") (car x))
           " "
           (format #f "~10,2f" (car (cdr x)))
           " "
           (format #f "~10,2f" (cdr (cdr x)))
           "\n")))
       accts))))           


(define la
  (lambda ()
    (bal/display-account-totals (bal/total-all-accounts))))

(define lae
  (lambda ()
    (bal/display-account-totals
     (bal/total-all-accounts-of-type 0))))

(define lai
  (lambda ()
    (bal/display-account-totals
     (bal/total-all-accounts-of-type 1))))

(define laa
  (lambda ()
    (bal/display-account-totals
     (bal/total-all-accounts-of-type 2))))

(define lal
  (lambda ()
    (bal/display-account-totals
     (bal/total-all-accounts-of-type 3))))


(define bt
  (lambda ()
    (let* ((accts (bal/total-by-account-type))
           (len
            (apply max
                   (map
                    (lambda (x) (string-length (car x))) accts))))
      (map-in-order
       (lambda (x)
         (display
          (if (pair? (cdr x))
              (string-append
               (format #f
                       (string-append "~"
                                      (number->string (+ len 5))
                                      "a") (car x))
               " "
               (format #f "~10,2f" (car (cdr x)))
               " "
               (format #f "~10,2f" (cdr (cdr x)))
               "\n")
              (string-append
               (format #f
                       (string-append "~"
                                      (number->string (+ len 5))
                                      "a") (car x))
               " "
               (format #f "~10,2f" (cdr x))
               "\n"))))
       accts))))

(define bal/current-total-of-type
  (lambda (n)
    (lambda ()
      (let* ((accts (bal/total-by-account-type))
             (el (cdr (list-ref accts n))))
        (display (string-append
                  (format #f "~,2f"
                          (if (pair? el)
                              (car el)
                              el))
                  "\n"))))))


(define cex (bal/current-total-of-type 0))
(define cin (bal/current-total-of-type 1))
(define cas (bal/current-total-of-type 2))
(define cli (bal/current-total-of-type 3))
(define cwo (bal/current-total-of-type 4))
(define cba (bal/current-total-of-type 5))

(define re
  (lambda ()
    (bal/print-tscts
     (bal/call "bal/get-transactions-by-regex"
               (list
                (cons "Account" "current_account")
                (cons "Regular Expression" "string"))))))

(define sa
  (lambda ()
    (bal/call "bal/set-account"
              (list
               (cons "Account" "account")))))

(define ca
  (lambda ()
    (display
     (string-append (bal/get-current-account) "\n"))))

(define w
  (lambda ()
    (bal/call "bal/write"
              (list
               (cons "File" "string")))))

(define r
  (lambda ()
    (bal/call "bal/read"
              (list
               (cons "File" "string")))))

(define bal/prompt
  (lambda ()
    (if (= (bal/get-number-of-accounts) 0)
        ":> "
        (string-append
         "("
         (bal/get-current-account)
         ") :> "))))

(define bal/t
  (lambda (to-account from-account amount desc day)
    (bal/at to-account amount desc day)
    (bal/at from-account (* -1 amount) desc day)))

(define t
  (lambda ()
    (bal/call "bal/t"
              (list
               (cons "To Account" "account")
               (cons "From Account" "account")
               (cons "Amount" "real")
               (cons "Description" "string")
               (cons "Day" "day")))))

(define bal/dtr
  (lambda (from-account location)
    (let ((transaction
           (bal/get-transaction-by-location (car location)
                                            (cdr location)))
          (all-from (bal/get-all-transactions from-account))
          (num-from (bal/get-account-location from-account)))
      (bal/dt (cons num-from
                    (list-index
                     (lambda (u)
                       (and (string=? (car u) (car transaction))
                            (lset= (lambda (v w) (= (abs v) (abs w)))
                                   (cdr u) (cdr transaction)))) all-from)))
      (bal/dt location))))

(define dtr
  (lambda ()
    (bal/call "bal/dtr"
              (list
               (cons "From Account" "account")
               (cons "Transaction" "transaction")))))


(define ltbd
  (lambda ()
    (bal/print-tscts
     (bal/call "bal/get-transactions-by-day"
               (list
                (cons "Account" "current_account")
                (cons "From Day" "day")
                (cons "To Day" "day"))))))

(define v
  (lambda ()
    (display
     (string-append
      "bal version: "
      (bal/v)
      "\n"))))

(define sd
  (lambda ()
    (bal/call "bal/set-current-day"
              (list
               (cons "Current Day" "day")))))

(define cd
  (lambda ()
    (let ((current-day (bal/get-current-day)))
      (display (string-append
                (number->string (list-ref current-day 2))
                "-"
                (format #f "~2,'0d" (list-ref current-day 1))
                "-"
                (format #f "~2,'0d" (list-ref current-day 0))
                "\n")))))

(use-modules (srfi srfi-19))

(define bal/day-from-time
  (lambda (x)
    (let ((xdate (time-utc->date x 0)))
      (list (date-day xdate) (date-month xdate) (date-year xdate)))))

(define bal/seq-days
  (lambda (first-day last-day by)
    (let ((first-time (date->time-utc (make-date 0 0 0 0
                                                 (list-ref first-day 0)
                                                 (list-ref first-day 1)
                                                 (list-ref first-day 2)
                                                 0)))
          (last-time (date->time-utc (make-date 0 0 0 0
                                                (list-ref last-day 0)
                                                (list-ref last-day 1)
                                                (list-ref last-day 2)
                                                0))))
      (if (time>=? first-time last-time)
          (list first-day)
          (cons first-day (bal/seq-days 
                           (bal/day-from-time
                            (add-duration first-time
                                          (make-time time-duration 0 (* 24 3600 by))))
                           last-day
                           by))))))
(define-syntax bal/loop-days
  (lambda (x)
    (syntax-case x ()
      ((_ days current-day val exp)
       (with-syntax ((i (datum->syntax x (quote i))))
         (syntax (let loop-day ((i 0))
                   (if (< i val)
                       (begin
                         (bal/set-current-day (list-ref days i))
                         (cons (cons (list-ref days i)
                                     exp)
                               (loop-day (+ i 1))))
                       (begin
                         (bal/set-current-day current-day)
                         (list))))))))))

(define bal/balance-account-on-days
  (lambda (first-day last-day by account)
    (let ((days (bal/seq-days first-day last-day by))
          (current-day (bal/get-current-day)))
      (bal/loop-days days current-day (length days)
                     (list-ref (bal/total-account account) 1)))))

(define bal/total-transact-in-account-between-days
  (lambda (first-day last-day by account)
    (let* ((balance (bal/balance-account-on-days
                     first-day last-day by account))
           (current-day (bal/get-current-day))
           (days (map car balance)))
      (bal/loop-days days current-day (- (length balance) 1)
                     (- (cdr (list-ref balance (+ i 1)))
                        (cdr (list-ref balance i)))))))

(define bal/total-transact-in-account-re
  (lambda (first-day last-day by account regex)
    (let ((current-day (bal/get-current-day))
          (days (bal/seq-days first-day last-day by)))
      (bal/loop-days days current-day (length days)
                     (apply +
                            (map (lambda (u) (list-ref u 1))
                                 (bal/get-transactions-by-regex account regex)))))))

(define bal/output-by-day
  (lambda (day amount)
    (display
     (string-append
      (number->string (list-ref day 2))
      "-"
      (format #f "~2,'0d" (list-ref day 1))
      "-"
      (format #f "~2,'0d" (list-ref day 0))
      " "
      (format #f "~10,2f" amount)
      "\n"))))

(define baod
  (lambda ()
    (let ((result (bal/call "bal/balance-account-on-days"
                            (list
                             (cons "First Day" "day")
                             (cons "Last Day" "day")
                             (cons "By" "number")
                             (cons "Account" "current_account")))))
      (map-in-order
       (lambda (x)
         (bal/output-by-day (car x) (cdr x)))
       result))))

(define bal/get-by-type-over-days
  (lambda (first-day last-day by num)
    (let ((days (bal/seq-days first-day last-day by))
          (current-day (bal/get-current-day)))
      (bal/loop-days days current-day (length days)
                     (list-ref
                      (list-ref (bal/total-by-account-type) num)
                      1)))))

(define bal/get-by-type-over-days-for-type
  (lambda (n)
    (lambda (first-day last-day by)
      (if (string=? by "")
          (bal/get-by-type-over-days first-day last-day 7 n)
          (bal/get-by-type-over-days first-day last-day (string->number by) n)))))

(define-syntax bal/over-day-cmd
  (syntax-rules ()
    ((over-day-cmd val)
     (let ((result (bal/call
                    (string-append
                     "(bal/get-by-type-over-days-for-type "
                     (number->string val)
                     ")")
                    (list
                     (cons "First Day" "day")
                     (cons "Last Day" "day")
                     (cons "By [7]" "string")))))
       (map-in-order
        (lambda (x)
          (bal/output-by-day (car x) (cdr x)))
        result)))))

(define exod
  (lambda ()
    (bal/over-day-cmd 0)))

(define inod
  (lambda ()
    (bal/over-day-cmd 1)))

(define asod
  (lambda ()
    (bal/over-day-cmd 2)))

(define liod
  (lambda ()
    (bal/over-day-cmd 3)))

(define wood
  (lambda ()
    (bal/over-day-cmd 4)))

(define ttbd
  (lambda ()
    (let ((result (bal/call
                   "bal/total-transact-in-account-between-days"
                   (list
                    (cons "First day" "day")
                    (cons "Last day" "day")
                    (cons "By" "number")
                    (cons "Account" "current_account")))))
      (map-in-order
       (lambda (x)
         (bal/output-by-day (car x) (cdr x)))
       result))))

(define ttre
  (lambda ()
    (let ((result (bal/call
                   "bal/total-transact-in-account-re"
                   (list
                    (cons "First day" "day")
                    (cons "Last day" "day")
                    (cons "By" "number")
                    (cons "Account" "current_account")
                    (cons "Regex" "string")))))
      (map-in-order
       (lambda (x)
         (bal/output-by-day (car x) (cdr x)))
       result))))


(define bal/pay-loan
  (lambda (loan-account interest-account from-account
                        principal interest desc day)
    (bal/at loan-account principal desc day)
    (bal/at interest-account interest desc day)
    (bal/at from-account (* -1 (+ principal interest)) desc day)))

(define pl
  (lambda ()
    (bal/call "bal/pay-loan"
              (list
               (cons "Loan Account" "account")
               (cons "Interest Account" "account")
               (cons "Pay from Account" "account")
               (cons "Principal" "real")
               (cons "Interest" "real")
               (cons "Description" "string")
               (cons "Day" "day")))))

(define fn
  (lambda ()
    (display
     (string-append (bal/get-current-file) "\n"))))

(define bal/change-stock-price
  (lambda (account from-account stock-price number day)
    (let* ((value (list-ref (bal/total-account account) 1))
           (newval (* number stock-price))
           (trval (- newval value)))
      (bal/t account from-account trval
             "Stock Price Change" day))))

(define csp
  (lambda ()
    (bal/call "bal/change-stock-price"
              (list
               (cons "To Account" "current_account")
               (cons "From Account" "account")
               (cons "Stock Price" "real")
               (cons "Number of Shares" "real")
               (cons "Day" "day")))))

(define cal
  (lambda* (#:optional args)
    (let ((cmd (string-append "cal "
                              (if args args ""))))
      (system cmd))))

