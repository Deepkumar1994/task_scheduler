#include "scheduler/job.hpp"

#include <stdexcept>
#include <utility>

namespace scheduler {

Job::Job(std::uint64_t id, JobPriority priority, std::string description, Task task)
    : id_(id),
      priority_(priority),
      description_(std::move(description)),
      task_(std::move(task)),
      status_(JobStatus::Pending) {}

std::uint64_t Job::get_id() const noexcept {
    return id_;
}

JobPriority Job::get_priority() const noexcept {
    return priority_;
}

JobStatus Job::get_status() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return status_;
}

const std::string& Job::get_description() const noexcept {
    return description_;
}

void Job::execute() const {
    if (!task_) {
        throw std::runtime_error("Job has no task");
    }

    task_();
}

void Job::set_status(JobStatus status) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    status_ = status;
}

std::string to_string(JobPriority priority) {
    switch (priority) {
    case JobPriority::Low:
        return "Low";
    case JobPriority::Medium:
        return "Medium";
    case JobPriority::High:
        return "High";
    }

    return "Unknown";
}

std::string to_string(JobStatus status) {
    switch (status) {
    case JobStatus::Pending:
        return "Pending";
    case JobStatus::Running:
        return "Running";
    case JobStatus::Completed:
        return "Completed";
    case JobStatus::Failed:
        return "Failed";
    }

    return "Unknown";
}

}  // namespace scheduler
