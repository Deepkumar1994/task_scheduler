#include "scheduler/job.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <stdexcept>
#include <string>

namespace scheduler {
namespace {

using ::testing::StrEq;

TEST(JobTest, ConstructorInitializesAllFields) {
    Job job(42, JobPriority::High, "example job", [] {});

    EXPECT_EQ(job.get_id(), 42U);
    EXPECT_EQ(job.get_priority(), JobPriority::High);
    EXPECT_EQ(job.get_status(), JobStatus::Pending);
    EXPECT_THAT(job.get_description(), StrEq("example job"));
}

TEST(JobTest, SetStatusUpdatesTheTrackedStatus) {
    Job job(7, JobPriority::Low, "status job", [] {});

    job.set_status(JobStatus::Running);
    EXPECT_EQ(job.get_status(), JobStatus::Running);

    job.set_status(JobStatus::Completed);
    EXPECT_EQ(job.get_status(), JobStatus::Completed);
}

TEST(JobTest, ExecuteRunsTheStoredTask) {
    bool executed = false;
    Job job(5, JobPriority::Medium, "callable job", [&executed] {
        executed = true;
    });

    job.execute();

    EXPECT_TRUE(executed);
}

TEST(JobTest, ExecuteThrowsWhenTaskIsEmpty) {
    Job job(11, JobPriority::Medium, "empty task", {});

    EXPECT_THROW(job.execute(), std::runtime_error);
}

TEST(JobTest, ToStringReturnsReadablePriorityNames) {
    EXPECT_EQ(to_string(JobPriority::Low), "Low");
    EXPECT_EQ(to_string(JobPriority::Medium), "Medium");
    EXPECT_EQ(to_string(JobPriority::High), "High");
}

TEST(JobTest, ToStringReturnsReadableStatusNames) {
    EXPECT_EQ(to_string(JobStatus::Pending), "Pending");
    EXPECT_EQ(to_string(JobStatus::Running), "Running");
    EXPECT_EQ(to_string(JobStatus::Completed), "Completed");
    EXPECT_EQ(to_string(JobStatus::Failed), "Failed");
}

TEST(JobTest, ExecuteSupportsMockFunctionCallables) {
    ::testing::MockFunction<void()> mock_task;
    EXPECT_CALL(mock_task, Call()).Times(1);

    Job job(99, JobPriority::High, "mock task", [&mock_task] {
        mock_task.Call();
    });

    job.execute();
}

}  // namespace
}  // namespace scheduler
