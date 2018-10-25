;; bal standard library

(use-modules (srfi srfi-19))
(define bal-sl/amount-of-transacts
  (lambda (x)
    (map (lambda (y) (list-ref y 1)) x)))

(define bal-sl/compare-tscts-to-day
  (lambda (tsct day)
    (let ((year (cons (list-ref tsct 2)
                      (list-ref day 2)))
          (month (cons (list-ref tsct 3)
                       (list-ref day 1)))
          (day (cons (list-ref tsct 4)
                     (list-ref day 0))))
      (cond ((> (car year) (cdr year)) 1)
            ((< (car year) (cdr year)) -1)
            ((> (car month) (cdr month)) 1)
            ((< (car month) (cdr month)) -1)
            ((> (car day) (cdr day)) 1)
            ((< (car day) (cdr day)) -1)
            (#t 0)))))


(define bal-sl/sum-transact-over-days
  (lambda (x first-day last-day)
    (apply + (map
              (lambda (y)
                (if (and (>= (bal-sl/compare-tscts-to-day y first-day) 0)
                         (< (bal-sl/compare-tscts-to-day y last-day) 0))
                    (list-ref y 1)
                    0)) x))))

(define bal-sl/day-from-time
  (lambda (x)
    (let ((xdate (time-utc->date x 0)))
      (list (date-day xdate) (date-month xdate) (date-year xdate)))))


(define bal-sl/seq-days
  (lambda (first-day last-day by)
    (let* ((first-time (date->time-utc (make-date 0 0 0 0
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
          (cons first-day (bal-sl/seq-days 
                           (bal-sl/day-from-time
                            (add-duration first-time
                                          (make-time time-duration 0 (* 24 3600 by))))
                           last-day
                           by))))))

(define bal-sl/sum-transact-over-sequence
  (lambda (x day-sequence)
    (if (= (length day-sequence) 2)
        (list (bal-sl/sum-transact-over-days x (list-ref day-sequence 0)
                                             (list-ref day-sequence 1)))
        (cons (bal-sl/sum-transact-over-days x (list-ref day-sequence 0)
                                             (list-ref day-sequence 1))
              (bal-sl/sum-transact-over-sequence x (cdr day-sequence))))))

(define bal-sl/total-transact-over-days
  (lambda (x first-day last-day)
    (let* ((days (bal-sl/seq-days first-day last-day 1))
           (totals (bal-sl/sum-transact-over-sequence x days)))
      (let jn-day ((i 0))
        (let ((cur-day (list-ref days i)))
          (if (= i (- (length totals) 1))
              (list (cons (list-ref totals i)
                          (string-append
                           (number->string (list-ref cur-day 2))
                           "-"
                           (format #f "~2,'0d" (list-ref cur-day 1))
                           "-"
                           (format #f "~2,'0d" (list-ref cur-day 0)))))
              (cons (cons (list-ref totals i)
                          (string-append
                           (number->string (list-ref cur-day 2))
                           "-"
                           (format #f "~2,'0d" (list-ref cur-day 1))
                           "-"
                           (format #f "~2,'0d" (list-ref cur-day 0))))
                    (jn-day (+ i 1)))))))))

(define bal-sl/total-transact-in-account-over-days
  (lambda (account first-day last-day)
    (bal-sl/total-transact-over-days
     (bal/get-all-transactions account)
     first-day
     last-day)))

(define ttod
  (lambda ()
    (let ((result (bal/call
                   "bal-sl/total-transact-in-account-over-days"
                   (list
                    (cons "Account" "current_account")
                    (cons "First day" "day")
                    (cons "Last day" "day")))))
      (map-in-order
       (lambda (x)
         (display
          (string-append
           (cdr x)
           (format #f "~10,2f" (car x))
           "\n")))
       result))))
