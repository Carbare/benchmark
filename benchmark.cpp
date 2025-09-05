#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <fstream>
#include <string>
#include <numeric>
#include <random>
#include <iomanip>
#include <cstdio>

using namespace std;
using namespace std::chrono;

// --- Configuration ---
const int CPU_INTEGER_LIMIT = 300000;
const int CPU_FP_MATRIX_SIZE = 400;
const long long MEM_BUFFER_SIZE = 256 * 1024 * 1024; // 256 MB
const int MEM_RANDOM_ACCESSES = 20000000;
const long long DISK_FILE_SIZE = 128 * 1024 * 1024; // 128 MB
const string DISK_FILE_NAME = "benchmark_temp_file.tmp";

// --- Score Scaling Factors ---
const double SCORE_CPU_INT_FACTOR = 5000.0;
const double SCORE_CPU_FP_FACTOR = 15000.0;
const double SCORE_MEM_SEQ_FACTOR = 20000.0;
const double SCORE_MEM_RAND_FACTOR = 10000.0;
const double SCORE_DISK_IO_FACTOR = 8000.0;

void print_header() {
    cout << "========================================" << endl;
    cout << "==      C++ System Benchmark Tool     ==" << endl;
    cout << "========================================" << endl;
    cout << "Each test will run 5 times for accuracy." << endl << endl;
}

double run_cpu_integer_test_instance() {
    auto start = high_resolution_clock::now();
    long long prime_count = 0;
    for (int i = 2; i < CPU_INTEGER_LIMIT; ++i) {
        bool is_prime = true;
        for (int j = 2; j <= static_cast<int>(sqrt(i)); ++j) {
            if (i % j == 0) {
                is_prime = false;
                break;
            }
        }
        if (is_prime) {
            prime_count++;
        }
    }
    auto end = high_resolution_clock::now();
    duration<double> duration = end - start;
    return duration.count();
}

double run_cpu_floating_point_test_instance() {
    vector<vector<double>> matrixA(CPU_FP_MATRIX_SIZE, vector<double>(CPU_FP_MATRIX_SIZE));
    vector<vector<double>> matrixB(CPU_FP_MATRIX_SIZE, vector<double>(CPU_FP_MATRIX_SIZE));
    vector<vector<double>> matrixC(CPU_FP_MATRIX_SIZE, vector<double>(CPU_FP_MATRIX_SIZE, 0.0));

    for (int i = 0; i < CPU_FP_MATRIX_SIZE; ++i) {
        for (int j = 0; j < CPU_FP_MATRIX_SIZE; ++j) {
            matrixA[i][j] = static_cast<double>(i) + static_cast<double>(j);
            matrixB[i][j] = static_cast<double>(i) - static_cast<double>(j);
        }
    }

    auto start = high_resolution_clock::now();
    for (int i = 0; i < CPU_FP_MATRIX_SIZE; ++i) {
        for (int j = 0; j < CPU_FP_MATRIX_SIZE; ++j) {
            for (int k = 0; k < CPU_FP_MATRIX_SIZE; ++k) {
                matrixC[i][j] += matrixA[i][k] * matrixB[k][j];
            }
        }
    }
    auto end = high_resolution_clock::now();
    duration<double> duration = end - start;
    return duration.count();
}

double run_memory_sequential_test_instance(vector<char>& buffer) {
    auto start = high_resolution_clock::now();
    for (long long i = 0; i < MEM_BUFFER_SIZE; ++i) {
        buffer[i] = static_cast<char>(i % 256);
    }
    volatile char temp;
    for (long long i = 0; i < MEM_BUFFER_SIZE; ++i) {
        temp = buffer[i];
    }
    auto end = high_resolution_clock::now();
    duration<double> duration = end - start;
    return duration.count();
}

double run_memory_random_test_instance(vector<char>& buffer) {
    mt19937 gen(12345); 
    uniform_int_distribution<long long> distrib(0, MEM_BUFFER_SIZE - 1);

    vector<long long> indices(MEM_RANDOM_ACCESSES);
    for(int i = 0; i < MEM_RANDOM_ACCESSES; ++i) {
        indices[i] = distrib(gen);
    }
    
    auto start = high_resolution_clock::now();
    volatile char temp;
    for (int i = 0; i < MEM_RANDOM_ACCESSES; ++i) {
        temp = buffer[indices[i]];
    }
    auto end = high_resolution_clock::now();
    duration<double> duration = end - start;
    return duration.count();
}


double run_disk_io_test_instance() {
    const int buffer_size = 4096;
    vector<char> buffer(buffer_size, 'X');

    auto start = high_resolution_clock::now();
    
    ofstream outFile(DISK_FILE_NAME, ios::binary);
    if (!outFile) return -1.0;
    for (long long i = 0; i < DISK_FILE_SIZE; i += buffer_size) {
        outFile.write(buffer.data(), buffer_size);
    }
    outFile.close();

    ifstream inFile(DISK_FILE_NAME, ios::binary);
    if (!inFile) return -1.0;
    while(inFile.read(buffer.data(), buffer_size));
    inFile.close();
    
    auto end = high_resolution_clock::now();
    duration<double> duration = end - start;
    
    remove(DISK_FILE_NAME.c_str());
    
    return duration.count();
}

template<typename Func>
double run_benchmark(const string& name, Func test_instance, double& score, const double score_factor) {
    const int NUM_RUNS = 5;
    cout << "Running " << name << "..." << endl;
    
    double total_duration = 0.0;

    for (int i = 0; i < NUM_RUNS; ++i) {
        cout << " > Run " << (i + 1) << "/" << NUM_RUNS << "... ";
        total_duration += test_instance();
        cout << "done." << endl;
    }
    
    double avg_duration_s = total_duration / NUM_RUNS;
    
    if (avg_duration_s > 0) {
        score = score_factor / avg_duration_s;
    } else {
        score = 0;
    }
    
    cout << " > Average Time: " << avg_duration_s << " s" << endl;
    cout << " > Score: " << score << endl << endl;
    
    return avg_duration_s;
}

int main() {
    cout << fixed << setprecision(2);
    print_header();

    double cpu_int_score = 0, cpu_fp_score = 0;
    double mem_seq_score = 0, mem_rand_score = 0;
    double disk_io_score = 0;

    run_benchmark("CPU Integer Performance", run_cpu_integer_test_instance, cpu_int_score, SCORE_CPU_INT_FACTOR);
    run_benchmark("CPU Floating-Point Performance", run_cpu_floating_point_test_instance, cpu_fp_score, SCORE_CPU_FP_FACTOR);
    
    vector<char> mem_buffer(MEM_BUFFER_SIZE);
    run_benchmark("Memory Sequential Access Speed", [&](){ return run_memory_sequential_test_instance(mem_buffer); }, mem_seq_score, SCORE_MEM_SEQ_FACTOR);
    run_benchmark("Memory Random Access Speed", [&](){ return run_memory_random_test_instance(mem_buffer); }, mem_rand_score, SCORE_MEM_RAND_FACTOR);
    mem_buffer.clear();
    mem_buffer.shrink_to_fit();

    run_benchmark("Disk I/O Speed", run_disk_io_test_instance, disk_io_score, SCORE_DISK_IO_FACTOR);

    double total_cpu_score = cpu_int_score + cpu_fp_score;
    double total_mem_score = mem_seq_score + mem_rand_score;
    double total_score = total_cpu_score + total_mem_score + disk_io_score;

    cout << "========================================" << endl;
    cout << "==           Benchmark Results        ==" << endl;
    cout << "========================================" << endl;
    cout << left << setw(25) << "Component" << right << setw(15) << "Score" << endl;
    cout << "----------------------------------------" << endl;
    cout << left << setw(25) << "CPU (Integer + FP)" << right << setw(15) << total_cpu_score << endl;
    cout << left << setw(25) << "Memory (Seq + Random)" << right << setw(15) << total_mem_score << endl;
    cout << left << setw(25) << "Disk I/O" << right << setw(15) << disk_io_score << endl;
    cout << "========================================" << endl;
    cout << left << setw(25) << "FINAL SCORE" << right << setw(15) << total_score << endl;
    cout << "========================================" << endl;

    return 0;
}
