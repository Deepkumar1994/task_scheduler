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

class Job {
public:
    using Task = std::function<void()>;

    Job(std::uint64_t id, JobPriority priority, std::string description, Task task);

    std::uint64_t get_id() const noexcept;
    JobPriority get_priority() const noexcept;
    JobStatus get_status() const noexcept;
    const std::string& get_description() const noexcept;

    void execute() const;
    void set_status(JobStatus status) noexcept;

private:
    std::uint64_t id_;
    JobPriority priority_;
    std::string description_;
    Task task_;
    mutable std::mutex mutex_;
    JobStatus status_;
};

std::string to_string(JobPriority priority);
std::string to_string(JobStatus status);

}  // namespace scheduler
