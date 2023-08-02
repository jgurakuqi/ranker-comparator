#ifndef UTILITIES_HPP
#define UTILITIES_HPP

#include <iostream>
#include <vector>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cmath>
#include <fstream>
#include <queue>
#include <string>
#include <stdexcept>
#include <unordered_set>

using namespace std;

// double jaccard_old_variant(const vector<pair<int, double>> &scores_a, const vector<pair<int, double>> &scores_b)
// {
//     double sum = 0.0,
//            number_of_nodes = min(scores_a.size(), scores_b.size());

//     for (const pair<int, double> &score_a : scores_a)
//     {
//         for (const pair<int, double> &score_b : scores_b)
//         {
//             if (score_a.first == score_b.first)
//             {
//                 sum = sum + 1.0;
//             }
//         }
//     }
//     return sum / (number_of_nodes * 2 - sum);
// }

/**
 * @brief Computes the jaccard similarity coefficient for the two given vectors of scores.
 * @param scores_a Scores of a first ranking method.
 * @param scores_b Scores of a second ranking method.
 * @return The jaccard similarity coefficient of the two collections.
 */
double jaccard(const vector<pair<int, double>> &scores_a, const vector<pair<int, double>> &scores_b)
{
    unordered_set<int> set_a, set_b, intersection;

    for (const pair<int, double> &score : scores_a)
        set_a.insert(score.first);

    for (const pair<int, double> &score : scores_b)
        set_b.insert(score.first);

    for (const int &element : set_a)
    {
        if (set_b.find(element) != set_b.end())
        {
            intersection.insert(element);
        }
    }

    double intersection_size = intersection.size(),
           union_size = set_a.size() + set_b.size() - intersection_size;

    return intersection_size / union_size;
}

/**
 * @brief Retrieve the top k ranked elements from the given vector, using a min-heap instead of a sorting,
 * achieving a n*log(k) complexity, instead of n*log(n), typicial of comparation-based sorting algorithms.
 * @param k Number of top elements to retrieve.
 * @param values Values among which to draw the k elements.
 * @param print_top_k Tells whether to print or not such k-elements.
 * @return Vector of int-double pairs, where int is the rank-position, double the rank-score.

 */
vector<pair<int, double>> top_k_retrieval(int k, const vector<double> &values, bool print_top_k)
{
    // Keep k equal to the minimimum between values.size and k to avoid out of bounds.
    k = (values.size() < k) ? values.size() : k;

    // Define the comparator requires to store the elements in the heap.
    auto cmp = [](const std::pair<int, double> &a, const std::pair<int, double> &b)
    {
        return a.second > b.second;
    };

    // Create a priority queue (min-heap) of int-double pairs.
    std::priority_queue<std::pair<int, double>, std::vector<std::pair<int, double>>, decltype(cmp)> min_heap(cmp);
    double min, score;

    // Store the k-elements in the heap.
    for (int i = 0; i < k; i++)
    {
        min_heap.push(make_pair(i, values[i]));
    }

    // DAAT algorithm
    for (int i = k; i < values.size(); i++)
    {
        min = min_heap.top().second; // Get current heap minimum.
        score = values[i];           // Current score.
        if (score > min)
        {
            min_heap.pop();             // Heap root delete.
            min_heap.emplace(i, score); // Insert the new score.
        }
    }

    std::vector<std::pair<int, double>> result;

    while (!min_heap.empty())
    {
        result.push_back(min_heap.top());
        min_heap.pop();
    }

    if (print_top_k)
    {
        cout << endl
             << "Top-k nodes:" << endl;
        for (pair<int, double> &pair : result)
        {
            cout << "     " << pair.first << ": " << pair.second << endl;
        }
    }

    return result;
}

#endif // UTILITIES_HPP
