// mh.c — Mother Hubbard (children, mother, father)
// Build: gcc -pthread -o mh mh.c
// Run:   ./mh 100   (for 100 cycles)

#define _XOPEN_SOURCE 700
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define KIDS 12

// Shared state for passing "bathed kids" to Father
static int qbuf[KIDS];                // simple ring buffer per day
static int qhead = 0, qtail = 0;      // positions within [0..KIDS-1]
static pthread_mutex_t qmtx = PTHREAD_MUTEX_INITIALIZER;
static sem_t qItems;                  // how many bathed kids are waiting
static sem_t qSpaces;                 // free slots (prevents overflow)

// Day / cycle coordination
static sem_t motherGo;                // Mother controls start of a day (starts OPEN)
static sem_t nextDayReady;            // Father signals when all 12 are in bed

// Bookkeeping
static int Ncycles = 1;

// push child id into queue
static void qpush(int childId) {
    sem_wait(&qSpaces);
    pthread_mutex_lock(&qmtx);
    qbuf[qtail] = childId;
    qtail = (qtail + 1) % KIDS;
    pthread_mutex_unlock(&qmtx);
    sem_post(&qItems);
}

// pop child id from queue
static int qpop(void) {
    sem_wait(&qItems);
    pthread_mutex_lock(&qmtx);
    int id = qbuf[qhead];
    qhead = (qhead + 1) % KIDS;
    pthread_mutex_unlock(&qmtx);
    sem_post(&qSpaces);
    return id;
}

// Printing helpers
static void print_day_begin(int day) {
    printf("This is day #%d of a day in the life of Mother Hubbard.\n", day);
}

static void print_day_end(int day) {
    printf("End of day #%d in the life of Mother Hubbard.\n", day);
}

// Mother thread
static void *mother_thread(void *arg) {
    for (int day = 1; day <= Ncycles; ++day) {
        // Mother wakes up to start the day
        sem_wait(&motherGo);  // blocked during Father's "sleep / hand-off"
        print_day_begin(day);
        printf("Mother is waking up to take care of the children.\n");

        // Reset the per-day queue state
        pthread_mutex_lock(&qmtx);
        qhead = qtail = 0;
        pthread_mutex_unlock(&qmtx);

        // Sequentially perform tasks (for each child, in order)
        for (int c = 1; c <= KIDS; ++c) {
            printf("Child #%d is being fed breakfast.\n", c); usleep(100);
            printf("Child #%d is being sent to school.\n", c); usleep(100);
            printf("Child #%d is being given dinner.\n", c);   usleep(100);
            printf("Child #%d is being given a bath by Mother.\n", c); usleep(100);

            // Hand this child off to Father (eligible for read+tuck)
            qpush(c);
        }

        // Mother naps until Father finishes putting all kids to bed
        printf("Mother is going to sleep (nap) while Father reads and tucks kids.\n");
        sem_wait(&nextDayReady);  // Father signals when all 12 are in bed

        print_day_end(day);

        // If more days remain, allow next day to start
        if (day < Ncycles) {
            sem_post(&motherGo);  // Wake Mother for the *next* day
        }
    }
    return NULL;
}

// Father thread
static void *father_thread(void *arg) {
    // Father begins asleep; he will block on qItems until first bathed child arrives
    for (int day = 1; day <= Ncycles; ++day) {
        int tucked = 0;
        int first_wakeup_printed = 0;

        // Consume exactly 12 kids for this day
        while (tucked < KIDS) {
            int childId = qpop();  // blocks if no bathed child yet

            if (!first_wakeup_printed) {
                printf("Father is waking up Mother->Father handoff: Father is now taking care of the children.\n");
                first_wakeup_printed = 1;
            }

            printf("Child #%d is being read a book by Father.\n", childId); usleep(100);
            printf("Child #%d is being tucked in bed by Father.\n", childId);
            ++tucked;
        }

        // Day complete from Father's perspective
        printf("Father is going to sleep and waking up Mother to take care of the children.\n");

        // Hand control to Mother to begin the next day (or finish)
        sem_post(&nextDayReady);
        // Note: Mother will post motherGo if another day remains (see mother_thread).
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Ncycles>\nExample: %s 100\n", argv[0], argv[0]);
        return 1;
    }
    Ncycles = atoi(argv[1]);
    if (Ncycles <= 0) {
        fprintf(stderr, "Ncycles must be a positive integer.\n");
        return 1;
    }

    // Init semaphores
    // Queue semaphores: zero items; KIDS spaces
    sem_init(&qItems, 0, 0);
    sem_init(&qSpaces, 0, KIDS);

    // Day control: Mother starts awake (open = 1), Father finishes day and signals via nextDayReady (starts 0)
    sem_init(&motherGo, 0, 1);       // Mother init 1 (awake)  ← matches solution box
    sem_init(&nextDayReady, 0, 0);   // Father init 0 (sleep)  ← matches solution box
    // (The counting queue qItems is what actually wakes Father as soon as a bathed child appears.)

    pthread_t mom, dad;
    pthread_create(&mom, NULL, mother_thread, NULL);
    pthread_create(&dad, NULL, father_thread, NULL);

    pthread_join(mom, NULL);
    pthread_join(dad, NULL);

    // Cleanup
    sem_destroy(&qItems);
    sem_destroy(&qSpaces);
    sem_destroy(&motherGo);
    sem_destroy(&nextDayReady);

    return 0;
}

