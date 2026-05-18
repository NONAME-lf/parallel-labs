#include <omp.h>
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
    const long long N = static_cast<long long>(a.size());
    for (long long i = 0; i < N; ++i) {
        sum += heavy_func(a[i]);
    }
    return sum;
}

// parallel with OpenMP
static double parallel_sum(const std::vector<double>& a, int nthreads) {
    double sum = 0.0;
    const long long N = static_cast<long long>(a.size());

    #pragma omp parallel for reduction(+:sum) \
                             num_threads(nthreads) \
                             schedule(static)
    for (long long i = 0; i < N; ++i) {
        sum += heavy_func(a[i]);  
    }
    return sum;
}

static void make_dir(const std::string& path) {
    mkdir(path.c_str(), 0755);
}

int main(int argc, char** argv) {
    // params
    long long N         = (argc > 1) ? std::atoll(argv[1]) : 1000000;
    int nthreads        = (argc > 2) ? std::atoi(argv[2])
                                     : omp_get_max_threads();
    int reps            = (argc > 3) ? std::atoi(argv[3])  : 3;
    std::string out_dir = (argc > 4) ? std::string(argv[4]) : "results";

    if (N <= 0 || nthreads <= 0) {
        std::cerr << "Usage: " << argv[0]
                  << " <N> [num_threads] [reps] [output_dir]\n";
        return 1;
    }

    // init of array, serial
    std::vector<double> array(N);
    {
        std::mt19937 rng(42);
        std::uniform_real_distribution<double> dist(0.0, 100.0);
        for (long long i = 0; i < N; ++i) array[i] = dist(rng);
    }

    // report
    make_dir(out_dir);
    {
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
    }

    // srial 
    std::vector<double> ser_times;
    double serial_result = 0.0;
    for (int r = 0; r < reps; ++r) {
        double t1 = omp_get_wtime();
        double s  = serial_sum(array);
        double t2 = omp_get_wtime();
        ser_times.push_back(t2 - t1);
        if (r == 0) serial_result = s;
    }
    double serial_time = 0;
    for (double t : ser_times) serial_time += t;
    serial_time /= reps;

    std::vector<double> par_times;
    double parallel_result = 0.0;
    for (int r = 0; r < reps; ++r) {
        double t1 = omp_get_wtime();
        double s  = parallel_sum(array, nthreads);
        double t2 = omp_get_wtime();
        par_times.push_back(t2 - t1);
        if (r == 0) parallel_result = s;
    }

    double par_avg = 0, par_min = par_times[0], par_max = par_times[0];
    for (double t : par_times) {
        par_avg += t;
        if (t < par_min) par_min = t;
        if (t > par_max) par_max = t;
    }
    par_avg /= reps;

    // metrics
    double speedup    = serial_time / par_avg;
    double efficiency = speedup / nthreads * 100.0;
    double abs_error  = std::fabs(parallel_result - serial_result);
    double rel_error  = abs_error / std::fabs(serial_result);

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "=== OpenMP: Sum of f(a[i]) ===\n";
    std::cout << "N=" << N << "  T=" << nthreads
              << "  reps=" << reps << "  HEAVY_K=" << HEAVY_K << "\n";
    std::cout << "Serial   time (s) : " << serial_time << "\n";
    std::cout << "Parallel time (s) : " << par_avg
              << "   [min=" << par_min << ", max=" << par_max << "]\n";
    std::cout << "Speedup    S(T)   : " << speedup << "\n";
    std::cout << "Efficiency E(T) % : " << efficiency << "\n";
    std::cout << "Verification      : abs_err=" << abs_error
              << "  rel_err=" << rel_error << "\n";

    std::cout << "CSV," << N << "," << nthreads << ","
              << serial_time << "," << par_avg << ","
              << par_min << "," << par_max << ","
              << speedup << "," << efficiency << "\n";

    std::ostringstream fname;
    fname << out_dir << "/output_N" << N << "_T" << nthreads << ".txt";
    std::ofstream fout(fname.str());
    fout << std::fixed << std::setprecision(8);
    fout << "================================================\n";
    fout << " OpenMP: Sum of f(a[i]) — Output Report\n";
    fout << "================================================\n";
    fout << "Parameters:\n";
    fout << "  N (array size)      = " << N << "\n";
    fout << "  T (threads)         = " << nthreads << "\n";
    fout << "  HEAVY_K (work/elem) = " << HEAVY_K << "\n";
    fout << "  Repetitions         = " << reps << "\n\n";

    fout << "Iteration distribution (schedule=static):\n";
    long long chunk = N / nthreads;
    long long rem   = N % nthreads;
    fout << "  Each thread gets ~" << chunk << " iterations\n";
    if (rem != 0) {
        fout << "  Перші " << rem << " потоків отримують на 1 ітерацію більше.\n";
    } else {
        fout << "  N кратне T — усі блоки однакові.\n";
    }
    fout << "\n";

    fout << "Results:\n";
    fout << "  Sum (serial)   = " << serial_result << "\n";
    fout << "  Sum (parallel) = " << parallel_result << "\n";
    fout << "  Abs error      = " << abs_error << "\n";
    fout << "  Rel error      = " << rel_error << "\n\n";

    fout << "Timings (seconds):\n";
    fout << "  Serial runs    : ";
    for (size_t i = 0; i < ser_times.size(); ++i) {
        fout << ser_times[i];
        if (i + 1 < ser_times.size()) fout << ", ";
    }
    fout << "\n";
    fout << "  Serial avg     : " << serial_time << "\n";
    fout << "  Parallel runs  : ";
    for (size_t i = 0; i < par_times.size(); ++i) {
        fout << par_times[i];
        if (i + 1 < par_times.size()) fout << ", ";
    }
    fout << "\n";
    fout << "  Parallel avg   : " << par_avg << "\n";
    fout << "  Parallel min/max: " << par_min << " / " << par_max << "\n\n";

    fout << "Metrics:\n";
    fout << "  Speedup    S(T) = T_s / T_p = " << speedup << "\n";
    fout << "  Efficiency E(T) = S(T) / T  = " << efficiency << " %\n";
    fout.close();

    return 0;
}
