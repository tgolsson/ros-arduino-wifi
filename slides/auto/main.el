(TeX-add-style-hook
 "main"
 (lambda ()
   (TeX-add-to-alist 'LaTeX-provided-class-options
                     '(("beamer" "12pt")))
   (TeX-add-to-alist 'LaTeX-provided-package-options
                     '(("inputenc" "utf8")))
   (add-to-list 'LaTeX-verbatim-environments-local "lstlisting")
   (add-to-list 'LaTeX-verbatim-environments-local "semiverbatim")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "lstinline")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "hyperref")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "hyperimage")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "hyperbaseurl")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "nolinkurl")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "url")
   (add-to-list 'LaTeX-verbatim-macros-with-braces-local "path")
   (add-to-list 'LaTeX-verbatim-macros-with-delims-local "lstinline")
   (add-to-list 'LaTeX-verbatim-macros-with-delims-local "url")
   (add-to-list 'LaTeX-verbatim-macros-with-delims-local "path")
   (TeX-run-style-hooks
    "latex2e"
    "beamer"
    "beamer12"
    "amsfonts"
    "amsmath"
    "amssymb"
    "array"
    "calc"
    "cancel"
    "caption"
    "color"
    "datatool"
    "dirtree"
    "etoolbox"
    "fancyhdr"
    "graphicx"
    "hyperref"
    "import"
    "inputenc"
    "lipsum"
    "listings"
    "longtable"
    "multicol"
    "marginnote"
    "parskip"
    "perpage"
    "pgfplots"
    "ragged2e"
    "relsize"
    "scalefnt"
    "stackengine"
    "textcomp"
    "tikz"
    "url"
    "varioref")
   (TeX-add-symbols
    "title")
   (LaTeX-add-labels
    "tikz:communication")
   (LaTeX-add-bibliographies
    "references")
   (LaTeX-add-array-newcolumntypes
    "L"
    "C"
    "R")))

