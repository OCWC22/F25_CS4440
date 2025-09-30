# Part 4: Real-World Applications
## AWS Infrastructure Decisions & Performance Optimization

**Learning Objectives:**
- Apply OS concepts to AWS EC2 instance selection
- Master performance optimization strategies
- Understand NUMA-aware programming
- Learn lock-free algorithms
- Make informed infrastructure decisions

---

## 1. AWS EC2 Instance Selection Framework

### 1.1 Understanding Instance Types

**Intel Xeon Sapphire Rapids Instances (c7i family):**

```
┌──────────────────────────────────────────────────────────────┐
│ Instance     │ vCPUs │ Memory │ Network  │ Cost/hr │ Use Case│
├──────────────────────────────────────────────────────────────┤
│ c7i.large    │   2   │  4 GB  │ 12.5 Gbps│  $0.17  │ Small   │
│ c7i.xlarge   │   4   │  8 GB  │ 12.5 Gbps│  $0.34  │ Medium  │
│ c7i.2xlarge  │   8   │ 16 GB  │ 12.5 Gbps│  $0.68  │ Large   │
│ c7i.4xlarge  │  16   │ 32 GB  │ 12.5 Gbps│  $1.36  │ High    │
│ c7i.8xlarge  │  32   │ 64 GB  │ 12.5 Gbps│  $2.72  │ V.High  │
│ c7i.16xlarge │  64   │ 128 GB │ 25 Gbps  │  $5.44  │ Extreme │
│ c7i.48xlarge │ 192   │ 384 GB │ 50 Gbps  │ $16.32  │ Maximum │
└──────────────────────────────────────────────────────────────┘

Memory-Optimized (r7i family):
- Higher memory-to-CPU ratio (8 GB per vCPU)
- Better for databases, caching, in-memory analytics

General Purpose (m7i family):
- Balanced CPU and memory (4 GB per vCPU)
- Good for web servers, application servers
```

### 1.2 Decision Framework

**Question 1: CPU-Bound or I/O-Bound?**

```
CPU-Bound Workload (e.g., video encoding, scientific computing):
├─ Characteristics:
│  ├─ High CPU utilization (>80%)
│  ├─ Low wait time
│  └─ Benefits from more cores
├─ Choose: Compute-optimized (c7i)
└─ Scaling: More cores = better performance

I/O-Bound Workload (e.g., database, web server):
├─ Characteristics:
│  ├─ Low CPU utilization (<50%)
│  ├─ High wait time (disk, network)
│  └─ Benefits from faster I/O
├─ Choose: Memory-optimized (r7i) or General (m7i)
└─ Scaling: More memory/network = better performance

Mixed Workload:
├─ Profile your application first!
├─ Use CloudWatch metrics
└─ Start with general-purpose, adjust
```

**Question 2: Vertical vs. Horizontal Scaling?**

```
Scenario: Web server handling 10,000 requests/second

Option A: Single c7i.48xlarge (192 vCPUs)
├─ Cost: $16.32/hour = $11,750/month
├─ Pros:
│  ├─ Simple architecture
│  ├─ Low latency (no network hops)
│  └─ Easy to manage
└─ Cons:
   ├─ Single point of failure
   ├─ Limited by single machine
   └─ Expensive

Option B: 12× c7i.4xlarge (16 vCPUs each)
├─ Cost: $1.36/hour × 12 = $16.32/hour = $11,750/month
├─ Pros:
│  ├─ Fault-tolerant (one fails, others continue)
│  ├─ Flexible scaling (add/remove instances)
│  ├─ Better load distribution
│  └─ Can use spot instances (70% cheaper!)
└─ Cons:
   ├─ More complex (load balancer needed)
   ├─ Network latency between instances
   └─ Requires distributed architecture

CEO Decision: Option B for production
- Same cost, but much more reliable
- Can optimize further with spot instances
- Scales better as traffic grows
```

**Question 3: How Many Threads per Core?**

```
Rule of Thumb:
├─ CPU-bound: 1 thread per core (no SMT benefit)
├─ I/O-bound: 2 threads per core (SMT helps)
└─ Mixed: 1.5 threads per core

Example Calculation:
Web server handling 10,000 req/s
├─ Each request: 10ms CPU time
├─ Total CPU time: 10,000 × 10ms = 100 seconds/second
├─ Need: 100 cores (or 50 cores with SMT)
└─ Instance: c7i.48xlarge (96 cores) with headroom

Headroom Rule:
- Always provision 20-30% extra capacity
- Handles traffic spikes
- Allows for maintenance
```

### 1.3 Cost Optimization Strategies

**Strategy 1: Reserved Instances**

```
On-Demand vs. Reserved (1-year commitment):
├─ c7i.4xlarge on-demand: $1.36/hour
├─ c7i.4xlarge reserved: $0.82/hour (40% savings)
└─ Annual savings: $4,730 per instance

When to use:
├─ Baseline capacity (always running)
├─ Predictable workload
└─ 1-3 year commitment acceptable
```

**Strategy 2: Spot Instances**

```
Spot vs. On-Demand:
├─ c7i.4xlarge on-demand: $1.36/hour
├─ c7i.4xlarge spot: ~$0.41/hour (70% savings!)
└─ Risk: Can be terminated with 2-minute warning

When to use:
├─ Fault-tolerant workloads
├─ Batch processing
├─ Stateless web servers (with auto-scaling)
└─ Non-critical tasks

Best Practice:
- Mix of on-demand (baseline) + spot (burst capacity)
- Use spot for 70% of capacity, on-demand for 30%
```

**Strategy 3: Auto-Scaling**

```
Dynamic Scaling:
├─ Scale up during peak hours (9am-5pm)
├─ Scale down during off-peak (nights, weekends)
└─ Can save 40-60% on compute costs

Example:
Peak: 20 instances (9am-5pm weekdays)
Off-peak: 5 instances (nights, weekends)
Average: ~10 instances
Savings: 50% compared to running 20 instances 24/7
```

---

## 2. Performance Optimization Strategies

### 2.1 Cache Optimization

**Strategy 1: Sequential Access**

```c
// BAD: Column-major access (cache-unfriendly)
for (int col = 0; col < N; col++) {
    for (int row = 0; row < N; row++) {
        sum += matrix[row][col];  // Non-sequential!
    }
}
// Cache miss rate: ~90%

// GOOD: Row-major access (cache-friendly)
for (int row = 0; row < N; row++) {
    for (int col = 0; col < N; col++) {
        sum += matrix[row][col];  // Sequential!
    }
}
// Cache miss rate: ~1%

// Performance difference: 10-100x faster!
```

**Why?** Arrays are stored row-major in C. Sequential access loads entire cache lines (64 bytes), minimizing misses.

**Business Impact:**

```
Matrix multiplication (1000×1000):
- Cache-unfriendly: 10 seconds
- Cache-friendly: 0.1 seconds
- Savings: 100x faster = 99% less CPU time = 99% cost reduction
```

**Strategy 2: Data Structure Layout**

```c
// BAD: Array of Structures (AoS)
struct particle {
    float x, y, z;     // Position
    float vx, vy, vz;  // Velocity
    float mass;
    float charge;
};
struct particle particles[1000000];

// Access pattern (physics simulation):
for (int i = 0; i < N; i++) {
    particles[i].x += particles[i].vx * dt;
    particles[i].y += particles[i].vy * dt;
    particles[i].z += particles[i].vz * dt;
}
// Loads entire struct (32 bytes) but only uses 24 bytes
// Wasted bandwidth: 25%

// GOOD: Structure of Arrays (SoA)
struct particles {
    float x[1000000];
    float y[1000000];
    float z[1000000];
    float vx[1000000];
    float vy[1000000];
    float vz[1000000];
    float mass[1000000];
    float charge[1000000];
};

// Access pattern:
for (int i = 0; i < N; i++) {
    x[i] += vx[i] * dt;
    y[i] += vy[i] * dt;
    z[i] += vz[i] * dt;
}
// Sequential access, perfect cache utilization
// Performance: 2-4x faster!
```

### 2.2 Avoiding False Sharing

**Problem:** Multiple threads accessing different variables in the same cache line.

```c
// BAD: False sharing
struct counter {
    int count_a;  // Thread A writes (bytes 0-3)
    int count_b;  // Thread B writes (bytes 4-7)
};  // Both in same 64-byte cache line!

// What happens:
// 1. Thread A writes count_a → Core 0 cache line "Modified"
// 2. Thread B writes count_b → Core 1 must fetch from Core 0
// 3. Ping-pong effect: 10-100x slowdown!
```

**Solution: Padding**

```c
// GOOD: Separate cache lines
struct counter {
    int count_a;
    char padding1[60];  // Pad to 64 bytes
    int count_b;
    char padding2[60];
};

// Or use compiler attribute:
struct counter {
    int count_a __attribute__((aligned(64)));
    int count_b __attribute__((aligned(64)));
};

// Performance: 10-100x faster!
```

**Real-World Example:**

```c
// From project_2/bounded_buffer.c
typedef struct {
    char buffer[BUFFER_SIZE];
    int in;
    int out;
    int count;
    
    // These are protected by mutex, so false sharing is avoided
    // But in lock-free code, you'd need padding!
    pthread_mutex_t mutex;
    sem_t empty;
    sem_t full;
} bounded_buffer_t;
```

### 2.3 CPU Affinity

**Problem:** Thread migration causes cache misses.

```
Without Affinity:
Time 0: Thread runs on Core 0 → Warms up L1/L2 cache
Time 1: Scheduler moves thread to Core 5 → Cache cold!
Time 2: Thread warms up Core 5 cache
Time 3: Scheduler moves thread back to Core 0 → Cache cold again!

Result: Constant cache misses, poor performance
```

**Solution: Pin Threads to Cores**

```c
#define _GNU_SOURCE
#include <sched.h>
#include <pthread.h>

void pin_thread_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    
    pthread_t current = pthread_self();
    pthread_setaffinity_np(current, sizeof(cpuset), &cpuset);
}

void* worker(void* arg) {
    int core_id = *(int*)arg;
    pin_thread_to_core(core_id);
    
    // Thread now runs only on specified core
    // Cache stays warm!
    
    // Do work...
    
    return NULL;
}

int main() {
    pthread_t threads[4];
    int core_ids[4] = {0, 1, 2, 3};
    
    for (int i = 0; i < 4; i++) {
        pthread_create(&threads[i], NULL, worker, &core_ids[i]);
    }
    
    // ...
}
```

**Performance Impact:**
- Without pinning: Thread migrates, 10-50% slower
- With pinning: Thread stays on core, cache stays warm
- Improvement: 10-50% faster for cache-sensitive workloads

**When to Use:**
- Long-running threads
- Cache-sensitive workloads
- Real-time applications
- High-performance computing

**When NOT to Use:**
- Short-lived threads
- I/O-bound workloads
- Oversubscribed systems (more threads than cores)

---

## 3. NUMA-Aware Programming

### 3.1 Understanding NUMA

**Dual-Socket Server (e.g., AWS c7i.48xlarge):**

```
┌─────────────────────────────────────────────────┐
│ Socket 0 (NUMA Node 0)                          │
│  ├─ 96 vCPUs (48 cores × 2 threads)             │
│  ├─ 112.5MB L3 cache                            │
│  └─ 192 GB local memory                         │
└─────────────────────────────────────────────────┘
              ↕ UPI (Ultra Path Interconnect)
┌─────────────────────────────────────────────────┐
│ Socket 1 (NUMA Node 1)                          │
│  ├─ 96 vCPUs (48 cores × 2 threads)             │
│  ├─ 112.5MB L3 cache                            │
│  └─ 192 GB local memory                         │
└─────────────────────────────────────────────────┘

Memory Access Latency:
├─ Local memory: ~100 ns
├─ Remote memory: ~200 ns (2x slower!)
└─ Bandwidth: Local 2x higher than remote
```

### 3.2 NUMA-Aware Allocation

```c
#include <numa.h>

// Check NUMA availability
if (numa_available() < 0) {
    fprintf(stderr, "NUMA not available\n");
    exit(1);
}

// Get current NUMA node
int node = numa_node_of_cpu(sched_getcpu());
printf("Running on NUMA node %d\n", node);

// Allocate memory on local node
void* buffer = numa_alloc_onnode(size, node);

// Interleave memory across all nodes (for shared data)
void* shared = numa_alloc_interleaved(size);

// Pin thread to local node
struct bitmask* nodemask = numa_allocate_nodemask();
numa_bitmask_setbit(nodemask, node);
numa_bind(nodemask);
```

**Performance Impact:**

```c
// BAD: Thread on Node 0, data on Node 1
void* thread_func(void* arg) {
    // Thread pinned to Node 0
    // Data allocated on Node 1 (default allocation)
    for (int i = 0; i < N; i++) {
        sum += data[i];  // Cross-NUMA access (2x slower!)
    }
}

// GOOD: Thread and data on same node
void* thread_func(void* arg) {
    int node = numa_node_of_cpu(sched_getcpu());
    void* data = numa_alloc_onnode(size, node);
    
    for (int i = 0; i < N; i++) {
        sum += data[i];  // Local NUMA access (fast!)
    }
}

// Performance difference: 2x faster!
```

---

## 4. Lock-Free Algorithms

### 4.1 Why Lock-Free?

**Problem with Locks:**
- Contention causes blocking
- Context switches are expensive
- Priority inversion
- Deadlock risk

**Lock-Free Benefits:**
- No blocking (always makes progress)
- No context switches
- Better scalability
- No deadlock

### 4.2 Lock-Free Counter

```c
// Lock-based (slow under contention)
pthread_mutex_t lock;
int counter = 0;

void increment() {
    pthread_mutex_lock(&lock);
    counter++;
    pthread_mutex_unlock(&lock);
}

// Lock-free (fast!)
_Atomic int counter = 0;

void increment() {
    __atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST);
}

// Performance under high contention: 10-100x faster!
```

### 4.3 Lock-Free Stack (Treiber Stack)

```c
struct node {
    int data;
    struct node* next;
};

_Atomic(struct node*) stack_top = NULL;

void push(int data) {
    struct node* new_node = malloc(sizeof(struct node));
    new_node->data = data;
    
    struct node* old_top;
    do {
        old_top = atomic_load(&stack_top);
        new_node->next = old_top;
    } while (!atomic_compare_exchange_weak(&stack_top, &old_top, new_node));
}

int pop() {
    struct node* old_top;
    struct node* new_top;
    
    do {
        old_top = atomic_load(&stack_top);
        if (!old_top) return -1;  // Empty
        new_top = old_top->next;
    } while (!atomic_compare_exchange_weak(&stack_top, &old_top, new_top));
    
    int data = old_top->data;
    free(old_top);
    return data;
}
```

**How It Works:**

```
Push Operation:
1. Create new node
2. Load current top
3. Set new_node->next = top
4. CAS: if (stack_top == old_top) stack_top = new_node
5. If CAS fails (another thread modified), retry

Pop Operation:
1. Load current top
2. If empty, return error
3. Load top->next
4. CAS: if (stack_top == old_top) stack_top = top->next
5. If CAS fails, retry
6. Free old top, return data
```

**Business Analogy:** Lock-free is like a multi-lane highway with careful merging. Locks are like a single-lane bridge where only one car can cross at a time.

---

## 5. Complete Performance Checklist

### 5.1 Code-Level Optimizations

```
✓ Cache Optimization:
  ├─ Use sequential access patterns
  ├─ Prefer SoA over AoS for large datasets
  ├─ Align data to cache line boundaries (64 bytes)
  └─ Prefetch data when possible

✓ Synchronization:
  ├─ Minimize critical section size
  ├─ Use lock-free algorithms when possible
  ├─ Avoid false sharing (pad to 64 bytes)
  ├─ Batch operations to reduce lock frequency
  └─ Use reader-writer locks for read-heavy workloads

✓ Thread Management:
  ├─ Pin threads to cores (CPU affinity)
  ├─ Match thread count to core count
  ├─ Use thread pools (avoid creation overhead)
  └─ NUMA-aware allocation and binding
```

### 5.2 System-Level Optimizations

```
✓ AWS Instance Selection:
  ├─ Profile workload (CPU vs. I/O bound)
  ├─ Choose appropriate instance family
  ├─ Use horizontal scaling for reliability
  └─ Mix reserved + spot for cost optimization

✓ Operating System:
  ├─ Disable CPU frequency scaling (use performance governor)
  ├─ Disable transparent huge pages (THP) for latency-sensitive apps
  ├─ Tune kernel parameters (sysctl)
  └─ Use huge pages (2MB) for large memory allocations

✓ Monitoring:
  ├─ Track CPU utilization (CloudWatch)
  ├─ Monitor cache miss rate (perf)
  ├─ Measure context switch rate
  └─ Profile with perf, gprof, or Valgrind
```

### 5.3 Measurement Tools

```bash
# CPU performance counters
perf stat -e cache-misses,context-switches ./program

# Profile CPU usage
perf record -g ./program
perf report

# Memory profiling
valgrind --tool=cachegrind ./program

# NUMA statistics
numastat -p $(pidof program)

# System-wide monitoring
htop
iostat
vmstat
```

---

## Summary: Real-World Best Practices

### AWS Decision Framework
1. **Profile first:** Understand your workload (CPU vs. I/O)
2. **Right-size:** Don't overpay for unused capacity
3. **Horizontal scaling:** Better reliability than vertical
4. **Cost optimization:** Mix reserved + spot instances
5. **Auto-scaling:** Scale dynamically with demand

### Performance Optimization
1. **Cache is king:** Sequential access is 10-100x faster
2. **Avoid false sharing:** Pad to 64-byte cache lines
3. **Pin threads:** Prevent migration, keep cache warm
4. **NUMA awareness:** 2x performance on large instances
5. **Lock-free when possible:** 10-100x better under contention

### Measurement & Monitoring
1. **Always measure:** Don't guess, profile!
2. **Use perf:** CPU performance counters are invaluable
3. **Monitor production:** CloudWatch + custom metrics
4. **A/B test:** Compare optimizations empirically
5. **Document findings:** Share knowledge with team

### Next Steps
Review **Part 5: Code Examples** for complete annotated implementations of all concepts covered in this course.
