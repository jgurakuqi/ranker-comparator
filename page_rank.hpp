#ifndef PAGE_RANK_HPP
#define PAGE_RANK_HPP

#include "ranker.hpp"

using namespace std;

class page_rank : public ranker
{
public:
    page_rank(const csr &csr_matrix) : ranker(csr_matrix) {}

    /// @brief Perform PageRank ranking.
    void perform_ranking()
    {
        // For the CSR representation, using mmaped files to store the column indexes of the value vector and the
        // position of the start of each row in the value vector.
        int *row_pointer = mmap_wrapper(csr_matrix.get_map_row_ptr_filename(), csr_matrix.get_row_ptr_size()),
            *column_index = mmap_wrapper(csr_matrix.get_map_col_idx_filename(), csr_matrix.get_col_idx_size());

        stochastise_matrix(row_pointer);
        perform_ranking_helper(row_pointer, column_index); // Perform the PageRank computation.

        munmap_wrapper(row_pointer, csr_matrix.get_row_ptr_size());
        munmap_wrapper(column_index, csr_matrix.get_col_idx_size());
    }

private:
    constexpr static float damping_factor = 0.85;

    /// @brief Stochasticizes the matrix by artificially introducing links from the dangling
    /// nodes towards every other node of the network.
    /// @param row_pointer
    void stochastise_matrix(const int *row_pointer)
    {
        unsigned int elem_count_per_row = 0,
                     current_column = 0,
                     num_of_nodes = csr_matrix.get_num_nodes();

        vector<int> out_degrees(num_of_nodes + 1, 0);
        vector<double> *csr_values = &(csr_matrix.get_values());

        for (int i = 0; i < num_of_nodes; ++i)
        {
            elem_count_per_row = row_pointer[i + 1] - row_pointer[i];
            // Init out_degrees to handle the dangling nodes in the next for-loop.
            if (row_pointer[i + 1] != 0)
            {
                out_degrees[i] = elem_count_per_row;
            }
            // Normalize and stochasticize the matrix dividing each csr value by its out_degrees.
            for (int j = 0; j < elem_count_per_row; ++j)
            {
                (*csr_values)[current_column] /= out_degrees[i];
                ++current_column;
            }
        }
    }

    /// @brief Perform PageRank computation.
    /// @param row_pointer
    /// @param column_index
    void perform_ranking_helper(const int *row_pointer, const int *column_index)
    {
        unsigned int iterations_to_convergence = 0,
                     elem_count_per_row,
                     current_column,
                     num_of_nodes = csr_matrix.get_num_nodes();
        bool keep_looping = true;

        // Wrapped (1-d)/N for efficiency.
        const double teleportation_factor = (1.0 - damping_factor) / num_of_nodes;

        scores = vector<double>(num_of_nodes, 1.0 / num_of_nodes); // Initial probability distribution.
        vector<double> new_scores(num_of_nodes + 1);               // Prestige-vector used for current iteration.

        vector<double> *csr_values = &(csr_matrix.get_values());

        while (keep_looping)
        {
            // Reset the following vars on each iteration.
            elem_count_per_row = current_column = 0;
            fill(new_scores.begin(), new_scores.end(), 0.0);

            // Optimized PageRank: initialize the teleportation matrix.
            for (int i = 0; i < num_of_nodes; ++i)
            {
                elem_count_per_row = row_pointer[i + 1] - row_pointer[i];
                for (int j = 0; j < elem_count_per_row; ++j)
                {
                    new_scores[column_index[current_column]] += (*csr_values)[current_column] * scores[i];
                    ++current_column;
                }
            }

            // Final Matrix after removing dangling nodes and adding teleportation
            // Add a link from each page to every page and give each link a small transition probability controlled by the damping_factor
            for (double &value : new_scores)
            {
                value = damping_factor * value + teleportation_factor;
            }

            keep_looping = check_convergence(scores, new_scores, iterations_to_convergence); // Check if the current prestige-vector is different or not.
            scores = new_scores;                                                             // Store prestige for the next iteration.
            ++iterations_to_convergence;                                                     // Increase iterations counter.
        }

        cout << "PAGE RANK iterations to converge: " << iterations_to_convergence << endl;
    }
};

#endif // PAGE_RANK_HPP
