all: phys.pdf

%.pdf: %.tex
	pdflatex $<

clean:
	rm -vf *.dvi *.aux *.log *.toc *.out

distclean: clean
	rm -vf phys.pdf

.PHONY: all clean distclean
