#ifndef CSR_HPP
#define CSR_HPP

#include "utilities.hpp"
#include <cstdio>
#include <regex>

using namespace std;

class csr
{
private:
    /// @brief Pathanems to:
    /// - the csr "column indexes" file.
    /// - the csr "row positions" file.
    /// - the original file.
    string map_col_idx_filename = "./CSR_MMAPS/map_col_idx_filename",
           map_row_ptr_filename = "./CSR_MMAPS/map_row_ptr_filename",
           filename;
    int nodes_cardinality,
        edges_cardinality;

    vector<double> values;
    int row_ptr_size = 0,
        col_idx_size = 0;

    /// @brief Extracts the cardinality of edges and nodes from the file header,
    /// and returns the file positioned on the first couple of nodes.
    /// @throws std::runtime_error
    ifstream extract_net_info_and_skip_header()
    {
        ifstream file = open_file_stream(filename);
        string line;
        unsigned int line_number = 0;

        while (getline(file, line))
        {
            if (line_number++ == 2)
            {
                istringstream iss(line);
                // Ignore the initial '#' character and "Nodes: "
                iss.ignore(9);
                iss >> nodes_cardinality;
                iss.ignore(8); // Skip " Edges: "
                iss >> edges_cardinality;
                return file;
            }
        }
        throw std::runtime_error("No header found for the file: " + filename);
    }

    /// @brief Open the file in the given pathname. If the opening fails,
    /// throw an exception.
    /// @param filename Path to the file to open.
    /// @throws std::runtime_error
    static std::ifstream open_file_stream(const std::string &filename)
    {
        std::ifstream file(filename);
        if (!file.is_open())
        {
            throw std::runtime_error("Cannot open the file: " + filename);
        }
        return file;
    }

public:
    /// @brief Stores the filename which will be used to build the csr.
    csr(const string &filename) : filename(filename) {}

    const string &get_map_col_idx_filename() const
    {
        return map_col_idx_filename;
    }

    const string &get_map_row_ptr_filename() const
    {
        return map_row_ptr_filename;
    }

    int get_num_nodes() const
    {
        return nodes_cardinality;
    }

    vector<double> &get_values()
    {
        return values;
    }

    int get_row_ptr_size() const
    {
        return row_ptr_size;
    }

    int get_col_idx_size() const
    {
        return col_idx_size;
    }

    /// @brief Checks if the transposed matrix is already generated, and if not, it creates it.
    /// @param original_file Original file's pathname.
    static void check_transposed_matrix(const string &original_file)
    {
        ifstream transposed_matrix_file(regex_replace(original_file, regex(".txt"), "-transpose.txt"));
        if (!transposed_matrix_file.is_open())
        {
            cout << "TRANSPOSED MATRIX FILE MISSING: starting creation." << endl;
            transposed_matrix_file.close();
            transpose_matrix(original_file, regex_replace(original_file, regex(".txt"), "-transpose.txt"));
            cout << "  -  CREATION COMPLETED." << endl;
        }
        else
            transposed_matrix_file.close();
    }

    /// @brief Generate a transposed matrix file from the given file.
    /// @param original_file Original file's pathname.
    /// @param transposed_filename Transposed file's pathname.
    static void transpose_matrix(const string &original_file, const string &transposed_filename)
    {
        vector<pair<int, int>> edges;
        ofstream transposed(transposed_filename);        // Container for new transposed file.
        ifstream file = open_file_stream(original_file); // Original file.
        string header,                                   // Header container.
            line;                                        // Current line container.

        while (getline(file, line)) // Process each line here
        {

            if (line[0] == '#') // read header
            {
                header += line + "\n";
            }
            else // read nodes
            {
                istringstream iss(line);
                int from_node, to_node;
                if (iss >> from_node >> to_node)
                {
                    edges.push_back(make_pair(to_node, from_node));
                }
                else
                {
                    throw runtime_error("Error parsing line.");
                }
            }
        }

        file.close();

        transposed << header; // paste header file

        sort(edges.begin(), edges.end());

        constexpr int BUFFER_SIZE = 20000; // 10000 very good
        ostringstream buffer;

        for (unsigned int i = 0; i < edges.size(); ++i)
        {
            buffer << edges[i].first << "   " << edges[i].second << '\n';
            if ((i + 1) % BUFFER_SIZE == 0 || i == edges.size() - 1)
            {
                transposed << buffer.str();
                buffer.str(""); // Clear the buffer
            }
        }

        transposed.close();
    }

    /// @brief Computes the csr matrix, storing the column indexes and starting row positions into
    /// files which will be accessed through mmap.
    void compute()
    {
        int current_row = 0,
            elem_count_per_row = 0, // Elements per each row.
            new_row_ptr_elem = 0,   // Cumulative numbers of elements. Stored in row_ptr.
            last_row;

        ifstream original_file(extract_net_info_and_skip_header()); // Main file with values.

        // Distinguish the pagerank csr files from Hits and In-degrees
        if (filename.find("transpose") != std::string::npos)
        {
            // In-degree and hits use the same csr.
            map_col_idx_filename += "_trsp";
            map_row_ptr_filename += "_trsp";
        }

        ofstream column_idx_file(map_col_idx_filename, ofstream::binary | ofstream::trunc); // Csr column file.
        ofstream row_ptr_file(map_row_ptr_filename, ofstream::binary | ofstream::trunc);    // Csr row positions file.
        string line;                                                                        // Current line of the examined input file.

        values = vector<double>(edges_cardinality, 1.0); // 1.0s vector used for stochatization.

        row_ptr_file.write(reinterpret_cast<const char *>(&current_row), sizeof(int));

        // Bufferization variables.
        constexpr size_t buffer_size = 5120; // 5120 works better. Alternatives: ... , 4096, 6144, 8192, 16384, ...
        std::vector<int> column_idx_buffer, row_ptr_buffer;
        column_idx_buffer.reserve(buffer_size);
        row_ptr_buffer.reserve(buffer_size);

        // Read all values from the file.
        while (getline(original_file, line))
        {
            istringstream iss(line);
            int from_node, to_node;
            if (iss >> from_node >> to_node)
            {
                if (from_node > current_row) // Row must change.
                {
                    new_row_ptr_elem += elem_count_per_row;
                    for (int k = current_row + 1; k <= from_node; k++)
                    {
                        row_ptr_buffer.push_back(new_row_ptr_elem); // Store the row.
                        if (row_ptr_buffer.size() >= buffer_size)
                        {
                            row_ptr_file.write(reinterpret_cast<const char *>(row_ptr_buffer.data()), row_ptr_buffer.size() * sizeof(int));
                            row_ptr_buffer.clear();
                        }
                    }
                    elem_count_per_row = 0;
                    current_row = from_node;
                }
                column_idx_buffer.push_back(to_node); // Store on buffer the col.
                if (column_idx_buffer.size() >= buffer_size)
                {
                    column_idx_file.write(reinterpret_cast<const char *>(column_idx_buffer.data()), column_idx_buffer.size() * sizeof(int));
                    column_idx_buffer.clear();
                }
                elem_count_per_row++;
            }
        }
        // Write remaining data from the buffers.
        if (!row_ptr_buffer.empty())
        {
            row_ptr_file.write(reinterpret_cast<const char *>(row_ptr_buffer.data()), row_ptr_buffer.size() * sizeof(int));
        }
        if (!column_idx_buffer.empty())
        {
            column_idx_file.write(reinterpret_cast<const char *>(column_idx_buffer.data()), column_idx_buffer.size() * sizeof(int));
        }

        last_row = new_row_ptr_elem + elem_count_per_row - 1;

        row_ptr_file.write(reinterpret_cast<const char *>(&last_row), sizeof(int));

        // Store row position and column indexes vector sizes for mmap.
        row_ptr_size = current_row + 2;
        col_idx_size = edges_cardinality;
    }
};

#endif // CSR_HPP
