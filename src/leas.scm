(use-modules (ice-9 format))
(use-modules (srfi srfi-1))
(use-modules (srfi srfi-19))

;; Parameters
(define leas/number-to-quick-list 20)
(define leas/prompt
  (lambda ()
    (if (= (leas/get-number-of-accounts) 0)
        ":> "
        (string-append
         "("
         (leas/get-current-account)
         ") :> "))))


;; Non-interactive functions

;;; Utility Functions

;;;; Pretty print transactions
(define leas/print-tscts
  (lambda (tsct-list)
    (if (list? tsct-list)
        (let ((len (apply max
                          (map (lambda (x)
                                 (string-length (car x))) tsct-list))))
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
           tsct-list)))))

;;;; Create a (day month year) list from a time object.
(define leas/day-from-time
  (lambda (x)
    (let ((xdate (time-utc->date x 0)))
      (list (date-day xdate) (date-month xdate) (date-year xdate)))))

;;;; Creates a list of days
(define leas/seq-days
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
          (cons first-day (leas/seq-days 
                           (leas/day-from-time
                            (add-duration first-time
                                          (make-time time-duration 0 (* 24 3600 by))))
                           last-day
                           by))))))


;;; Add and Delete functions

;;;; Transfer From one account to another account
(define leas/t
  (lambda (to-account from-account amount desc day)
    (leas/at to-account amount desc day)
    (leas/at from-account (* -1 amount) desc day)))

;;;; Delete transfer from one account to another 
(define leas/dtr
  (lambda (from-account location)
    (let ((transaction
           (leas/get-transaction-by-location (car location)
                                             (cdr location)))
          (all-from (leas/get-all-transactions from-account))
          (num-from (leas/get-account-location from-account)))
      (leas/dt (cons num-from
                     (list-index
                      (lambda (u)
                        (and (string=? (car u) (car transaction))
                             (lset= (lambda (v w) (= (abs v) (abs w)))
                                    (cdr u) (cdr transaction)))) all-from)))
      (leas/dt location))))

;;;; Pay a loan by deducting the pricipal from the loan account,
;;;; adding the interest to the interest-account (an Expense account),
;;;; and deducting the total from an asset or another liability
;;;; account (from-account)
(define leas/pay-loan
  (lambda (loan-account interest-account from-account
                        principal interest desc day)
    (leas/at loan-account principal desc day)
    (leas/at interest-account interest desc day)
    (leas/at from-account (* -1 (+ principal interest)) desc day)))

;;;; Update balances in "account" after a change in stock price by
;;;; moving money from an Income account (like "Stock Income") to the
;;;; account
(define leas/change-stock-price
  (lambda (account from-account stock-price number day)
    (let* ((value (list-ref (leas/total-account account) 1))
           (newval (* number stock-price))
           (trval (- newval value)))
      (leas/t account from-account trval
              "Stock Price Change" day))))

;;; Modification functions

;;;; Edit a transaction by deleting it and replacing it with something
;;;; new
(define leas/edit-transact
  (lambda (tsct-loc day amount desc)
    (let ((tsct (leas/get-transaction-by-location
                 (car tsct-loc) (cdr tsct-loc))))
      (leas/dt tsct-loc)
      (leas/at (car (leas/get-account-by-location (car tsct-loc)))
               (if (string-null? amount)
                   (list-ref tsct 1)
                   (string->number amount))
               (if (string-null? desc)
                   (list-ref tsct 0)
                   desc)
               day))))

;;; Summary functions

;;;; Print out totals of a list of accounts
(define leas/display-account-totals
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

;;;; Display totals for all accounts of a certain type
(define leas/current-total-of-type
  (lambda (n)
    (lambda ()
      (let* ((accts (leas/total-by-account-type))
             (el (cdr (list-ref accts n))))
        (display (string-append
                  (format #f "~,2f"
                          (if (pair? el)
                              (car el)
                              el))
                  "\n"))))))

;;; Over Time functions

;;;; A syntax to apply a function for each day
(define-syntax leas/loop-days
  (lambda (x)
    (syntax-case x ()
      ((_ days current-day val exp)
       (with-syntax ((i (datum->syntax x (quote i))))
                    (syntax (let loop-day ((i 0))
                              (if (< i val)
                                  (begin
                                    (leas/set-current-day (list-ref days i))
                                    (cons (cons (list-ref days i)
                                                exp)
                                          (loop-day (+ i 1))))
                                  (begin
                                    (leas/set-current-day current-day)
                                    (list))))))))))

(define leas/balance-account-on-days
  (lambda (first-day last-day by account)
    (let ((days (leas/seq-days first-day last-day by))
          (current-day (leas/get-current-day)))
      (leas/loop-days days current-day (length days)
                      (list-ref (leas/total-account account) 1)))))

(define leas/total-transact-in-account-between-days
  (lambda (first-day last-day by account)
    (let* ((balance (leas/balance-account-on-days
                     first-day last-day by account))
           (current-day (leas/get-current-day))
           (days (map car balance)))
      (leas/loop-days days current-day (- (length balance) 1)
                      (- (cdr (list-ref balance (+ i 1)))
                         (cdr (list-ref balance i)))))))

(define leas/total-transact-in-account-re
  (lambda (first-day last-day by account regex)
    (let ((current-day (leas/get-current-day))
          (days (leas/seq-days first-day last-day by)))
      (leas/loop-days days current-day (length days)
                      (apply +
                             (map (lambda (u) (list-ref u 1))
                                  (leas/get-transactions-by-regex account regex)))))))

(define leas/output-by-day
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

(define leas/get-by-type-over-days
  (lambda (first-day last-day by num)
    (let ((days (leas/seq-days first-day last-day by))
          (current-day (leas/get-current-day)))
      (leas/loop-days days current-day (length days)
                      (list-ref
                       (list-ref (leas/total-by-account-type) num)
                       1)))))

(define leas/get-by-type-over-days-for-type
  (lambda (n)
    (lambda (first-day last-day by)
      (if (string=? by "")
          (leas/get-by-type-over-days first-day last-day 7 n)
          (leas/get-by-type-over-days first-day last-day (string->number by) n)))))

(define-syntax leas/over-day-cmd
  (syntax-rules ()
    ((over-day-cmd val)
     (let ((result (leas/call
                    (string-append
                     "(leas/get-by-type-over-days-for-type "
                     (number->string val)
                     ")")
                    (list
                     (cons "First Day" "day")
                     (cons "Last Day" "day")
                     (cons "By [7]" "string")))))
       (map-in-order
        (lambda (x)
          (leas/output-by-day (car x) (cdr x)))
        result)))))



;; interactive functions

(define aa
  (lambda ()
    (leas/call "leas/aa"
               (list
                (cons "Account" "string")
                (cons "Type" "type")
                (cons "Opening Balance" "real")))))

(define at
  (lambda ()
    (leas/call "leas/at"
               (list
                (cons "Account" "current_account")
                (cons "Amount" "real")
                (cons "Description" "string")
                (cons "Day" "day")))))



(define ltn
  (lambda ()
    (let ((tscts (leas/call "leas/get-transactions"
                            (list
                             (cons "Account" "current_account")
                             (cons "How many?" "integer")))))
      (leas/print-tscts tscts))))


(define et
  (lambda* (#:optional n)
           (if n (leas/set-select-transact-num n))
           (leas/call "leas/edit-transact"
                      (list
                       (cons "Transaction" "transaction")
                       (cons "Day (default is current day)" "day")
                       (cons "Amount" "string")
                       (cons "Description" "string")))))

(define lt
  (lambda ()
    (let* ((tscts (leas/get-transactions-by-day
                   (leas/get-current-account)
                   (list 0 0 0)
                   (leas/get-current-day)))
           (final-tsct (list-tail tscts
                                  (max (- (length tscts)
                                          leas/number-to-quick-list) 0))))
      (if (and (list? final-tsct)
               (not (null? final-tsct)))
          (leas/print-tscts final-tsct)))))

(define ea
  (lambda ()
    (leas/call "leas/ea"
               (list
                (cons "Account" "account")
                (cons "New account name" "string")
                (cons "Opening balance" "string")))))

(define da
  (lambda ()
    (leas/call "leas/da"
               (list
                (cons "Account" "account")))))

(define dt
  (lambda ()
    (leas/call "leas/dt"
               (list
                (cons "Transaction" "transaction")))))

(define la
  (lambda ()
    (leas/display-account-totals (leas/total-all-accounts))))

(define lae
  (lambda ()
    (leas/display-account-totals
     (leas/total-all-accounts-of-type 1))))

(define lai
  (lambda ()
    (leas/display-account-totals
     (leas/total-all-accounts-of-type 2))))

(define laa
  (lambda ()
    (leas/display-account-totals
     (leas/total-all-accounts-of-type 4))))

(define lal
  (lambda ()
    (leas/display-account-totals
     (leas/total-all-accounts-of-type 8))))


(define bt
  (lambda ()
    (let* ((accts (leas/total-by-account-type))
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

(define cex (leas/current-total-of-type 0))
(define cin (leas/current-total-of-type 1))
(define cas (leas/current-total-of-type 2))
(define cli (leas/current-total-of-type 3))
(define cwo (leas/current-total-of-type 4))
(define cba (leas/current-total-of-type 5))

(define re
  (lambda ()
    (leas/print-tscts
     (leas/call "leas/get-transactions-by-regex"
                (list
                 (cons "Account" "current_account")
                 (cons "Regular Expression" "string"))))))

(define sa
  (lambda ()
    (leas/call "leas/set-account"
               (list
                (cons "Account" "account")))))

(define ca
  (lambda ()
    (display
     (string-append (leas/get-current-account) "\n"))))

(define w
  (lambda ()
    (leas/call "leas/write"
               (list
                (cons "File" "string")))))

(define r
  (lambda ()
    (leas/call "leas/read"
               (list
                (cons "File" "string")))))

(define t
  (lambda ()
    (leas/call "leas/t"
               (list
                (cons "To Account" "account")
                (cons "From Account" "account")
                (cons "Amount" "real")
                (cons "Description" "string")
                (cons "Day" "day")))))

(define dtr
  (lambda ()
    (leas/call "leas/dtr"
               (list
                (cons "From Account" "account")
                (cons "Transaction" "transaction")))))


(define ltbd
  (lambda ()
    (leas/print-tscts
     (leas/call "leas/get-transactions-by-day"
                (list
                 (cons "Account" "current_account")
                 (cons "From Day" "day")
                 (cons "To Day" "day"))))))

(define v
  (lambda ()
    (display
     (string-append
      "Leas version: "
      (leas/v)
      "\n"))))

(define sd
  (lambda ()
    (leas/call "leas/set-current-day"
               (list
                (cons "Current Day" "day")))))

(define cd
  (lambda ()
    (let ((current-day (leas/get-current-day)))
      (display (string-append
                (number->string (list-ref current-day 2))
                "-"
                (format #f "~2,'0d" (list-ref current-day 1))
                "-"
                (format #f "~2,'0d" (list-ref current-day 0))
                "\n")))))

(define baod
  (lambda ()
    (let ((result (leas/call "leas/balance-account-on-days"
                             (list
                              (cons "First Day" "day")
                              (cons "Last Day" "day")
                              (cons "By" "number")
                              (cons "Account" "current_account")))))
      (map-in-order
       (lambda (x)
         (leas/output-by-day (car x) (cdr x)))
       result))))

(define exod
  (lambda ()
    (leas/over-day-cmd 0)))

(define inod
  (lambda ()
    (leas/over-day-cmd 1)))

(define asod
  (lambda ()
    (leas/over-day-cmd 2)))

(define liod
  (lambda ()
    (leas/over-day-cmd 3)))

(define wood
  (lambda ()
    (leas/over-day-cmd 4)))

(define ttbd
  (lambda ()
    (let ((result (leas/call
                   "leas/total-transact-in-account-between-days"
                   (list
                    (cons "First day" "day")
                    (cons "Last day" "day")
                    (cons "By" "number")
                    (cons "Account" "current_account")))))
      (map-in-order
       (lambda (x)
         (leas/output-by-day (car x) (cdr x)))
       result))))

(define ttre
  (lambda ()
    (let ((result (leas/call
                   "leas/total-transact-in-account-re"
                   (list
                    (cons "First day" "day")
                    (cons "Last day" "day")
                    (cons "By" "number")
                    (cons "Account" "current_account")
                    (cons "Regex" "string")))))
      (map-in-order
       (lambda (x)
         (leas/output-by-day (car x) (cdr x)))
       result))))


(define pl
  (lambda ()
    (leas/call "leas/pay-loan"
               (list
                (cons "Loan Account" "liability_account")
                (cons "Interest Account" "expense_account")
                (cons "Pay from Account" "pay_from_account")
                (cons "Principal" "real")
                (cons "Interest" "real")
                (cons "Description" "string")
                (cons "Day" "day")))))

(define fn
  (lambda ()
    (display
     (string-append (leas/get-current-file) "\n"))))


(define csp
  (lambda ()
    (leas/call "leas/change-stock-price"
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


(define spend
  (lambda ()
    (leas/call "leas/t"
	             (list
	              (cons "To Account" "expense_account")
	              (cons "From Account" "asset_account")
	              (cons "Amount" "real")
	              (cons "Description" "string")
	              (cons "Day" "day")))))

(define charge
  (lambda ()
    (leas/call "leas/t"
	             (list
	              (cons "To Account" "expense_account")
	              (cons "From Account" "liability_account")
	              (cons "Amount" "real")
	              (cons "Description" "string")
	              (cons "Day" "day")))))


(define earn
  (lambda ()
    (leas/call "leas/t"
	             (list
	              (cons "To Account" "asset_account")
	              (cons "From Account" "income_account")
	              (cons "Amount" "real")
	              (cons "Description" "string")
	              (cons "Day" "day")))))

(define borrow
  (lambda ()
    (leas/call "leas/t"
	             (list
	              (cons "To Account" "asset_account")
	              (cons "From Account" "liability_account")
	              (cons "Amount" "real")
	              (cons "Description" "string")
	              (cons "Day" "day")))))
