#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifndef MAX_JOBS
#define MAX_JOBS 50          
#endif
#define NAME_LEN   24        
#define INPUT_LEN  128       
#define MAX_PRIORITY 10      
#define MAX_GPU_GB   80      
#define MAX_JOB_GB   1000    
#define MAX_JOB_MIN  10000   

typedef struct {
    int  id;                 
    char name[NAME_LEN];     
    int  priority;           
    int  memGB;              
    int  timeMin;            
    int  seq;                
} Job;

typedef struct {
    Job data[MAX_JOBS];
    int size;                
} MinHeap;

typedef struct {
    int capacityGB;          
    int usedGB;              
    int hasRunning;          
    Job running;             
    int clockMin;            
} Gpu;

typedef struct {
    int submitted;           
    int completed;           
    int cancelled;           
} Stats;

static int parent(int i)     { return (i - 1) / 2; }
static int leftChild(int i)  { return 2 * i + 1;   }
static int rightChild(int i) { return 2 * i + 2;   }

static int comesBefore(const Job *a, const Job *b)
{
    if (a->priority != b->priority)
        return a->priority < b->priority;
    return a->seq < b->seq;
}

static void swapJobs(Job *a, Job *b)
{
    Job temp = *a;
    *a = *b;
    *b = temp;
}

static void initHeap(MinHeap *h) { h->size = 0; }

static void heapifyUp(MinHeap *h, int i)
{
    while (i > 0 && comesBefore(&h->data[i], &h->data[parent(i)])) {
        swapJobs(&h->data[i], &h->data[parent(i)]);
        i = parent(i);
    }
}

static void heapifyDown(MinHeap *h, int i)
{
    for (;;) {
        int best = i;
        int l = leftChild(i);
        int r = rightChild(i);

        if (l < h->size && comesBefore(&h->data[l], &h->data[best])) best = l;
        if (r < h->size && comesBefore(&h->data[r], &h->data[best])) best = r;

        if (best == i) break;                 
        swapJobs(&h->data[i], &h->data[best]);
        i = best;
    }
}

static int insertJob(MinHeap *h, Job job)
{
    if (h->size >= MAX_JOBS) return 0;
    h->data[h->size] = job;                   
    h->size++;
    heapifyUp(h, h->size - 1);                
    return 1;
}

static const Job *peekMin(const MinHeap *h)
{
    return (h->size == 0) ? NULL : &h->data[0];
}

static int extractMin(MinHeap *h, Job *out)
{
    if (h->size == 0) return 0;
    *out = h->data[0];
    h->data[0] = h->data[h->size - 1];        
    h->size--;
    heapifyDown(h, 0);                        
    return 1;
}

static int findJobIndexById(const MinHeap *h, int id)
{
    int i;
    for (i = 0; i < h->size; i++)
        if (h->data[i].id == id) return i;
    return -1;
}

static int removeJobById(MinHeap *h, int id, Job *removed)
{
    int idx = findJobIndexById(h, id);
    if (idx == -1) return 0;

    *removed = h->data[idx];
    h->data[idx] = h->data[h->size - 1];      
    h->size--;

    if (idx < h->size) {                      
        
        if (idx > 0 && comesBefore(&h->data[idx], &h->data[parent(idx)]))
            heapifyUp(h, idx);
        else
            heapifyDown(h, idx);
    }
    return 1;
}

static int isValidMinHeap(const MinHeap *h)
{
    int i;
    for (i = 1; i < h->size; i++)
        if (comesBefore(&h->data[i], &h->data[parent(i)])) return 0;
    return 1;
}

static int selectEligibleJob(MinHeap *h, int availableGB, Job *chosen)
{
    Job skipped[MAX_JOBS];
    int skippedCount = 0;
    int found = 0;
    Job current;
    int i;

    while (extractMin(h, &current)) {
        if (current.memGB <= availableGB) {
            *chosen = current;
            found = 1;
            break;
        }
        printf("  [Skipped] Job %d (%s) needs %d GB but only %d GB is available.\n",
               current.id, current.name, current.memGB, availableGB);
        skipped[skippedCount++] = current;
    }

    for (i = 0; i < skippedCount; i++)        
        insertJob(h, skipped[i]);

    return found;
}

static void displayJobsInPriorityOrder(const MinHeap *h, int capacityGB)
{
    MinHeap copy = *h;                        
    Job j;
    int rank = 1;
    int i;

    if (h->size == 0) {
        printf("The job queue is empty.\n");
        return;
    }

    printf("\nPending jobs in priority order (%d job(s)):\n", h->size);
    printf("Heap root via peekMin(): Job %d (%s)\n", peekMin(h)->id, peekMin(h)->name);
    printf("%-5s %-6s %-23s %-4s %-8s %-9s %-5s %-10s\n",
           "Rank", "ID", "Name", "Pri", "Mem(GB)", "Time(min)", "Sub#", "Fits GPU?");
    printf("--------------------------------------------------------------------------------\n");

    while (extractMin(&copy, &j)) {
        printf("%-5d %-6d %-23s %-4d %-8d %-9d %-5d %-10s\n",
               rank++, j.id, j.name, j.priority, j.memGB, j.timeMin, j.seq,
               (j.memGB <= capacityGB) ? "Yes" : "No (too big)");
    }

    printf("\nRaw heap array (index:id/priority): ");
    for (i = 0; i < h->size; i++)
        printf("[%d:%d/p%d] ", i, h->data[i].id, h->data[i].priority);
    printf("\nHeap property check: %s\n", isValidMinHeap(h) ? "OK" : "VIOLATED");
}

static void readLine(const char *prompt, char *buf, int size)
{
    printf("%s", prompt);
    if (fgets(buf, size, stdin) == NULL) {
        printf("\nInput ended. Exiting.\n");
        exit(0);
    }
    if (strchr(buf, '\n') == NULL) {          
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) { }
    } else {
        buf[strcspn(buf, "\n")] = '\0';
    }
}

static int readInt(const char *prompt, int min, int max)
{
    char line[INPUT_LEN];
    for (;;) {
        char *end;
        long value;
        readLine(prompt, line, sizeof line);
        value = strtol(line, &end, 10);
        while (isspace((unsigned char)*end)) end++;      
        if (end != line && *end == '\0' && value >= min && value <= max)
            return (int)value;
        printf("  Invalid input. Please enter a whole number from %d to %d.\n", min, max);
    }
}

static void readName(const char *prompt, char *name, int size)
{
    for (;;) {
        char *p;
        readLine(prompt, name, size);
        p = name;
        while (isspace((unsigned char)*p)) p++;          
        if (*p != '\0') {
            memmove(name, p, strlen(p) + 1);
            return;
        }
        printf("  Job name cannot be empty.\n");
    }
}

static void addJob(MinHeap *h, const Gpu *gpu, Stats *stats)
{
    Job job;
    int id;

    if (h->size >= MAX_JOBS) {
        printf("Queue is full (%d jobs). Execute or cancel a job first.\n", MAX_JOBS);
        return;
    }

    id = readInt("Job ID (1-999999): ", 1, 999999);
    if (findJobIndexById(h, id) != -1 || (gpu->hasRunning && gpu->running.id == id)) {
        printf("A job with ID %d already exists. IDs must be unique.\n", id);
        return;
    }

    job.id = id;
    readName("Job name: ", job.name, NAME_LEN);
    job.priority = readInt("Priority (1 = highest ... 10 = lowest): ", 1, MAX_PRIORITY);
    job.memGB    = readInt("GPU memory required (GB): ", 1, MAX_JOB_GB);
    job.timeMin  = readInt("Estimated execution time (minutes): ", 1, MAX_JOB_MIN);
    job.seq      = stats->submitted + 1;                 

    if (insertJob(h, job)) {
        stats->submitted++;
        printf("Job %d (%s) added to the queue.\n", job.id, job.name);
        if (job.memGB > gpu->capacityGB)
            printf("  Warning: it needs %d GB but the GPU has only %d GB, so it can never run.\n"
                   "  It will stay pending until cancelled.\n", job.memGB, gpu->capacityGB);
    } else {
        printf("Queue is full. Job not added.\n");
    }
}

static void executeNextJob(MinHeap *h, Gpu *gpu, Stats *stats)
{
    Job next;
    int available;

    if (gpu->hasRunning) {
        gpu->clockMin += gpu->running.timeMin;
        stats->completed++;
        printf("[Completed] Job %d (%s) finished after %d min. GPU memory released.\n",
               gpu->running.id, gpu->running.name, gpu->running.timeMin);
        gpu->hasRunning = 0;
        gpu->usedGB = 0;
    }

    if (h->size == 0) {
        printf("The job queue is empty. Nothing to execute.\n");
        return;
    }

    available = gpu->capacityGB - gpu->usedGB;
    if (!selectEligibleJob(h, available, &next)) {
        printf("No pending job fits in the available %d GB. Cancel the oversized job(s) "
               "or add smaller ones.\n", available);
        return;
    }

    gpu->running = next;
    gpu->hasRunning = 1;
    gpu->usedGB = next.memGB;
    printf("[Started] Job %d (%s) | priority %d | %d GB | est. %d min\n",
           next.id, next.name, next.priority, next.memGB, next.timeMin);
}

static void cancelJob(MinHeap *h, const Gpu *gpu, Stats *stats)
{
    Job removed;
    int id;

    if (h->size == 0 && !gpu->hasRunning) {
        printf("The job queue is empty. Nothing to cancel.\n");
        return;
    }

    id = readInt("Job ID to cancel: ", 1, 999999);

    if (gpu->hasRunning && gpu->running.id == id) {
        printf("Job %d is currently running and cannot be cancelled.\n", id);
        return;
    }

    if (removeJobById(h, id, &removed)) {
        stats->cancelled++;
        printf("Job %d (%s) cancelled and removed from the queue.\n", removed.id, removed.name);
    } else {
        printf("Job %d was not found in the queue.\n", id);
    }
}

static void displayGpuStatus(const Gpu *gpu)
{
    printf("\n--- GPU Status ---\n");
    printf("Total memory     : %d GB\n", gpu->capacityGB);
    printf("Memory in use    : %d GB\n", gpu->usedGB);
    printf("Memory available : %d GB\n", gpu->capacityGB - gpu->usedGB);
    if (gpu->hasRunning)
        printf("Running job      : %d (%s), est. %d min\n",
               gpu->running.id, gpu->running.name, gpu->running.timeMin);
    else
        printf("Running job      : none (GPU idle)\n");
    printf("Simulated time of completed work: %d min\n", gpu->clockMin);
}

static void displayStatistics(const MinHeap *h, const Gpu *gpu, const Stats *stats)
{
    printf("\n--- Job Statistics ---\n");
    printf("Total submitted : %d\n", stats->submitted);
    printf("Completed       : %d\n", stats->completed);
    printf("Cancelled       : %d\n", stats->cancelled);
    printf("Running         : %d\n", gpu->hasRunning ? 1 : 0);
    printf("Pending         : %d\n", h->size);
}

static void printMenu(void)
{
    printf("\n========== GPU JOB QUEUE SIMULATOR (Min-Heap) ==========\n");
    printf(" 1. Add a job\n");
    printf(" 2. Execute next eligible job\n");
    printf(" 3. Cancel a job\n");
    printf(" 4. Display job queue (priority order)\n");
    printf(" 5. Display GPU status\n");
    printf(" 6. Display job statistics\n");
    printf(" 7. Exit\n");
    printf("========================================================\n");
}

int main(void)
{
    MinHeap queue;
    Gpu gpu;
    Stats stats = {0, 0, 0};
    int choice;

    initHeap(&queue);
    gpu.usedGB = 0;
    gpu.hasRunning = 0;
    gpu.clockMin = 0;

    printf("GPU Job Queue Simulator (teaching simulation, not a real scheduler)\n");
    gpu.capacityGB = readInt("Enter simulated GPU memory capacity in GB (1-80, e.g. 6): ", 1, MAX_GPU_GB);

    do {
        printMenu();
        choice = readInt("Enter your choice (1-7): ", 1, 7);
        switch (choice) {
            case 1: addJob(&queue, &gpu, &stats);                       break;
            case 2: executeNextJob(&queue, &gpu, &stats);               break;
            case 3: cancelJob(&queue, &gpu, &stats);                    break;
            case 4: displayJobsInPriorityOrder(&queue, gpu.capacityGB); break;
            case 5: displayGpuStatus(&gpu);                             break;
            case 6: displayStatistics(&queue, &gpu, &stats);            break;
            case 7: printf("Exiting. No dynamic memory was used, so nothing to free. Goodbye!\n"); break;
        }
    } while (choice != 7);

    return 0;
}
