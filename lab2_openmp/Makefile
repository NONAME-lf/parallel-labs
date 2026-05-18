CXX ?= clang++
LIBOMP := $(shell brew --prefix libomp 2>/dev/null)

ifeq ($(findstring g++,$(CXX)),g++)
    OMP_CFLAGS  := -fopenmp
    OMP_LDFLAGS := -fopenmp
else
    ifeq ($(LIBOMP),)
        $(error libomp not found. Run: brew install libomp)
    endif
    OMP_CFLAGS  := -Xpreprocessor -fopenmp -I$(LIBOMP)/include
    OMP_LDFLAGS := -L$(LIBOMP)/lib -lomp
endif

CXXFLAGS := -O2 -std=c++17 -Wall $(OMP_CFLAGS)
LDFLAGS  := $(OMP_LDFLAGS)
TARGET   := array_sum_openmp
SRC      := src/array_sum_openmp.cpp

.PHONY: all clean run check-env

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

run: $(TARGET)
	./$(TARGET) 1000000

check-env:
	@command -v $(CXX) >/dev/null 2>&1 || { \
		echo "ERROR: $(CXX) not found"; exit 1; }
	@echo "OK: compiler $(CXX)"
	@if [ -n "$(LIBOMP)" ]; then echo "OK: libomp at $(LIBOMP)"; fi

clean:
	rm -f $(TARGET)
	rm -rf results/*.txt results/*.csv
