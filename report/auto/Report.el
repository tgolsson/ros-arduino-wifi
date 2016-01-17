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
    "eq:et"
    "eq:pid"
    "fig:pid"
    "subsec:cr"
    "eq:position1"
    "eq:position2"
    "fig:robot"
    "fig:wiring"
    "tikz:communicationschema"
    "subsec:cn"
    "subsec:ptn"
    "subsec:dd"
    "subsec:pidt")
   (LaTeX-add-bibliographies
    "references.bib")))

