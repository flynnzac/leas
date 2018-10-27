(use-modules (ice-9 regex))
(use-modules (ice-9 textual-ports))

(define regex-text "(<!--)")
(define main-text
  (get-string-all (open-file "baldoc.html" "r")))

(set! main-text
  (regexp-substitute #f
                     (string-match regex-text main-text)
                     1 'post))

(let ((output-port (open-file "baldoc.html" "w")))
  (display main-text output-port)
  (close-port output-port))




