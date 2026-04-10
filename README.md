# Task Scheduler Example

This repository contains a small modern C++ task scheduler example with:

- priority-based job scheduling
- job status tracking
- worker-thread execution
- a simple demo application

This project is intentionally small and educational. It is not meant to be a production-ready scheduler.

## Purpose

This repository is mainly an example to help understand how Codex can analyze, scaffold, extend, and explain a codebase.

The code shows a simple progression:

- define a `Job` model
- manage jobs with a `Scheduler`
- run jobs concurrently using worker threads
- preserve priority-based dispatch

## Project Structure

- `include/scheduler/job.hpp`: public `Job` type, priorities, and statuses
- `include/scheduler/scheduler.hpp`: public `Scheduler` API
- `src/job.cpp`: `Job` implementation
- `src/scheduler.cpp`: scheduler logic, priority queue, worker threads
- `src/main.cpp`: basic demo program
- `CMakeLists.txt`: build configuration

## Features

- `Job` contains:
  - unique ID
  - priority
  - status
  - description
  - `std::function<void()>` task
- `Scheduler` supports:
  - job creation
  - job submission
  - concurrent execution with worker threads
  - priority-based scheduling
  - waiting for all jobs to finish
- status transitions:
  - `Pending`
  - `Running`
  - `Completed`
  - `Failed`

## Build

From the project root on Windows PowerShell:

```powershell
& 'C:\Program Files\CMake\bin\cmake.exe' -S . -B build
$env:Path = $env:PATH; Remove-Item Env:PATH -ErrorAction SilentlyContinue
& 'C:\Program Files\CMake\bin\cmake.exe' --build build
```

If `cmake` is already available on your `PATH`, you can use:

```powershell
cmake -S . -B build
cmake --build build
```

## Run

```powershell
& '.\build\Debug\task_scheduler_demo.exe'
```

## Notes

- This is a learning example, not a full task scheduling framework.
- The current demo uses simple in-process worker threads.
- A natural next step would be adding cancellation, retries, timestamps, and tests.
