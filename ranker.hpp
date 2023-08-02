#ifndef RANKER_HPP
#define RANKER_HPP

#include "csr.hpp"
#include <utility>

using namespace std;

class ranker
{
protected:
    /// @brief network's matrix stored as csr.
    csr csr_matrix;

    /// @brief scores of the specific ranking algorithm (i.e., authority for hits).

    vector<double> scores;
    /// @brief Threshold for the termination of the ranking algorithms.
    constexpr static double difference_threshold = 5.0e-6;

public:
    /// @brief Stores the csr_matrix.
    /// @param csr_matrix
    ranker(const csr &csr_matrix) : csr_matrix(csr_matrix) {}

    /// @brief Get the scores computed by the ranking algorithm.
    /// @return
    const vector<double> &get_scores() const
    {
        return scores;
    }

    /**
     * @brief Checks whether the euclidean distance between the given vectors exceeds the given threshold.
     * @param vector_a First vector.
     * @param vector_b Second vector.
     * @param iterations_to_convergence Number of iterations used to check whether the algorithm is proceeding to much without changes in the difference.
     * @return True if the distance between the vectors exceeds the threshold, otherwise false.
     */
    static bool check_convergence(const vector<double> &vector_a, const std::vector<double> &vector_b, const int &iterations_to_convergence)
    {
        unsigned int number_of_nodes = vector_a.size();
        double difference = 0.0;
        for (int i = 0; i < number_of_nodes; ++i)
        {
            difference += fabs(vector_b[i] - vector_a[i]);
        }
        // if two consecutive instances of the algorithm vector are almost identical, stop
        return difference_threshold < difference && iterations_to_convergence < 200;
    }

    /// @brief Handles the ranking according to the implementation.
    void perform_ranking();

    /**
     * @brief Map the current process's memory to a permanent memory region of size map_ptr_size.
     * @param filename Path of the to map.
     * @param map_ptr_size Size of the region to map.
     * @throws std::runtime_error
     * @return The pointer to the mapped memory.
     */
    static int *mmap_wrapper(const string &filename, const int &map_ptr_size)
    {
        int file_row = open(filename.c_str(), O_RDONLY);
        int *pointer = (int *)mmap(nullptr, map_ptr_size * sizeof(int), PROT_READ, MAP_SHARED, file_row, 0);
        close(file_row);
        if (pointer == MAP_FAILED)
        {
            throw std::runtime_error("Error with the mapping of the file.");
        }
        return pointer;
    }

    /**
     * @brief Deallocate the permanent memory mapping.
     * @param __addr
     * @param map_ptr_size
     */
    static void munmap_wrapper(int *const &__addr, const int &map_ptr_size)
    {
        if (munmap(__addr, map_ptr_size * sizeof(int)) == -1)
        {
            throw std::runtime_error("Mapping deallocation failed.");
        }
    }
};

#endif // RANKER_HPP
