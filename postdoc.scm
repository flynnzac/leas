(use-modules (ice-9 regex))
(use-modules (ice-9 textual-ports))

(define regex-text "(<!--)")
(define main-text
  (get-string-all (open-file "docs/baldoc.html" "r")))

(set! main-text
  (regexp-substitute #f
                     (string-match regex-text main-text)
                     1 'post))

(let ((output-port (open-file "docs/baldoc.html" "w")))
  (display main-text output-port)
  (close-port output-port))


(define README-text
  (get-string-all (open-file "README" "r")))

(define regex-README "(---\n)[^bal]*(bal)")

(set! README-text
  (regexp-substitute #f
                     (string-match regex-README README-text)
                     'pre 1 2 'post))

(let ((output-port (open-file "README" "w")))
  (display README-text output-port)
  (close-port output-port))



