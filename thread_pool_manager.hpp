#ifndef THREAD_POOL_MANAGER_HPP
#define THREAD_POOL_MANAGER_HPP

#include <thread>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <exception>

/// @brief Allows to create and handle a persistent pool of threads through very simple utilities.
/// Useful to remove the overhead introduced by frequent init and join of threads without pooling.
class thread_pool_manager
{
private:
    class barrier
    {

    public:
        barrier(int barrier_size) : barrier_mutex(),
                                    barrier_condition(),
                                    missing_tasks(barrier_size),
                                    barrier_size(barrier_size) {}

        void await()
        {
            std::unique_lock<std::mutex> lock(barrier_mutex);
            if (0 == --missing_tasks)
            {
                barrier_condition.notify_all();
            }
            else
            {
                barrier_condition.wait(lock, [this]()
                                       { return 0 == missing_tasks; });
            }
        }

    private:
        /// @brief Mutex used to sinchronise the access to the missing_tasks variable.
        std::mutex barrier_mutex;

        /// @brief Number of threads to wait at the barrier.
        int barrier_size;

        /// @brief Number of tasks to complete in order to proceed.
        int missing_tasks;

        /// @brief Condition variable required to perform the missing_tasks control.
        std::condition_variable barrier_condition;
    };

    /// @brief Pool of threads.
    std::vector<std::thread> thread_pool;

    /// @brief Used to store and retrieve tasks.
    std::queue<std::function<void()>> tasks_queue;

    /// @brief Used to shutdown the pool safely.
    std::mutex pool_mutex;

    /// @brief Used to protect the tasks_queue queue.
    std::mutex queue_mutex;

    /// @brief Used to build queue waiting conditions.
    std::condition_variable pool_condition;

    /// @brief Used to start the shutdown.
    bool terminate_pool;

    /// @brief Pointer to the pool barrier.
    std::unique_ptr<barrier> pool_barrier;

    /// @brief Exception utility invoked in case of a wrong number of threads requires to the
    /// thread_pool_manager library.
    struct multithread_init_exception : public std::exception
    {
        const char *what() const throw()
        {
            return "Cannot start a thread pool with less then 1 thread!";
        }
    };

    /// @brief Creates as many threads as specified by the num_of_threads param.
    /// @param num_of_threads Number of threads to create for the pool.
    /// @param barrier_size Number of threads to wait at the barrier.
    void create_threads(int num_of_threads, int barrier_size)
    {
        pool_barrier = std::make_unique<barrier>(barrier_size);
        terminate_pool = false;
        if (num_of_threads > 0)
        {
            for (int i = 0; i < num_of_threads; i++)
            {
                thread_pool.push_back(
                    std::thread(&thread_pool_manager::wait_for_tasks, this));
            }
        }
        else
        {
            throw multithread_init_exception();
        }
    }

public:
    thread_pool_manager(const thread_pool_manager &) = delete;
    thread_pool_manager &operator=(const thread_pool_manager &) = delete;

    /// @brief Initializes a pool of threads of the size supported by
    /// the running machine, creating a barrier of the same size of the pool.
    thread_pool_manager()
    {
        unsigned int concurrency_level = std::thread::hardware_concurrency();
        create_threads(concurrency_level, concurrency_level);
    }

    /// @brief Initializes a pool of threads of the given size, creating
    /// also a barrier of the chosen size.
    /// @param num_of_threads Number of threads to create.
    /// @param barrier_size Size of the barrier.
    thread_pool_manager(int num_of_threads, int barrier_size)
    {
        // uncompleted_tasks = 0;
        create_threads(num_of_threads, barrier_size);
    }

    /// @brief Returns the number of created threads.
    /// @return Number of created threads.
    int get_pool_size()
    {
        return thread_pool.size();
    }

    /// @brief Pushes the new task into the queue of tasks, to execute it as soon as
    /// a thread will be on idle/available.
    /// @param newTask
    void execute_task(std::function<void()> &&newTask)
    {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            tasks_queue.push(std::function<void()>(newTask));
        }
        pool_condition.notify_one();
    }

    /// @brief Keeps each created thread on idle, waiting for a new task to be executed.
    /// When a new task becomes available, one of the idle threads will be awakened in
    /// order to process the given task.
    void wait_for_tasks()
    {
        std::function<void()> taskToPerform;
        while (true)
        {
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                pool_condition.wait(lock, [&]
                                    { return !tasks_queue.empty() || terminate_pool; });
                if (!tasks_queue.empty())
                {
                    taskToPerform = tasks_queue.front();
                    tasks_queue.pop();
                }
                else
                {
                    return;
                }
            }
            if (taskToPerform != nullptr)
            {
                taskToPerform();
            }
        }
    }

    /// @brief Waits for the completion of all the tasks assigned to the threads,
    /// without terminating them.
    void wait_on_barrier()
    {
        pool_barrier.get()->await();
    }

    /// @brief Terminates the pool of threads.
    void shutdown()
    {
        // Notify the shutdown beginning to all threads.
        {
            std::lock_guard<std::mutex> thMutex(pool_mutex);
            terminate_pool = true;
        }
        pool_condition.notify_all();

        // Wait for the threads to join.
        for (std::thread &actThread : thread_pool)
        {
            actThread.join();
        }
    }
};

#endif // THREAD_POOL_MANAGER_HPP