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

