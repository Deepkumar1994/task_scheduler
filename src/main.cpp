// Demonstrates priority-based scheduling and job status reporting.

#include "scheduler/scheduler.hpp"

#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>

namespace {

void print_job_summary(const scheduler::Scheduler& scheduler) {
    std::cout << "\nTracked job summary:\n";
    for (const auto& job : scheduler.get_all_jobs()) {
        std::cout
            << "Job #" << job->get_id()
            << " (" << job->get_description() << ")"
            << " status: " << scheduler::to_string(job->get_status())
            << '\n';
    }
}

}  // namespace

int main() {
    using namespace std::chrono_literals;

    scheduler::Scheduler scheduler(3);
    std::mutex output_mutex;

    const auto backup_job = scheduler.create_job(
        scheduler::JobPriority::Low,
        "Nightly backup",
        [&output_mutex]() {
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                std::cout << "Starting nightly backup on worker " << std::this_thread::get_id() << '\n';
            }
            std::this_thread::sleep_for(900ms);
        });
    const auto report_job = scheduler.create_job(
        scheduler::JobPriority::Medium,
        "Generate sales report",
        [&output_mutex]() {
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                std::cout << "Generating report on worker " << std::this_thread::get_id() << '\n';
            }
            std::this_thread::sleep_for(600ms);
        });
    const auto alert_job = scheduler.create_job(
        scheduler::JobPriority::High,
        "Process urgent alert",
        [&output_mutex]() {
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                std::cout << "Handling urgent alert on worker " << std::this_thread::get_id() << '\n';
            }
            std::this_thread::sleep_for(300ms);
        });
    const auto cleanup_job = scheduler.create_job(
        scheduler::JobPriority::High,
        "Clean temporary files",
        [&output_mutex]() {
            {
                std::lock_guard<std::mutex> lock(output_mutex);
                std::cout << "Cleaning temporary files on worker " << std::this_thread::get_id() << '\n';
            }
            std::this_thread::sleep_for(500ms);
        });

    scheduler.submit(backup_job);
    scheduler.submit(report_job);
    scheduler.submit(alert_job);
    scheduler.submit(cleanup_job);

    std::cout << "Submitting jobs to worker threads...\n";
    scheduler.wait_for_all();
    print_job_summary(scheduler);

    return 0;
}
