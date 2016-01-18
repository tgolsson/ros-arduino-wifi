(TeX-add-style-hook
 "Report"
 (lambda ()
   (TeX-add-to-alist 'LaTeX-provided-class-options
                     '(("article" "11pt")))
   (add-to-list 'LaTeX-verbatim-environments-local "lstlisting")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "path")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "url")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "nolinkurl")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "hyperbaseurl")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "hyperimage")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "hyperref")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "lstinline")
   (add-to-list 'LaTeX-verbatim-macros-with-delims-local "path")
   (add-to-list 'LaTeX-verbatim-macros-with-delims-local "url")
   (add-to-list 'LaTeX-verbatim-macros-with-delims-local "lstinline")
   (TeX-run-style-hooks
    "latex2e"
    "preamble"
    "article"
    "art11")
   (LaTeX-add-labels
    "eq:et"
    "eq:pid"
    "fig:pid"
    "sec:protocol"
    "sec:odometrymath"
    "eq:position1"
    "eq:position2"
    "fig:robot"
    "fig:wiring"
    "tikz:communicationschema"
    "subsec:cn"
    "subsec:ptn"
    "subsec:dd"
    "eq:slidingwindow"
    "subsec:pidt"
    "tab:pid"
    "sec:odometry"
    "tab:ccw"
    "tab:cw"
    "tab:straight"
    "tab:singleturn")
   (LaTeX-add-bibliographies
    "references.bib")))

