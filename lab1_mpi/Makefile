CXX      := mpic++
CXXFLAGS := -O2 -std=c++17 -Wall
TARGET   := array_sum_mpi
SRC      := src/array_sum_mpi.cpp

.PHONY: all clean run check-env

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

run: $(TARGET)
	mpirun -np 4 ./$(TARGET) 1000000

check-env:
	@command -v mpic++ >/dev/null 2>&1 || { \
		echo "ERROR: mpic++ not found. Run: brew install open-mpi"; exit 1; }
	@echo "OK: $$(mpic++ --version | head -n 1)"
	@echo "OK: $$(mpirun --version | head -n 1)"

clean:
	rm -f $(TARGET)
	rm -rf results/*.txt results/*.csv
