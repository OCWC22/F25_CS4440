# Banker's Algorithm: Complete Step-by-Step Tutorial

## Table of Contents
1. [What is the Banker's Algorithm?](#what-is-the-bankers-algorithm)
2. [Key Data Structures](#key-data-structures)
3. [The Safety Algorithm](#the-safety-algorithm)
4. [The Resource-Request Algorithm](#the-resource-request-algorithm)
5. [Complete Walkthrough with Examples](#complete-walkthrough-with-examples)
6. [C Implementation](#c-implementation)
7. [Common Mistakes and Clarifications](#common-mistakes-and-clarifications)

## What is the Banker's Algorithm?

The **Banker's Algorithm** is a **deadlock avoidance** algorithm developed by Edsger Dijkstra. It ensures that the system never enters an **unsafe state** where deadlock could occur.

**Key Idea:** Before granting any resource request, the algorithm checks if the system will remain in a **safe state**. If the request would lead to an **unsafe state**, the request is denied.

**Safety vs. Deadlock:**
- **Safe State**: There exists at least one sequence of process execution that allows all processes to complete
- **Unsafe State**: No such sequence exists (deadlock may or may not occur)
- **Deadlock**: All processes are blocked waiting for resources that will never become available

## Key Data Structures

### 1. Available Vector
```
Available[j] = Number of instances of resource type Rj currently available
```

### 2. Max Matrix
```
Max[i][j] = Maximum number of instances of resource type Rj that process Pi may need
```

### 3. Allocation Matrix
```
Allocation[i][j] = Number of instances of resource type Rj currently allocated to process Pi
```

### 4. Need Matrix
```
Need[i][j] = Max[i][j] - Allocation[i][j]
            = Remaining instances of resource type Rj that process Pi still needs
```

### 5. Work and Finish Vectors (for Safety Algorithm)
```
Work[j] = Temporary copy of Available
Finish[i] = Boolean indicating if process Pi can complete
```

## The Safety Algorithm

**Purpose:** Determine if the current system state is safe.

**Algorithm Steps:**
```
1. Let Work = Available
2. Let Finish[i] = false for all i

3. Find a process Pi such that:
   - Finish[i] = false, AND
   - Need[i] ≤ Work (component-wise comparison)

   If no such Pi exists, go to step 5.

4. For the found process Pi:
   - Work = Work + Allocation[i]  (simulate Pi finishing and releasing resources)
   - Finish[i] = true
   - Go to step 3

5. If Finish[i] = true for all processes:
   - System is SAFE
   - The sequence of processes found is a safe sequence
Else:
   - System is UNSAFE
```

## The Resource-Request Algorithm

**Purpose:** Determine if a resource request can be safely granted.

**Algorithm Steps:**
```
Given Request[i] from process Pi:

1. Check if Request[i] ≤ Need[i]:
   - If not, error (process exceeded maximum claim)

2. Check if Request[i] ≤ Available:
   - If not, process must wait (resources not available)

3. Tentatively allocate resources:
   - Available = Available - Request[i]
   - Allocation[i] = Allocation[i] + Request[i]
   - Need[i] = Need[i] - Request[i]

4. Run Safety Algorithm on new state:
   - If safe: allocation is permanent
   - If unsafe: rollback (undo step 3) and process must wait
```

## Complete Walkthrough with Examples

### Example 1: Safe State Analysis

**System Setup:**
- **Resources**: 2 types (R1, R2)
- **Processes**: 3 processes (P0, P1, P2)
- **Total Resources**: R1 = 7, R2 = 5

**Initial State:**
```
Allocation:
    R1  R2
P0:  1   0
P1:  2   1
P2:  3   1

Max:
    R1  R2
P0:  3   2
P1:  4   2
P2:  7   3
```

#### Step 1: Calculate Need Matrix
```
Need = Max - Allocation

For P0: Need[0] = (3,2) - (1,0) = (2,2)
For P1: Need[1] = (4,2) - (2,1) = (2,1)
For P2: Need[2] = (7,3) - (3,1) = (4,2)

Need Matrix:
    R1  R2
P0:  2   2
P1:  2   1
P2:  4   2
```

#### Step 2: Calculate Available
```
Sum of Allocations:
R1: 1 + 2 + 3 = 6
R2: 0 + 1 + 1 = 2

Available = Total - Sum(Allocation)
Available = (7,5) - (6,2) = (1,3)
```

#### Step 3: Run Safety Algorithm

**Initial State:**
```
Work = Available = (1,3)
Finish = [false, false, false]
```

**Iteration 1: Find a process with Need ≤ Work**

Check P0: Need[0] = (2,2) ≤ Work = (1,3)?
- 2 ≤ 1? NO → P0 cannot run

Check P1: Need[1] = (2,1) ≤ Work = (1,3)?
- 2 ≤ 1? NO → P1 cannot run

Check P2: Need[2] = (4,2) ≤ Work = (1,3)?
- 4 ≤ 1? NO → P2 cannot run

**Result:** No process can run → **SYSTEM IS UNSAFE**

This means the current allocation pattern could lead to deadlock!

### Example 2: Resource Request Analysis

**Continuing from Example 1's unsafe state...**

**New Request:** P1 requests Request[1] = (1,0)

#### Step 1: Check Request ≤ Need
```
Need[1] = (2,1)
Request[1] = (1,0)

(1,0) ≤ (2,1)? YES (1≤2 and 0≤1) ✓
```

#### Step 2: Check Request ≤ Available
```
Available = (1,3)
Request[1] = (1,0)

(1,0) ≤ (1,3)? YES (1≤1 and 0≤3) ✓
```

#### Step 3: Tentatively Allocate
```
New Available = (1,3) - (1,0) = (0,3)
New Allocation[1] = (2,1) + (1,0) = (3,1)
New Need[1] = (2,1) - (1,0) = (1,1)

Updated State:
Available = (0,3)

Allocation:
    R1  R2
P0:  1   0
P1:  3   1  ← Updated
P2:  3   1

Need:
    R1  R2
P0:  2   2
P1:  1   1  ← Updated
P2:  4   2
```

#### Step 4: Run Safety Algorithm on New State

**Initial State:**
```
Work = Available = (0,3)
Finish = [false, false, false]
```

**Iteration 1: Find process with Need ≤ Work**

Check P0: Need[0] = (2,2) ≤ Work = (0,3)?
- 2 ≤ 0? NO → P0 cannot run

Check P1: Need[1] = (1,1) ≤ Work = (0,3)?
- 1 ≤ 0? NO → P1 cannot run

Check P2: Need[2] = (4,2) ≤ Work = (0,3)?
- 4 ≤ 0? NO → P2 cannot run

**Result:** Still no process can run → **NEW STATE IS UNSAFE**

**Final Decision:** **DENY REQUEST** - P1 must wait

### Example 3: Safe State with Successful Requests

**Let's try a different initial allocation:**

**New Initial State (Safe):**
```
Allocation:
    R1  R2
P0:  0   1
P1:  2   0
P2:  2   1

Max:
    R1  R2
P0:  7   5
P1:  3   2
P2:  9   0

Total Resources: R1 = 10, R2 = 7
```

#### Step 1: Calculate Need Matrix
```
Need[0] = (7,5) - (0,1) = (7,4)
Need[1] = (3,2) - (2,0) = (1,2)
Need[2] = (9,0) - (2,1) = (7,-1) = (7,0) [Can't have negative, so (7,0)]

Need Matrix:
    R1  R2
P0:  7   4
P1:  1   2
P2:  7   0
```

#### Step 2: Calculate Available
```
Sum of Allocations:
R1: 0 + 2 + 2 = 4
R2: 1 + 0 + 1 = 2

Available = (10,7) - (4,2) = (6,5)
```

#### Step 3: Run Safety Algorithm

**Initial State:**
```
Work = Available = (6,5)
Finish = [false, false, false]
```

**Iteration 1: Find process with Need ≤ Work**

Check P0: Need[0] = (7,4) ≤ (6,5)?
- 7 ≤ 6? NO → P0 cannot run

Check P1: Need[1] = (1,2) ≤ (6,5)?
- 1 ≤ 6? YES
- 2 ≤ 5? YES → **P1 CAN RUN!**

**Execute P1:**
```
Work = Work + Allocation[1] = (6,5) + (2,0) = (8,5)
Finish[1] = true

Current State:
Work = (8,5)
Finish = [false, true, false]
```

**Iteration 2: Find another process with Need ≤ Work**

Check P0: Need[0] = (7,4) ≤ (8,5)?
- 7 ≤ 8? YES
- 4 ≤ 5? YES → **P0 CAN RUN!**

Check P2: Need[2] = (7,0) ≤ (8,5)?
- 7 ≤ 8? YES
- 0 ≤ 5? YES → **P2 CAN RUN!**

**Let's execute P0:**
```
Work = Work + Allocation[0] = (8,5) + (0,1) = (8,6)
Finish[0] = true

Current State:
Work = (8,6)
Finish = [true, true, false]
```

**Iteration 3: Find remaining process**

Check P2: Need[2] = (7,0) ≤ (8,6)?
- 7 ≤ 8? YES
- 0 ≤ 6? YES → **P2 CAN RUN!**

**Execute P2:**
```
Work = Work + Allocation[2] = (8,6) + (2,1) = (10,7)
Finish[2] = true

Final State:
Work = (10,7)
Finish = [true, true, true]
```

**Result:** All processes can finish → **SYSTEM IS SAFE**
**Safe Sequence:** P1 → P0 → P2

## C Implementation

Here's a complete C implementation of the Banker's Algorithm:

```c
#include <stdio.h>
#include <stdbool.h>

#define MAX_PROCESSES 10
#define MAX_RESOURCES 10

int processes, resources;
int available[MAX_RESOURCES];
int max[MAX_PROCESSES][MAX_RESOURCES];
int allocation[MAX_PROCESSES][MAX_RESOURCES];
int need[MAX_PROCESSES][MAX_RESOURCES];

// Calculate Need matrix
void calculate_need() {
    for (int i = 0; i < processes; i++) {
        for (int j = 0; j < resources; j++) {
            need[i][j] = max[i][j] - allocation[i][j];
        }
    }
}

// Safety Algorithm
bool is_safe() {
    int work[MAX_RESOURCES];
    bool finish[MAX_PROCESSES];
    int safe_sequence[MAX_PROCESSES];
    int safe_count = 0;

    // Initialize Work = Available, Finish = false
    for (int i = 0; i < resources; i++) {
        work[i] = available[i];
    }
    for (int i = 0; i < processes; i++) {
        finish[i] = false;
    }

    // Find process that can finish
    while (safe_count < processes) {
        bool found = false;

        for (int i = 0; i < processes; i++) {
            if (!finish[i]) {
                // Check if Need[i] <= Work
                bool can_run = true;
                for (int j = 0; j < resources; j++) {
                    if (need[i][j] > work[j]) {
                        can_run = false;
                        break;
                    }
                }

                if (can_run) {
                    // Process can finish
                    printf("Process P%d can finish with Work = (", i);
                    for (int j = 0; j < resources; j++) {
                        printf("%d", work[j]);
                        if (j < resources - 1) printf(", ");
                    }
                    printf(")\n");

                    // Add allocation to Work
                    for (int j = 0; j < resources; j++) {
                        work[j] += allocation[i][j];
                    }

                    finish[i] = true;
                    safe_sequence[safe_count++] = i;
                    found = true;

                    printf("After P%d finishes, Work = (", i);
                    for (int j = 0; j < resources; j++) {
                        printf("%d", work[j]);
                        if (j < resources - 1) printf(", ");
                    }
                    printf(")\n\n");
                }
            }
        }

        if (!found) {
            printf("No process can finish. System is UNSAFE!\n");
            return false;
        }
    }

    printf("System is SAFE! Safe sequence: ");
    for (int i = 0; i < safe_count; i++) {
        printf("P%d", safe_sequence[i]);
        if (i < safe_count - 1) printf(" -> ");
    }
    printf("\n\n");

    return true;
}

// Resource Request Algorithm
bool request_resources(int process_num, int request[]) {
    printf("\n=== Request from P%d ===\n", process_num);
    printf("Request: (");
    for (int i = 0; i < resources; i++) {
        printf("%d", request[i]);
        if (i < resources - 1) printf(", ");
    }
    printf(")\n");

    // Step 1: Check if Request <= Need
    for (int i = 0; i < resources; i++) {
        if (request[i] > need[process_num][i]) {
            printf("ERROR: Request exceeds maximum claim\n");
            return false;
        }
    }

    // Step 2: Check if Request <= Available
    for (int i = 0; i < resources; i++) {
        if (request[i] > available[i]) {
            printf("Resources not available. Process must wait.\n");
            return false;
        }
    }

    // Step 3: Tentatively allocate
    printf("Tentatively allocating resources...\n");
    for (int i = 0; i < resources; i++) {
        available[i] -= request[i];
        allocation[process_num][i] += request[i];
        need[process_num][i] -= request[i];
    }

    // Step 4: Check if new state is safe
    printf("Checking if new state is safe...\n");
    if (is_safe()) {
        printf("Request GRANTED!\n");
        return true;
    } else {
        printf("Request DENIED! Rolling back...\n");
        // Rollback
        for (int i = 0; i < resources; i++) {
            available[i] += request[i];
            allocation[process_num][i] -= request[i];
            need[process_num][i] += request[i];
        }
        return false;
    }
}

// Print system state
void print_state() {
    printf("\n=== Current System State ===\n");

    printf("Available: (");
    for (int i = 0; i < resources; i++) {
        printf("%d", available[i]);
        if (i < resources - 1) printf(", ");
    }
    printf(")\n\n");

    printf("Process | Allocation | Max | Need\n");
    printf("--------|-------------|-----|-----\n");
    for (int i = 0; i < processes; i++) {
        printf("P%d     | (", i);
        for (int j = 0; j < resources; j++) {
            printf("%d", allocation[i][j]);
            if (j < resources - 1) printf(",");
        }
        printf(") | (");
        for (int j = 0; j < resources; j++) {
            printf("%d", max[i][j]);
            if (j < resources - 1) printf(",");
        }
        printf(") | (");
        for (int j = 0; j < resources; j++) {
            printf("%d", need[i][j]);
            if (j < resources - 1) printf(",");
        }
        printf(")\n");
    }
    printf("\n");
}

int main() {
    printf("=== Banker's Algorithm Implementation ===\n\n");

    // Input example values (you can modify these)
    processes = 3;
    resources = 2;

    // Available resources
    available[0] = 1;  // R1
    available[1] = 3;  // R2

    // Max claims
    max[0][0] = 3; max[0][1] = 2;  // P0: (3,2)
    max[1][0] = 4; max[1][1] = 2;  // P1: (4,2)
    max[2][0] = 7; max[2][1] = 3;  // P2: (7,3)

    // Current allocation
    allocation[0][0] = 1; allocation[0][1] = 0;  // P0: (1,0)
    allocation[1][0] = 2; allocation[1][1] = 1;  // P1: (2,1)
    allocation[2][0] = 3; allocation[2][1] = 1;  // P2: (3,1)

    // Calculate Need matrix
    calculate_need();

    // Print initial state
    print_state();

    // Check if current state is safe
    printf("=== Safety Analysis ===\n");
    is_safe();

    // Example requests
    int request1[] = {1, 0};  // P1 requests (1,0)
    int request2[] = {0, 1};  // P0 requests (0,1)

    request_resources(1, request1);
    print_state();

    request_resources(0, request2);
    print_state();

    return 0;
}
```

## Common Mistakes and Clarifications

### 1. Component-wise Comparison
When checking `Need[i] ≤ Work`, this means:
```
For ALL resource types j:
    Need[i][j] ≤ Work[j]
```
NOT the sum of resources!

### 2. Negative Need Values
`Need[i][j] = Max[i][j] - Allocation[i][j]` should never be negative.
If it is, there's an error in your input data.

### 3. Available Calculation
```
Available[j] = Total[j] - sum(Allocation[*][j])
```
Make sure to sum across all processes for each resource type.

### 4. Safe Sequence Order
The order in which processes can finish matters for the safety check.
Any valid safe sequence proves the system is safe.

### 5. Request vs. Need
```
Request[i] must be ≤ Need[i]
```
A process can never request more than its maximum claim.

### 6. System State Changes
When testing a request:
1. **Always** run the safety algorithm on the **new** state
2. **Always** rollback if the new state is unsafe
3. **Never** grant a request that makes the system unsafe

### 7. Edge Cases
- **Zero allocation**: Process may have Max > 0 but Allocation = 0
- **Zero need**: Process may have Max = Allocation (already has all it needs)
- **Zero available**: System may have no free resources but still be safe

## Practice Problems

### Problem 1: Basic Safety Check
```
Resources: A=10, B=5, C=7

Allocation:
    A  B  C
P0:  0  1  0
P1:  2  0  0
P2:  3  0  2
P3:  2  1  1
P4:  0  0  2

Max:
    A  B  C
P0:  7  5  3
P1:  3  2  2
P2:  9  0  2
P3:  2  2  2
P4:  4  3  3

Question: Is the system safe? If yes, provide a safe sequence.
```

### Problem 2: Resource Request
```
System from Problem 1 is safe.

P1 requests (1,0,2)
Question: Should this request be granted? Show your work.
```

### Problem 3: Unsafe State Detection
```
Resources: R1=5, R2=3

Allocation:
    R1  R2
P0:  2   0
P1:  1   1
P2:  2   1

Max:
    R1  R2
P0:  3   2
P1:  2   2
P2:  4   2

Question: Is this state safe? Explain why or why not.
```

## Summary

The Banker's Algorithm is a powerful tool for deadlock avoidance that works by:

1. **Tracking resource usage** through Allocation, Max, and Need matrices
2. **Checking safety** by finding at least one sequence of process completion
3. **Controlling resource allocation** by only granting requests that maintain safety
4. **Preventing deadlock** by never allowing the system to enter unsafe states

**Key Takeaways:**
- Safe ≠ Deadlock-free (unsafe states may not always deadlock)
- The algorithm is conservative (may deny requests that could be safe)
- It requires knowing maximum resource needs in advance
- Real systems often use detection and recovery instead of avoidance

---

*This tutorial provides a comprehensive understanding of the Banker's Algorithm with practical examples and implementation details for operating systems coursework and real-world applications.*