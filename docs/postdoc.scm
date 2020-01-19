(use-modules (ice-9 regex))
(use-modules (ice-9 textual-ports))

(define README-text
  (get-string-all (open-file "../README" "r")))

(define regex-README "(\n)[^leas]*(leas)")

(set! README-text
  (regexp-substitute #f
                     (string-match regex-README README-text)
                     'pre 1 2 'post))

(let ((output-port (open-file "../README" "w")))
  (display "To see a full manual for Leas in a tutorial format, see: zflynn.com/leas/index.html\n" output-port)
  (display README-text output-port)
  (close-port output-port))



