# docs - the LaTeX/TikZ derivation behind ../CONCEPTS.md.
#
# Dependencies (TeX Live packages, Arch names): pdflatex with amsmath and
# geometry (texlive-latex), booktabs (texlive-latexrecommended), enumitem
# (texlive-latexextra) and tikz/pgf (texlive-pictures):
#   pacman -S texlive-latex texlive-latexrecommended \
#             texlive-latexextra texlive-pictures
# Read the result with pdftotext(1) - there is no X server here, so there is
# no viewer rule.

PDFLATEX   ?= pdflatex
LATEXFLAGS := -interaction=nonstopmode -halt-on-error

TEX := phys.tex
PDF := $(TEX:.tex=.pdf)

all: $(PDF)

# Two passes on purpose: the first writes the \eqref/\ref targets into the
# .aux, the second resolves them and settles the page breaks.  latexmk would
# iterate by itself, but it is not installed; two passes is what this document
# actually needs, and it is idempotent.
%.pdf: %.tex
	$(PDFLATEX) $(LATEXFLAGS) $<
	$(PDFLATEX) $(LATEXFLAGS) $<

clean:
	rm -vf *.dvi *.aux *.log *.toc *.out *.fls *.fdb_latexmk

distclean: clean
	rm -vf $(PDF)

.PHONY: all clean distclean
