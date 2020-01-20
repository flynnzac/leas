(use-modules (ice-9 regex))
(use-modules (ice-9 textual-ports))

(define README-text
  (get-string-all (open-file "../README" "r")))

(define regex-README "(\n)[^leas]*(leas)")

(set! README-text
  (regexp-substitute #f
                     (string-match regex-README README-text)
                     'pre 1 2 'post))

(let ((output-port (open-file "../README.md" "w")))
  (display "Leas is an interactive, command-line program for managing personal finances.  " output-port)
  (display "I have been using it to manage mine for about a year and a half now so it has been well-used... there shouldn't be too many bugs.  Let me know if you find any issues!\n\n"
           output-port)
  (display "To see a full manual for Leas in a tutorial format, see: zflynn.com/leas/index.html\n\n"
           output-port)
  (display "The manpage is replicated below to give an overview of the basic commands.  See the file INSTALL for installation instructions.\n\n```\n"
           output-port)
  (display README-text output-port)
  (display "\n```\n" output-port)
  (close-port output-port))



