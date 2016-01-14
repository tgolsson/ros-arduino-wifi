(TeX-add-style-hook
 "Report"
 (lambda ()
   (TeX-add-to-alist 'LaTeX-provided-class-options
                     '(("article" "11pt")))
   (TeX-run-style-hooks
    "latex2e"
    "preamble"
    "article"
    "art11")
   (LaTeX-add-labels
    "subsec:cr"
    "subsec:cn"
    "subsec:ptn"
    "subsec:dd"
    "subsec:pidt")
   (LaTeX-add-bibliographies
    "references.bib")))

