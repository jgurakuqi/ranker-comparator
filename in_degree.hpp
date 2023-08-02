#ifndef IN_DEGREE_HPP
#define IN_DEGREE_HPP

#include "ranker.hpp"

using namespace std;

class in_degree : public ranker
{

public:
    in_degree(const csr &csr_matrix) : ranker(csr_matrix) {}

    /// @brief Perform In-degree scores computation.
    void perform_ranking()
    {
        int *row_pointer = mmap_wrapper(csr_matrix.get_map_row_ptr_filename(), csr_matrix.get_row_ptr_size());

        unsigned int num_nodes = csr_matrix.get_num_nodes();

        scores = vector<double>(num_nodes + 1);

        for (int i = 0; i < num_nodes; i++)
        {
            scores[i] = static_cast<double>(row_pointer[i + 1] - row_pointer[i]) / num_nodes;
        }

        munmap_wrapper(row_pointer, csr_matrix.get_row_ptr_size());
    }
};

#endif // IN_DEGREE_HPP
