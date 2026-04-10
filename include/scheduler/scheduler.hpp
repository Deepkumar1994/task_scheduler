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
    explicit Scheduler(std::size_t worker_count = std::thread::hardware_concurrency());
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    JobPtr create_job(JobPriority priority, const std::string& description, Job::Task task);
    void submit(const JobPtr& job);
    void wait_for_all();
    void shutdown();

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t pending_jobs() const noexcept;
    [[nodiscard]] std::vector<JobPtr> get_all_jobs() const;
    [[nodiscard]] std::optional<JobPtr> find_job(std::uint64_t id) const;

private:
    struct QueueEntry {
        JobPtr job;
        std::uint64_t sequence;
    };

    struct QueueCompare {
        bool operator()(const QueueEntry& left, const QueueEntry& right) const;
    };

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
