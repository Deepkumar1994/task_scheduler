#include "scheduler/scheduler.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

namespace scheduler {
namespace {

class SchedulerTest : public ::testing::Test {
protected:
    Scheduler scheduler {1};
};

TEST_F(SchedulerTest, CreateJobReturnsTrackedPendingJob) {
    const auto job = scheduler.create_job(JobPriority::Medium, "created job", [] {});

    ASSERT_NE(job, nullptr);
    EXPECT_EQ(job->get_id(), 1U);
    EXPECT_EQ(job->get_priority(), JobPriority::Medium);
    EXPECT_EQ(job->get_status(), JobStatus::Pending);
    EXPECT_EQ(job->get_description(), "created job");
}

TEST_F(SchedulerTest, SubmitRejectsNullJob) {
    EXPECT_THROW(scheduler.submit(nullptr), std::invalid_argument);
}

TEST_F(SchedulerTest, SubmitRejectsJobFromAnotherScheduler) {
    Scheduler other_scheduler {1};
    const auto foreign_job = other_scheduler.create_job(JobPriority::High, "foreign", [] {});

    EXPECT_THROW(scheduler.submit(foreign_job), std::invalid_argument);
}

TEST_F(SchedulerTest, SubmitRejectsTheSameJobTwice) {
    const auto job = scheduler.create_job(JobPriority::Low, "duplicate", [] {});

    scheduler.submit(job);
    scheduler.wait_for_all();

    EXPECT_THROW(scheduler.submit(job), std::runtime_error);
}

TEST_F(SchedulerTest, WaitForAllCompletesQueuedWork) {
    std::atomic<bool> executed {false};
    const auto job = scheduler.create_job(JobPriority::High, "wait job", [&executed] {
        executed.store(true, std::memory_order_relaxed);
    });

    scheduler.submit(job);
    scheduler.wait_for_all();

    EXPECT_TRUE(executed.load(std::memory_order_relaxed));
    EXPECT_EQ(job->get_status(), JobStatus::Completed);
}

TEST_F(SchedulerTest, EmptyReflectsQueuedAndCompletedWork) {
    EXPECT_TRUE(scheduler.empty());

    const auto job = scheduler.create_job(JobPriority::Medium, "emptiness", [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    });

    scheduler.submit(job);
    EXPECT_FALSE(scheduler.empty());

    scheduler.wait_for_all();
    EXPECT_TRUE(scheduler.empty());
}

TEST_F(SchedulerTest, PendingJobsReturnsNumberOfQueuedJobs) {
    const auto slow_job = scheduler.create_job(JobPriority::Low, "slow", [] {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    });
    const auto queued_job = scheduler.create_job(JobPriority::High, "queued", [] {});

    scheduler.submit(slow_job);
    scheduler.submit(queued_job);

    EXPECT_LE(scheduler.pending_jobs(), 1U);

    scheduler.wait_for_all();
    EXPECT_EQ(scheduler.pending_jobs(), 0U);
}

TEST_F(SchedulerTest, GetAllJobsReturnsTrackedJobsSortedById) {
    const auto first = scheduler.create_job(JobPriority::Low, "first", [] {});
    const auto second = scheduler.create_job(JobPriority::High, "second", [] {});
    const auto third = scheduler.create_job(JobPriority::Medium, "third", [] {});

    const auto jobs = scheduler.get_all_jobs();

    ASSERT_EQ(jobs.size(), 3U);
    EXPECT_EQ(jobs[0], first);
    EXPECT_EQ(jobs[1], second);
    EXPECT_EQ(jobs[2], third);
}

TEST_F(SchedulerTest, FindJobReturnsTrackedJobById) {
    const auto job = scheduler.create_job(JobPriority::High, "find me", [] {});

    const auto found_job = scheduler.find_job(job->get_id());
    const auto missing_job = scheduler.find_job(999U);

    ASSERT_TRUE(found_job.has_value());
    EXPECT_EQ(found_job.value(), job);
    EXPECT_FALSE(missing_job.has_value());
}

TEST_F(SchedulerTest, ShutdownPreventsCreateJobAndSubmit) {
    const auto job = scheduler.create_job(JobPriority::Medium, "before shutdown", [] {});

    scheduler.shutdown();

    EXPECT_THROW(scheduler.create_job(JobPriority::Low, "after shutdown", [] {}), std::runtime_error);
    EXPECT_THROW(scheduler.submit(job), std::runtime_error);
}

TEST_F(SchedulerTest, FailingTaskTransitionsToFailedStatus) {
    const auto job = scheduler.create_job(JobPriority::High, "failing", [] {
        throw std::runtime_error("boom");
    });

    scheduler.submit(job);
    scheduler.wait_for_all();

    EXPECT_EQ(job->get_status(), JobStatus::Failed);
}

}  // namespace
}  // namespace scheduler
