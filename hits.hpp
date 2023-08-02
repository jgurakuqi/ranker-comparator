#ifndef HITS_HPP
#define HITS_HPP

#include "ranker.hpp"

using namespace std;

class hits : public ranker
{
public:
    hits(const csr &csr_matrix) : ranker(csr_matrix) {}

    /// @brief Perform hits authority computation.
    void perform_ranking()
    {
        bool keep_looping = true;
        unsigned int iterations_to_convergence = 0,
                     current_column,
                     num_of_nodes = csr_matrix.get_num_nodes(),
                     elem_count_per_row;
        double cumulative_sum; // Used to normalize.

        int *row_pointer = mmap_wrapper(csr_matrix.get_map_row_ptr_filename(), csr_matrix.get_row_ptr_size()),
            *column_index = mmap_wrapper(csr_matrix.get_map_col_idx_filename(), csr_matrix.get_col_idx_size());

        // scores and new_scores store the authorities computed at the previous loop and during the current
        // loop, allowing for a comparison useful to check for the termination of the loop.
        scores = vector<double>(num_of_nodes, 1.0 / num_of_nodes); // Initial probability distribution.
        vector<double> new_scores(num_of_nodes + 1, 0.0),
            *csr_values = &(csr_matrix.get_values()); // References the csr values.

        while (keep_looping)
        {
            // Reset variables:
            cumulative_sum = 0.0;
            current_column = 0;
            fill(new_scores.begin(), new_scores.end(), 0.0);

            for (int i = 0; i < num_of_nodes; ++i)
            {
                elem_count_per_row = row_pointer[i + 1] - row_pointer[i];
                for (int j = 0; j < elem_count_per_row; ++j)
                {
                    // Product Matrix * Vector.
                    new_scores[i] += ((*csr_values)[current_column] * scores[column_index[current_column]]);
                    ++current_column;
                }
                cumulative_sum += new_scores[i];
            }

            // Normalization new_scores:
            for (double &curr_auth : new_scores)
            {
                curr_auth /= cumulative_sum;
            }

            keep_looping = check_convergence(scores, new_scores, iterations_to_convergence); // Check if the current prestige-vector is different or not.
            scores = new_scores;                                                             // Update authority scores.
            ++iterations_to_convergence;                                                     // Increase iterations counter.
        }

        munmap_wrapper(column_index, csr_matrix.get_col_idx_size());
        munmap_wrapper(row_pointer, csr_matrix.get_row_ptr_size());

        cout << "HITS iterations to converge: " << iterations_to_convergence << endl;
    }
};

#endif // HITS_HPP
