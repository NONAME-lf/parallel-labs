set -e
cd "$(dirname "$0")"

if [ ! -x ./array_sum_openmp ]; then
    echo "Бінарника немає — виконую make..."
    make
fi

mkdir -p results

EXEC=./array_sum_openmp
SIZES=(1000000 10000000)              # 2 набори даних: 10^6 та 10^7
THREADS=(1 2 4 8)
REPS=3

CSV=results/results.csv
echo "N,T,T_serial,T_parallel_avg,T_parallel_min,T_parallel_max,Speedup,Efficiency_percent" > "$CSV"

for N in "${SIZES[@]}"; do
    for T in "${THREADS[@]}"; do
        echo ""
        echo "============================================="
        echo "  Експеримент: N=$N, T=$T потоків"
        echo "============================================="
        LOG="results/run_N${N}_T${T}.log"

        "$EXEC" "$N" "$T" "$REPS" results | tee "$LOG"

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
