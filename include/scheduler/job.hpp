// Defines the scheduler's unit of work and the small enums used to describe
// its priority and execution lifecycle.

#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

namespace scheduler {

enum class JobPriority : std::uint8_t {
    Low = 0,
    Medium = 1,
    High = 2
};

enum class JobStatus : std::uint8_t {
    Pending = 0,
    Running = 1,
    Completed = 2,
    Failed = 3
};

// Represents one single-shot task tracked by the scheduler.
class Job {
public:
    using Task = std::function<void()>;

    // Stores the task metadata and callable. Jobs start in the Pending state.
    Job(std::uint64_t id, JobPriority priority, std::string description, Task task);

    // Returns the scheduler-assigned unique identifier.
    std::uint64_t get_id() const noexcept;
    // Returns the configured execution priority.
    JobPriority get_priority() const noexcept;
    // Returns the current thread-safe execution status.
    JobStatus get_status() const noexcept;
    // Returns the human-readable description supplied at creation time.
    const std::string& get_description() const noexcept;

    // Executes the task body. Throws if no task was provided or if the task fails.
    void execute() const;
    // Updates the execution status in a thread-safe way.
    void set_status(JobStatus status) noexcept;

private:
    std::uint64_t id_;
    JobPriority priority_;
    std::string description_;
    Task task_;
    mutable std::mutex mutex_;
    JobStatus status_;
};

// Converts a priority to a readable label for logs and demos.
std::string to_string(JobPriority priority);
// Converts a status to a readable label for logs and demos.
std::string to_string(JobStatus status);

}  // namespace scheduler
