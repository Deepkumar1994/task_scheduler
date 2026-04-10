// Implements the small Job type used by the scheduler.

#include "scheduler/job.hpp"

#include <stdexcept>
#include <string_view>
#include <utility>

namespace scheduler {
namespace {

std::string to_owned_string(std::string_view value) {
    return std::string(value);
}

}  // namespace

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
        return to_owned_string("Low");
    case JobPriority::Medium:
        return to_owned_string("Medium");
    case JobPriority::High:
        return to_owned_string("High");
    }

    return to_owned_string("Unknown");
}

std::string to_string(JobStatus status) {
    switch (status) {
    case JobStatus::Pending:
        return to_owned_string("Pending");
    case JobStatus::Running:
        return to_owned_string("Running");
    case JobStatus::Completed:
        return to_owned_string("Completed");
    case JobStatus::Failed:
        return to_owned_string("Failed");
    }

    return to_owned_string("Unknown");
}

}  // namespace scheduler
