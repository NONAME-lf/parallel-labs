TEX_SRC := Lab2_OpenMP_Report.tex
PDF     := $(TEX_SRC:.tex=.pdf)
TEX     := xelatex
TEXFLAGS := -interaction=nonstopmode -halt-on-error

.PHONY: all clean

all: $(PDF)

$(PDF): $(TEX_SRC)
	$(TEX) $(TEXFLAGS) $<
	$(TEX) $(TEXFLAGS) $< 

clean:
	rm -f *.aux *.log *.out *.toc *.fdb_latexmk *.fls *.synctex.gz
	rm -f $(PDF)
