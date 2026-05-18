TEX_SRC := Lab1_MPI_Report.tex
PDF     := $(TEX_SRC:.tex=.pdf)
TEX     := xelatex
TEXFLAGS := -interaction=nonstopmode -halt-on-error

.PHONY: all clean

all: $(PDF)

$(PDF): $(TEX_SRC)
	$(TEX) $(TEXFLAGS) $<
	$(TEX) $(TEXFLAGS) $<  # двічі — для коректних cross-references

clean:
	rm -f *.aux *.log *.out *.toc *.fdb_latexmk *.fls *.synctex.gz
	rm -f $(PDF)
