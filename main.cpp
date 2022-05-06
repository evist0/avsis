#include <iostream>
#include <functional>
#include <random>

template<class T>
using uniform_distribution =
typename std::conditional<
        std::is_floating_point<T>::value,
        std::uniform_real_distribution<T>,
        typename std::conditional<
                std::is_integral<T>::value,
                std::uniform_int_distribution<T>,
                void
        >::type
>::type;

template<typename T>
T** fill_matrix(std::size_t& size, T** matrix) {
    std::random_device r;

    std::default_random_engine generator(r());
    uniform_distribution<T> distribution(0, 1000);
    auto next = [&]() { return distribution(generator); };

    for (std::size_t i = 0; i < size; i++) {
        for (std::size_t j = 0; j < size; j++) {
            matrix[i][j] = next();
        }
    }

    return matrix;
}

template<typename T>
T** allocate_matrix(std::size_t& row, std::size_t& columns) {
    T** matrix = new T* [row];

    for (std::size_t i = 0; i < row; ++i) {
        matrix[i] = new T[columns]{ 0 };
    }

    return matrix;
}

template<typename T>
void free_matrix(std::size_t& size, T** matrix) {
    for (std::size_t i = 0; i < size; i++) {
        delete matrix[i];
    }

    delete[] matrix;
}

template<typename T>
T** multiply_matrix(std::size_t& size_a, T** matrix_a, std::size_t& size_b, T** matrix_b) {
    if (size_a != size_b)
        return nullptr;

    auto result = allocate_matrix<T>(size_a, size_b);

    std::size_t i, j, k;

    for (i = 0; i < size_a; i++) {
        for (j = 0; j < size_b; j++) {
            for (k = 0; k < size_a; k++) {
                result[i][j] += matrix_a[i][k] * matrix_b[k][j];
            }
        }
    }

    return result;
}

float get_average(std::vector<clock_t>& results) {
    clock_t average = 0;

    for (auto result: results) {
        average += result;
    }

    return (float)average / (float)results.size() / CLOCKS_PER_SEC;
}

float get_gibson_performance(std::vector<clock_t>& results) {
    auto probability = 1.f / (float)results.size();
    auto performance = 0.f;

    for (auto result: results) {
        performance += probability * ((float)result / CLOCKS_PER_SEC);
    }

    return 1 / performance;
}

void process_results(std::vector<clock_t>& results, const char* operand_type, const char* optimization_flags) {
    auto average_time = get_average(results);

    auto cpu_model = "Intel(R) Xeon(R) CPU (1 vCPU)";
    auto task = "matrix multiply";
    auto instruction_count = 100 * 100 * 100;
    auto timer = "clock()";

    FILE* stream;
    if ((stream = fopen("results.csv", "a+")) == nullptr) {
        std::cerr << "can not open file";
    }

    std::size_t launch_number = 1;
    for (auto result: results) {
        auto time = (float)result / CLOCKS_PER_SEC;

        auto absolute_error = time - average_time;
        auto relative_error = absolute_error / (float)result * 100.f;

        auto gibson_performance = get_gibson_performance(results);
        auto omega = (100.f * 100.f * 100.f) / (float)result;

        printf("%s;%s;%s;%s;%d;%s;%lf;%lu;%lf;%lf;%lf;%lf;%lf\n", cpu_model, task, operand_type,
                optimization_flags, instruction_count, timer, time, launch_number, average_time, absolute_error,
                relative_error, gibson_performance, omega);

        fprintf(stream, "%s;%s;%s;%s;%d;%s;%lf;%lu;%lf;%lf;%lf;%lf;%lf\n", cpu_model, task, operand_type,
                optimization_flags, instruction_count, timer, time, launch_number, average_time, absolute_error,
                relative_error, gibson_performance, omega);

        launch_number += 1; // Chaotic good xD
    }

    fflush(stream);
    fclose(stream);
}

template<typename T>
void benchmark(std::size_t operations_amount, const char* operand_type, const char* optimization_flags) {
    std::size_t size = 100;

    auto results = std::vector<clock_t>(operations_amount);

    for (auto i = 0; i < operations_amount; i++) {
        T** matrix_a = allocate_matrix<T>(size, size);
        matrix_a = fill_matrix(size, matrix_a);

        T** matrix_b = allocate_matrix<T>(size, size);
        matrix_b = fill_matrix(size, matrix_b);

        auto start_time = clock();
        auto result = multiply_matrix<T>(size, matrix_a, size, matrix_b);
        auto end_time = clock();

        results[i] = end_time - start_time;

        free_matrix(size, matrix_a);
        free_matrix(size, matrix_b);
        free_matrix(size, result);
    }

    process_results(results, operand_type, optimization_flags);
}

int main(int argc, char* argv[]) {
    auto optimization_flags = "-O3";

    std::size_t int_operations, float_operations, double_operations;

    if (argc < 3) {
        std::cerr << "arguments required" << std::endl;
    }

    sscanf(argv[1], "%lu", &int_operations);
    if (int_operations < 10) {
        std::cerr << "operations amount should be greater or equal 10" << std::endl;
    }

    sscanf(argv[2], "%lu", &float_operations);
    if (float_operations < 10) {
        std::cerr << "operations amount should be greater or equal 10" << std::endl;
    }

    sscanf(argv[3], "%lu", &double_operations);
    if (double_operations < 10) {
        std::cerr << "operations amount should be greater or equal 10" << std::endl;
    }

    benchmark<int>(int_operations, "int", optimization_flags);
    benchmark<float>(float_operations, "float", optimization_flags);
    benchmark<double>(double_operations, "double", optimization_flags);

    return 0;
}
