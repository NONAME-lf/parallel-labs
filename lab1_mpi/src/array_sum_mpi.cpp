#include <mpi.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <random>
#include <string>
#include <sys/stat.h>

// number of heavy operations per element
static constexpr int HEAVY_K = 50;

// heavy computation to make the workload per element significant and more realistic for parallelization
static inline double heavy_func(double x) {
    double result = 0.0;
    for (int k = 1; k <= HEAVY_K; ++k) {
        result += std::sin(x + k) * std::cos(x - k)
                  / (1.0 + std::sqrt(std::fabs(x) + k));
    }
    return result;
}

// serial
static double serial_sum(const std::vector<double>& a) {
    double sum = 0.0;
    const size_t n = a.size();
    for (size_t i = 0; i < n; ++i) {
        sum += heavy_func(a[i]);
    }
    return sum;
}

static void make_dir(const std::string& path) {
    mkdir(path.c_str(), 0755); 
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // params
    long long N    = (argc > 1) ? std::atoll(argv[1])  : 1000000;
    int       reps = (argc > 2) ? std::atoi(argv[2])   : 3;
    std::string out_dir = (argc > 3) ? std::string(argv[3]) : "results";

    if (N < (long long)size) {
        if (rank == 0)
            std::cerr << "Error: N=" << N << " < P=" << size << "\n";
        MPI_Finalize();
        return 1;
    }

    // init array on 
    std::vector<double> array;
    if (rank == 0) {
        array.resize(N);
        std::mt19937 rng(42);  // seed for reproducibility
        std::uniform_real_distribution<double> dist(0.0, 100.0);
        for (long long i = 0; i < N; ++i) {
            array[i] = dist(rng);
        }

        make_dir(out_dir);
        std::ofstream fin(out_dir + "/input_sample.txt");
        fin << "# Вхідні дані: масив a[0..N-1] випадкових double\n";
        fin << "# N = " << N << "\n";
        fin << "# Seed RNG = 42 — фіксований для відтворюваності\n";
        fin << "# Діапазон значень: [0, 100)\n";
        fin << "# Нижче — перші 20 елементів масиву:\n\n";
        fin << std::fixed << std::setprecision(6);
        for (int i = 0; i < std::min<long long>(20, N); ++i) {
            fin << "a[" << i << "] = " << array[i] << "\n";
        }
        fin << "\n# ... (всього " << N << " елементів)\n";
        fin.close();
    }

    
    std::vector<int> counts(size), displs(size);
    long long base = N / size;
    long long rem  = N % size;
    int offset = 0;
    for (int i = 0; i < size; ++i) {
        counts[i] = static_cast<int>(base + (i < rem ? 1 : 0));
        displs[i] = offset;
        offset += counts[i];
    }
    std::vector<double> local(counts[rank]);

// serial
    double serial_time = 0.0;
    double serial_result = 0.0;
    if (rank == 0) {
        std::vector<double> times;
        for (int r = 0; r < reps; ++r) {
            double t1 = MPI_Wtime();
            double s  = serial_sum(array);
            double t2 = MPI_Wtime();
            times.push_back(t2 - t1);
            if (r == 0) serial_result = s;
        }
        for (double t : times) serial_time += t;
        serial_time /= reps;
    }

    MPI_Barrier(MPI_COMM_WORLD);  // sync before parallel section

// parallel
    std::vector<double> par_times;
    double parallel_result = 0.0;

    for (int r = 0; r < reps; ++r) {
        MPI_Barrier(MPI_COMM_WORLD);
        double t_start = MPI_Wtime();

        MPI_Scatterv(rank == 0 ? array.data() : nullptr,
                     counts.data(), displs.data(), MPI_DOUBLE,
                     local.data(), counts[rank], MPI_DOUBLE,
                     0, MPI_COMM_WORLD);

        double local_sum = 0.0;
        for (int i = 0; i < counts[rank]; ++i) {
            local_sum += heavy_func(local[i]);
        }

        double global_sum = 0.0;
        MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM,
                   0, MPI_COMM_WORLD);

        double t_end = MPI_Wtime();
        par_times.push_back(t_end - t_start);

        if (rank == 0 && r == 0) parallel_result = global_sum;
    }

    if (rank == 0) {
        double par_avg = 0, par_min = par_times[0], par_max = par_times[0];
        for (double t : par_times) {
            par_avg += t;
            if (t < par_min) par_min = t;
            if (t > par_max) par_max = t;
        }
        par_avg /= reps;

        double speedup    = serial_time / par_avg;
        double efficiency = speedup / size * 100.0;
        double abs_error  = std::fabs(parallel_result - serial_result);
        double rel_error  = abs_error / std::fabs(serial_result);

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "=== MPI: Sum of f(a[i]) ===\n";
        std::cout << "N=" << N << "  P=" << size
                  << "  reps=" << reps << "  HEAVY_K=" << HEAVY_K << "\n";
        std::cout << "Serial   time (s) : " << serial_time << "\n";
        std::cout << "Parallel time (s) : " << par_avg
                  << "   [min=" << par_min << ", max=" << par_max << "]\n";
        std::cout << "Speedup    S(P)   : " << speedup << "\n";
        std::cout << "Efficiency E(P) % : " << efficiency << "\n";
        std::cout << "Verification      : abs_err=" << abs_error
                  << "  rel_err=" << rel_error << "\n";

        std::cout << "CSV," << N << "," << size << ","
                  << serial_time << "," << par_avg << ","
                  << par_min << "," << par_max << ","
                  << speedup << "," << efficiency << "\n";

        std::ostringstream fname;
        fname << out_dir << "/output_N" << N << "_P" << size << ".txt";
        std::ofstream fout(fname.str());
        fout << std::fixed << std::setprecision(8);
        fout << "================================================\n";
        fout << " MPI: Sum of f(a[i]) — Output Report\n";
        fout << "================================================\n";
        fout << "Parameters:\n";
        fout << "  N (array size)      = " << N << "\n";
        fout << "  P (processes)       = " << size << "\n";
        fout << "  HEAVY_K (work/elem) = " << HEAVY_K << "\n";
        fout << "  Repetitions         = " << reps << "\n\n";

        fout << "Block decomposition (counts/displs):\n";
        for (int i = 0; i < size; ++i) {
            fout << "  rank " << i << " : count = " << counts[i]
                 << ",  displ = " << displs[i] << "\n";
        }
        fout << "  N/P  = " << base << "  N%P = " << rem << "\n";
        if (rem != 0) {
            fout << "  Перші " << rem
                 << " процесів отримують на 1 елемент більше.\n";
        } else {
            fout << "  N кратне P — усі блоки однакові.\n";
        }
        fout << "\n";

        fout << "Results:\n";
        fout << "  Sum (serial)   = " << serial_result << "\n";
        fout << "  Sum (parallel) = " << parallel_result << "\n";
        fout << "  Abs error      = " << abs_error << "\n";
        fout << "  Rel error      = " << rel_error << "\n\n";

        fout << "Timings (seconds):\n";
        fout << "  Serial   (avg of " << reps << " runs): "
             << serial_time << "\n";
        fout << "  Parallel runs                    : ";
        for (size_t i = 0; i < par_times.size(); ++i) {
            fout << par_times[i];
            if (i + 1 < par_times.size()) fout << ", ";
        }
        fout << "\n";
        fout << "  Parallel avg                     : " << par_avg << "\n";
        fout << "  Parallel min / max               : "
             << par_min << " / " << par_max << "\n\n";

        fout << "Metrics:\n";
        fout << "  Speedup    S(P) = T_s / T_p = " << speedup << "\n";
        fout << "  Efficiency E(P) = S(P) / P  = "
             << efficiency << " %\n";
        fout.close();
    }

    MPI_Finalize();
    return 0;
}
