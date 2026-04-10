// Implements the scheduler's queue management, worker lifecycle, and job tracking.

#include "scheduler/scheduler.hpp"

#include <algorithm>
#include <ranges>
#include <stdexcept>
#include <utility>

namespace scheduler {
namespace {

void throw_if_stopping(bool stopping) {
    if (stopping) {
        throw std::runtime_error("Scheduler is shut down");
    }
}

void validate_job_for_submission(
    const JobPtr& job,
    const std::unordered_map<std::uint64_t, JobPtr>& jobs,
    const std::unordered_set<std::uint64_t>& submitted_job_ids) {
    const auto job_it = jobs.find(job->get_id());
    if (job_it == jobs.end() || job_it->second != job) {
        throw std::invalid_argument("Cannot submit a job that was not created by this scheduler");
    }

    if (submitted_job_ids.contains(job->get_id())) {
        throw std::runtime_error("Cannot submit the same job more than once");
    }
}

}  // namespace

bool Scheduler::QueueCompare::operator()(const QueueEntry& left, const QueueEntry& right) const {
    if (left.job->get_priority() == right.job->get_priority()) {
        return left.sequence > right.sequence;
    }

    return static_cast<int>(left.job->get_priority()) < static_cast<int>(right.job->get_priority());
}

Scheduler::Scheduler(std::size_t worker_count)
    : worker_count_(std::max<std::size_t>(1, worker_count)) {
    workers_.reserve(worker_count_);

    for (std::size_t index = 0; index < worker_count_; ++index) {
        workers_.emplace_back(&Scheduler::worker_loop, this);
    }
}

Scheduler::~Scheduler() {
    shutdown();
}

JobPtr Scheduler::create_job(JobPriority priority, const std::string& description, Job::Task task) {
    std::lock_guard<std::mutex> lock(mutex_);
    throw_if_stopping(stopping_);

    auto job = std::make_shared<Job>(next_job_id_++, priority, description, std::move(task));
    jobs_[job->get_id()] = job;
    return job;
}

void Scheduler::submit(const JobPtr& job) {
    if (!job) {
        throw std::invalid_argument("Cannot submit a null job");
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        throw_if_stopping(stopping_);
        validate_job_for_submission(job, jobs_, submitted_job_ids_);

        job->set_status(JobStatus::Pending);
        submitted_job_ids_.insert(job->get_id());
        queue_.push(QueueEntry{job, next_sequence_++});
    }

    work_available_.notify_one();
}

void Scheduler::wait_for_all() {
    std::unique_lock<std::mutex> lock(mutex_);
    all_done_.wait(lock, [this] {
        return queue_.empty() && active_jobs_ == 0;
    });
}

void Scheduler::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) {
            return;
        }

        stopping_ = true;
    }

    work_available_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    workers_.clear();
}

bool Scheduler::empty() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty() && active_jobs_ == 0;
}

std::size_t Scheduler::pending_jobs() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

std::vector<JobPtr> Scheduler::get_all_jobs() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<JobPtr> result;
    result.reserve(jobs_.size());

    for (const auto& [id, job] : jobs_) {
        (void)id;
        result.push_back(job);
    }

    std::ranges::sort(result, [](const JobPtr& left, const JobPtr& right) {
        return left->get_id() < right->get_id();
    });

    return result;
}

std::optional<JobPtr> Scheduler::find_job(std::uint64_t id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = jobs_.find(id);
    if (it == jobs_.end()) {
        return std::nullopt;
    }

    return it->second;
}

void Scheduler::worker_loop() {
    while (true) {
        JobPtr job;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            work_available_.wait(lock, [this] {
                return stopping_ || !queue_.empty();
            });

            if (stopping_ && queue_.empty()) {
                return;
            }

            auto entry = queue_.top();
            queue_.pop();
            job = entry.job;
            ++active_jobs_;
        }

        // Job status transitions are kept outside the scheduler mutex so a
        // long-running task never blocks unrelated readers of scheduler state.
        job->set_status(JobStatus::Running);

        try {
            job->execute();
            job->set_status(JobStatus::Completed);
        } catch (...) {
            job->set_status(JobStatus::Failed);
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            --active_jobs_;
            if (queue_.empty() && active_jobs_ == 0) {
                all_done_.notify_all();
            }
        }
    }
}

}  // namespace scheduler
