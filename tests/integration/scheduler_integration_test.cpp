#include "scheduler/scheduler.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

namespace scheduler {
namespace {

using namespace std::chrono_literals;

TEST(SchedulerIntegrationTest, SingleWorkerExecutesJobsByPriorityThenSubmissionOrder) {
    Scheduler scheduler {1};
    std::mutex order_mutex;
    std::vector<std::uint64_t> execution_order;

    const auto low_job = scheduler.create_job(JobPriority::Low, "low", [&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(1U);
    });
    const auto high_job_first = scheduler.create_job(JobPriority::High, "high 1", [&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(2U);
    });
    const auto medium_job = scheduler.create_job(JobPriority::Medium, "medium", [&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(3U);
    });
    const auto high_job_second = scheduler.create_job(JobPriority::High, "high 2", [&] {
        std::lock_guard<std::mutex> lock(order_mutex);
        execution_order.push_back(4U);
    });

    scheduler.submit(low_job);
    scheduler.submit(high_job_first);
    scheduler.submit(medium_job);
    scheduler.submit(high_job_second);
    scheduler.wait_for_all();

    const std::vector<std::uint64_t> expected_order {2U, 4U, 3U, 1U};
    EXPECT_EQ(execution_order, expected_order);
}

TEST(SchedulerIntegrationTest, MultipleWorkersAllowConcurrentExecution) {
    Scheduler scheduler {3};
    std::atomic<int> active_jobs {0};
    std::atomic<int> max_active_jobs {0};
    std::vector<JobPtr> jobs;

    for (int index = 0; index < 6; ++index) {
        jobs.push_back(scheduler.create_job(JobPriority::Medium, "concurrent job", [&] {
            const int now_active = active_jobs.fetch_add(1, std::memory_order_relaxed) + 1;

            int observed_max = max_active_jobs.load(std::memory_order_relaxed);
            while (now_active > observed_max &&
                   !max_active_jobs.compare_exchange_weak(
                       observed_max,
                       now_active,
                       std::memory_order_relaxed,
                       std::memory_order_relaxed)) {
            }

            std::this_thread::sleep_for(80ms);
            active_jobs.fetch_sub(1, std::memory_order_relaxed);
        }));
    }

    for (const auto& job : jobs) {
        scheduler.submit(job);
    }

    scheduler.wait_for_all();

    EXPECT_GE(max_active_jobs.load(std::memory_order_relaxed), 2);
    for (const auto& job : jobs) {
        EXPECT_EQ(job->get_status(), JobStatus::Completed);
    }
}

TEST(SchedulerIntegrationTest, MixedSuccessAndFailurePreserveFinalStatuses) {
    Scheduler scheduler {2};

    const auto success_job = scheduler.create_job(JobPriority::High, "success", [] {
        std::this_thread::sleep_for(20ms);
    });
    const auto failure_job = scheduler.create_job(JobPriority::Medium, "failure", [] {
        throw std::runtime_error("task failed");
    });
    const auto another_success = scheduler.create_job(JobPriority::Low, "another success", [] {});

    scheduler.submit(success_job);
    scheduler.submit(failure_job);
    scheduler.submit(another_success);
    scheduler.wait_for_all();

    EXPECT_EQ(success_job->get_status(), JobStatus::Completed);
    EXPECT_EQ(failure_job->get_status(), JobStatus::Failed);
    EXPECT_EQ(another_success->get_status(), JobStatus::Completed);
}

TEST(SchedulerIntegrationTest, GetAllJobsProvidesDeterministicSnapshotsAfterExecution) {
    Scheduler scheduler {2};

    const auto first = scheduler.create_job(JobPriority::Low, "first", [] {});
    const auto second = scheduler.create_job(JobPriority::High, "second", [] {});
    const auto third = scheduler.create_job(JobPriority::Medium, "third", [] {});

    scheduler.submit(first);
    scheduler.submit(second);
    scheduler.submit(third);
    scheduler.wait_for_all();

    const auto jobs = scheduler.get_all_jobs();

    ASSERT_EQ(jobs.size(), 3U);
    EXPECT_EQ(jobs[0]->get_id(), first->get_id());
    EXPECT_EQ(jobs[1]->get_id(), second->get_id());
    EXPECT_EQ(jobs[2]->get_id(), third->get_id());
    EXPECT_TRUE(std::all_of(jobs.begin(), jobs.end(), [](const JobPtr& job) {
        return job->get_status() == JobStatus::Completed;
    }));
}

}  // namespace
}  // namespace scheduler
