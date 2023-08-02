#include "page_rank.hpp"
#include "hits.hpp"
#include "in_degree.hpp"
#include "thread_pool_manager.hpp"
#include <type_traits>

using namespace std;

/// @brief Rank the given network using the ranker of the chosen type, and use a DAAT retrieval to retrieve the top k ranked.
/// This variant uses pre-computed csr to allow multithreading.
/// @tparam Ranker Chosen type of ranker.
/// @param top_k top_k Amount of top ranked nodes to retrieve.
/// @param csr_obj csr matrix required by the ranking algorithm.
/// @param print_top_k Decided whether to print the DAAT retireved top-k nodes or not.
/// @return
template <typename Ranker>
vector<pair<int, double>> retrieve_top_k(const int &top_k, const csr &csr_obj, const bool &print_top_k = false)
{
    Ranker ranker_obj(csr_obj);

    ranker_obj.perform_ranking();

    return top_k_retrieval(top_k, ranker_obj.get_scores(), print_top_k);
}

/// @brief Run the 3 rankers over the same dataset, retrieving the top_k ranked nodes, and
/// compare the results of all of them using jaccard coefficients.
/// @param top_k The top k elements to retrieve from the ranked nodes.
/// @param chosen_dataset Path to the dataset.
/// @param multithreading_enabled Choose whether to enable multithreading or not.
/// @param print_top_k Choose whether to enable multithreading or not.
void compare_rankers(const int &top_k, const string &chosen_dataset, const bool &multithreading_enabled, const bool &print_top_k)
{
    double jaccard_page_rank_hits,
        jaccard_page_rank_in_degree,
        jaccard_hits_in_degree;

    vector<pair<int, double>> top_k_page_rank,
        top_k_hits,
        top_k_in_degree;

    csr csr_page_rank = csr(chosen_dataset),
        csr_shared = csr(regex_replace(chosen_dataset, std::regex(".txt"), "-transpose.txt"));
    if (multithreading_enabled)
    {
        // Start the thread pool.
        thread_pool_manager pool(4, 3);

        // Create the Csrs on parallel threads.
        pool.execute_task([&]()
                          { csr_page_rank.compute(); pool.wait_on_barrier(); });

        pool.execute_task([&]()
                          { csr_shared.compute(); pool.wait_on_barrier(); });

        // Wait for the threads to complete the assigned tasks.
        pool.wait_on_barrier();

        // Perform the rankings on parallel threads.
        pool.execute_task([&]()
                          { top_k_page_rank = retrieve_top_k<page_rank>(top_k, csr_page_rank, print_top_k); });
        pool.execute_task([&]()
                          { top_k_hits = retrieve_top_k<hits>(top_k, csr_shared, print_top_k); });
        pool.execute_task([&]()
                          { top_k_in_degree = retrieve_top_k<in_degree>(top_k, csr_shared, print_top_k); });

        pool.shutdown();
    }
    else
    {
        csr_page_rank.compute();
        csr_shared.compute();

        top_k_page_rank = retrieve_top_k<page_rank>(top_k, csr_page_rank, print_top_k),
        top_k_hits = retrieve_top_k<hits>(top_k, csr_shared, print_top_k),
        top_k_in_degree = retrieve_top_k<in_degree>(top_k, csr_shared, print_top_k);
    }

    // Compute and print the Jaccard similarity coefficients.
    jaccard_page_rank_hits = jaccard(top_k_page_rank, top_k_hits),
    jaccard_page_rank_in_degree = jaccard(top_k_page_rank, top_k_in_degree),
    jaccard_hits_in_degree = jaccard(top_k_hits, top_k_in_degree);
    cout << "Jaccard Similarity Coefficients:" << endl
         << "        [PageRank - HITS Authority]: " << jaccard_page_rank_hits << endl
         << "        [PageRank    -    InDegree]: " << jaccard_page_rank_in_degree << endl
         << "        [HITS Authority - InDegree]: " << jaccard_hits_in_degree << endl;
}

int main()
{
    /// @brief Available datasets:
    ///   --  web-Stanford.txt
    ///   --  web-NotreDame.txt -- best for faster tests
    ///   --  web-BerkStan.txt
    string chosen_dataset = "./DATASET/web-NotreDame.txt";

    /// @brief Tells whether to:
    ///   -- Test performance.
    ///   -- Compare the algorithms using Jaccard with different top-ks.
    bool test_performance = true;

    /// @brief Tells whether to print or not the DAAT retrieved top-k elems.
    bool print_top_k = false;

    /// @brief Tells whether to use or not multithreading.
    /// Multithreading and print_top_k should not both be true as cout is not thread safe.
    bool use_multithreading = true;

    if (test_performance) // Performance test.
    {
        int top_k = 10;
        auto start_time = std::chrono::high_resolution_clock::now(); // Start timing.

        csr::check_transposed_matrix(chosen_dataset);
        compare_rankers(top_k, chosen_dataset, use_multithreading, print_top_k);

        auto elapsed_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now() - start_time); // Measure elapsed time.

        cout << "\nELAPSED TIME in seconds: " << elapsed_time.count() * 1e-9 << endl;

        return 0;
    }
    else // jaccard-based comparisons.
    {
        csr::check_transposed_matrix(chosen_dataset);

        // Top-ks to use.
        vector<int> top_ks = {10, 100, 1000, 10000, 25000, 50000, 100000, 250000, 500000, 1000000};

        for (int &top_k : top_ks)
        {
            compare_rankers(top_k, chosen_dataset, use_multithreading, print_top_k);
            cout << endl
                 << endl;
        }
    }

    return 0;
}