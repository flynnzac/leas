
;; these definitions are omitted for privacy, but:
;; define number of shares owned as fzac/vanguard-shares
;; define fund as fzac/mutual-fund

;; the command "vup" will then update the stock price in account "Vanguard" by transferring from
;; income account "Stock Income"

(define fzac/fetch-vanguard-fund-price
  (lambda (day month year fund)
    (let* ((fetch-date (string-append month "%2F" day "%2F" year))
	         (url (string-append
                 "https://personal.vanguard.com/us/funds/tools/pricehistorysearch?radio=1&results=get&FundType=VanguardFunds&FundIntExt=INT&FundId="
                 fund
                 "&Sc=1&fundName="
                 fund
                 "&fundValue="
                 fund
                 "&radiobutton2=1&beginDate=" fetch-date "&endDate=" fetch-date "&year=#res"))
	         (input (open-input-pipe (string-append "wget -O - \"" url "\"")))
           (vanguard-file (get-string-all input))
           (price-block (match:substring
      			             (string-match
      			              "<!--CBD: Table-->(.*)<!--End CBD-->"
      			              vanguard-file) 1))
           (re-price (string-match "\\$([0-9\\.]+)" price-block))
           (price (string->number (match:substring re-price 1))))
      (close-pipe input)
      price)))

(define fzac/day-to-string
  (lambda (day)
    (map-in-order
     (lambda (u)
       (if (< u 10)
           (string-append "0" (number->string u))
           (number->string u)))
     day)))




(define vup
  (lambda ()
    (let* ((day (bal/get-current-day))
           (price (apply
                   fzac/fetch-vanguard-fund-price
                   (append (fzac/day-to-string day)
                           (list fzac/mutual-fund)))))
      (if price
          (bal/change-stock-price
           "Vanguard"
           "Stock Income"
           price
           fzac/vanguard-shares
           day)
          '()))))
     

