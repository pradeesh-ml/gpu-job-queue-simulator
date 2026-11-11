# GPU Job Queue Simulator Using a Min-Heap

A **menu-driven GPU job scheduling simulator implemented in C**, demonstrating how AI/ML workloads can be prioritised and scheduled using a manually implemented, array-based **Min-Heap** data structure.


## Key Features

- **Job Management:** Submit jobs with a unique ID, name, priority, GPU memory requirement, and estimated execution time.
- **Priority-Based Scheduling:** Select jobs according to priority using a Min-Heap.
- **Memory-Aware Scheduling:** Execute the highest-priority pending job that fits the available GPU memory.
- **Job Cancellation:** Remove pending jobs using their unique IDs.
- **Priority-Ordered Display:** Display pending jobs in their actual scheduling order.
- **GPU Resource Monitoring:** View simulated GPU capacity, memory utilisation, available memory, and running job.
- **Job Statistics:** Track submitted, completed, cancelled, running, and pending jobs.
- **Input Validation:** Handle invalid input, duplicate IDs, empty queues, and queue capacity limits.

## Technology Stack

| Component | Technology |
|---|---|
| Programming Language | C |
| Standard | C99 |
| Data Structure | Array-Based Min-Heap |
| Compiler | GCC / MinGW |
| Dependencies | C Standard Library |

No external libraries are required.

## Getting Started

### Prerequisites

- GCC or any C99-compatible compiler.
- Windows, Linux, macOS, or WSL.

### Compilation and Execution

**Linux / macOS / WSL**

```bash
gcc -Wall -Wextra -std=c99 -o gpu_job_queue gpu_job_queue.c
./gpu_job_queue
```

**Windows (MinGW)**

```bash
gcc -Wall -Wextra -std=c99 -o gpu_job_queue.exe gpu_job_queue.c
gpu_job_queue.exe
```

To test queue capacity handling with a smaller heap:

```bash
gcc -std=c99 -DMAX_JOBS=3 -o gpu_small gpu_job_queue.c
```

## Application Menu

On startup, the application prompts the user to configure the simulated GPU memory capacity (1–80 GB).

```text
========== GPU JOB QUEUE SIMULATOR (MIN-HEAP) ==========
 1. Add a job
 2. Execute next eligible job
 3. Cancel a job
 4. Display job queue (priority order)
 5. Display GPU status
 6. Display job statistics
 7. Exit
========================================================
```

## Scheduling Algorithm

The simulator follows these scheduling rules:

1. **Priority Ordering:** Lower priority values represent higher priority. Priority 1 is the highest, while priority 10 is the lowest.
2. **FIFO Tie-Breaking:** Jobs with equal priority are scheduled in their submission order.
3. **Memory Eligibility:** A job can execute only when its memory requirement does not exceed the currently available GPU memory.
4. **Non-Blocking Scheduling:** If the highest-priority job cannot fit into available memory, it is temporarily skipped. The scheduler continues searching for an eligible job instead of blocking the queue.
5. **Single-Job Execution:** The simulated GPU executes one job at a time.

### Execution Model

Each execution operation follows two steps:

- Complete the currently running job and release its allocated memory.
- Select and start the next eligible job from the pending queue.

Consequently, the completed job count is one behind the started job count while a job is running.

## Example Execution

Consider a simulated GPU with a memory capacity of **6 GB** and the following submitted jobs:

| Job ID | Job Name | Priority | Memory (GB) | Time (min) |
|---|---|---:|---:|---:|
| 101 | ResNet Training | 2 | 3 | 20 |
| 102 | LLM Fine-Tuning | 1 | 5 | 45 |
| 103 | CNN Experiment | 3 | 2 | 15 |
| 104 | BERT Training | 2 | 4 | 30 |

### Priority-Ordered Queue

When the user selects option 4, the application displays the jobs in their scheduling order:

```text
Pending jobs in priority order (4 job(s)):
Heap root via peekMin(): Job 102 (LLM Fine-Tuning)

Rank  ID    Name                  Pri  Mem(GB)  Time(min)  Sub#  Fits GPU?
----------------------------------------------------------------------------
1     102   LLM Fine-Tuning       1    5        45         2     Yes
2     101   ResNet Training       2    3        20         1     Yes
3     104   BERT Training         2    4        30         4     Yes
4     103   CNN Experiment        3    2        15         3     Yes

Raw heap array (index:id/priority):
[0:102/p1] [1:101/p2] [2:103/p3] [3:104/p2]

Heap property check: OK
```

**Important:** The internal heap array is not necessarily sorted. It only guarantees that every parent node has a priority no greater than its children. Therefore, the priority-ordered display operates on a copy of the heap and repeatedly extracts the minimum element without modifying the original queue.

### Memory-Aware Scheduling

Consider an additional job requiring 8 GB of GPU memory.

Since the simulated GPU has only 6 GB available, this job cannot execute.

```text
[Skipped] Job 105 (Giant Model Pretraining)
          Requires 8 GB; only 6 GB available.

[Started] Job 101 (ResNet Training)
          Priority: 2 | Memory: 3 GB | Estimated time: 20 min
```

The scheduler temporarily removes ineligible jobs, searches for an eligible job, and reinserts the skipped jobs after selection.

## Implementation Details

### 1. Min-Heap Data Structure

The pending jobs are maintained using an array-based Min-Heap.

Each job is ordered using two attributes:

- Primary key: Priority (ascending).
- Secondary key: Submission sequence number (ascending).

This ensures deterministic scheduling and FIFO ordering for jobs with equal priorities.

### 2. Memory-Aware Job Selection

The simulator implements a **Pop–Skip–Reinsert** strategy:

1. Extract the minimum-priority job from the heap.
2. Check whether its memory requirement fits the available GPU memory.
3. If eligible, select it for execution.
4. Otherwise, temporarily store it in an auxiliary array.
5. Continue extracting jobs until an eligible job is found or the heap becomes empty.
6. Reinsert all temporarily skipped jobs.

Original submission sequence numbers are preserved, maintaining the scheduling order and heap properties.

### 3. Job Cancellation

Job cancellation is implemented without rebuilding the entire heap:

1. Search for the requested job ID using linear search.
2. Replace the target element with the last element in the heap.
3. Reduce the heap size.
4. Restore the Min-Heap property using either heapify-up or heapify-down.

### 4. Priority-Ordered Display

The internal heap array does not represent a completely sorted sequence.

To display jobs in true priority order, the application:

1. Creates a copy of the existing heap.
2. Repeatedly extracts the minimum element from the copy.
3. Displays the extracted jobs in scheduling order.

The original pending queue remains unchanged.

## Time and Space Complexity

Let `n` represent the number of pending jobs and `k` represent the number of ineligible jobs skipped during memory-aware selection.

| Operation | Time Complexity |
|---|---|
| Insert Job | O(log n) |
| Peek Minimum | O(1) |
| Extract Minimum | O(log n) |
| Search Job by ID | O(n) |
| Remove Job by ID | O(n) + O(log n) |
| Priority-Ordered Display | O(n log n) |
| Memory-Aware Selection | O((k + 1) log n) |

**Additional space complexity:**

- Heap storage: O(n)
- Sorted display: O(n) auxiliary space.
- Memory-aware selection: O(k) temporary storage.

Memory-aware selection has O(log n) best-case time when the first extracted job is eligible and O(n log n) worst-case time when all jobs must be examined.

## Code Organisation

The implementation is organised into functional modules to maintain readability and separation of concerns.

| Module | Functions |
|---|---|
| Heap Indexing | `parent`, `leftChild`, `rightChild` |
| Job Ordering | `comesBefore` |
| Heap Operations | `heapifyUp`, `heapifyDown`, `insertJob`, `peekMin`, `extractMin` |
| Job Management | `findJobIndexById`, `removeJobById` |
| Heap Validation | `isValidMinHeap` |
| Scheduling | `selectEligibleJob`, `executeNextJob` |
| Display | `displayJobsInPriorityOrder`, `displayGpuStatus`, `displayStatistics` |
| Input Handling | `readLine`, `readInt`, `readName` |
| Menu Operations | `addJob`, `cancelJob`, `main` |

## Testing and Validation

The implementation has been tested against the following scenarios:

- Jobs with different priority levels.
- Multiple jobs with identical priorities.
- FIFO ordering for equal-priority jobs.
- Execution of the highest-priority eligible job.
- Memory-ineligible jobs and non-blocking scheduling.
- Cancellation of existing and nonexistent jobs.
- Empty queue handling.
- Invalid input and duplicate job IDs.
- Maximum queue capacity handling.
- Min-Heap property validation.

### Randomised Stress Testing

An additional randomised stress test was performed:

- 2,000 test rounds.
- 300 random insert, select, and remove operations per round.
- Results compared against a brute-force reference implementation.
- Min-Heap property validated after every operation.

**Result:** All tested scenarios passed.

## Limitations

- The application is a simulation and does not execute actual GPU workloads.
- Only one job can run at a time.
- GPU memory allocation is modelled against the full configured capacity.
- The queue has a fixed capacity of 50 jobs by default.
- Priority aging is not implemented, so lower-priority jobs may experience starvation.
- Completed and cancelled job IDs are not retained for duplicate detection.
- Job information is not persisted between application sessions.
- Execution times are estimates and are not measured or simulated in real time.

## Future Enhancements

Potential extensions include:

- **Concurrent Job Execution:** Support multiple jobs running simultaneously when combined GPU memory requirements permit.
- **Priority Aging:** Dynamically increase the priority of waiting jobs to reduce starvation.
- **Hash-Based Job Lookup:** Introduce an ID-to-index hash map for faster job cancellation.
- **Dynamic Heap Allocation:** Replace the fixed-size array with dynamically allocated storage.
- **Persistent Storage:** Implement file-based queue saving and restoration.
- **Advanced Scheduling Policies:** Explore additional scheduling strategies such as Shortest Job First and weighted priority scheduling.

## Project Scope

This project focuses on demonstrating core Data Structures and Algorithms concepts through a practical GPU job scheduling scenario.

The implementation deliberately excludes:

- CUDA and real GPU APIs.
- Machine learning frameworks and model execution.
- Operating-system process scheduling.
- Multithreading and concurrent execution.
- Database integration.
- Web-based interfaces.

## License

Released for educational use. An appropriate open-source license, such as the MIT License, can be added before public distribution.