set -e
cd "$(dirname "$0")"

if [ ! -x ./array_sum_mpi ]; then
    echo "Бінарника немає — виконую make..."
    make
fi

mkdir -p results

# params
EXEC=./array_sum_mpi
SIZES=(1000000 10000000)               
PROCS=(1 2 4 8)                       
REPS=3                                 

CSV=results/results.csv
echo "N,P,T_serial,T_parallel_avg,T_parallel_min,T_parallel_max,Speedup,Efficiency_percent" > "$CSV"

for N in "${SIZES[@]}"; do
    for P in "${PROCS[@]}"; do
        echo ""
        echo "============================================="
        echo "  Експеримент: N=$N, P=$P"
        echo "============================================="
        LOG="results/run_N${N}_P${P}.log"

        mpirun --oversubscribe -np "$P" "$EXEC" "$N" "$REPS" results | tee "$LOG"

        grep "^CSV," "$LOG" | sed 's/^CSV,//' >> "$CSV"
    done
done

echo ""
echo "============================================="
echo " Всі експерименти завершено."
echo " Зведена таблиця: $CSV"
echo "============================================="
echo ""
column -t -s, "$CSV"
