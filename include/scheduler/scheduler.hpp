// Declares a small priority-based task scheduler with tracked single-shot jobs.

#pragma once

#include "scheduler/job.hpp"

#include <cstddef>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace scheduler {

using JobPtr = std::shared_ptr<Job>;

class Scheduler {
public:
    // Starts the worker pool immediately. A worker count of zero falls back to one worker.
    explicit Scheduler(std::size_t worker_count = std::thread::hardware_concurrency());
    // Shuts the scheduler down and joins any worker threads that are still running.
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    // Creates a tracked job owned by this scheduler. Throws after shutdown.
    JobPtr create_job(JobPriority priority, const std::string& description, Job::Task task);
    // Queues a job for execution. Each created job can be submitted exactly once.
    void submit(const JobPtr& job);
    // Blocks until no queued or running jobs remain.
    void wait_for_all();
    // Stops accepting new work and joins the worker pool after queued work finishes.
    void shutdown();

    // Returns true when there is no queued or currently running work.
    [[nodiscard]] bool empty() const noexcept;
    // Returns the number of jobs still waiting in the queue.
    [[nodiscard]] std::size_t pending_jobs() const noexcept;
    // Returns every tracked job sorted by job ID for deterministic iteration.
    [[nodiscard]] std::vector<JobPtr> get_all_jobs() const;
    // Finds a tracked job by ID.
    [[nodiscard]] std::optional<JobPtr> find_job(std::uint64_t id) const;

private:
    struct QueueEntry {
        JobPtr job;
        std::uint64_t sequence;
    };

    struct QueueCompare {
        bool operator()(const QueueEntry& left, const QueueEntry& right) const;
    };

    // Worker entry point that waits for available work and executes jobs.
    void worker_loop();

    std::uint64_t next_job_id_ {1};
    std::uint64_t next_sequence_ {0};
    std::size_t active_jobs_ {0};
    bool stopping_ {false};
    std::size_t worker_count_ {0};
    mutable std::mutex mutex_;
    std::condition_variable work_available_;
    std::condition_variable all_done_;
    std::priority_queue<QueueEntry, std::vector<QueueEntry>, QueueCompare> queue_;
    std::unordered_map<std::uint64_t, JobPtr> jobs_;
    std::unordered_set<std::uint64_t> submitted_job_ids_;
    std::vector<std::thread> workers_;
};

}  // namespace scheduler
