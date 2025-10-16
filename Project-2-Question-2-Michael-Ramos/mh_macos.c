// mh_macos.c — Mother Hubbard (Problem 2) — macOS variant using *named* POSIX semaphores
// Build: gcc -Wall -Wextra -pthread -o mh_macos mh_macos.c
// Run:   ./mh_macos 1
// This version replaces sem_init/sem_destroy with sem_open/sem_close/sem_unlink
// so it runs natively on macOS (which lacks unnamed semaphores).

#define _XOPEN_SOURCE 700
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>

#define KIDS 12

// Named semaphore handles
static sem_t *qItems = NULL;
static sem_t *qSpaces = NULL;
static sem_t *motherGo = NULL;
static sem_t *nextDayReady = NULL;

// Names (unique per process using pid)
static char name_qItems[64];
static char name_qSpaces[64];
static char name_motherGo[64];
static char name_nextDayReady[64];

static int qbuf[KIDS];
static int qhead = 0, qtail = 0;
static pthread_mutex_t qmtx = PTHREAD_MUTEX_INITIALIZER;

static int Ncycles = 1;

// helpers for named sems
static sem_t* must_sem_open(const char* name, unsigned int value) {
    sem_t *s = sem_open(name, O_CREAT | O_EXCL, 0600, value);
    if (s == SEM_FAILED) {
        perror("sem_open");
        fprintf(stderr, "Failed creating semaphore '%s'\n", name);
        exit(1);
    }
    return s;
}
static void must_sem_unlink(const char* name) { sem_unlink(name); }

static void qpush(int childId) {
    sem_wait(qSpaces);
    pthread_mutex_lock(&qmtx);
    qbuf[qtail] = childId;
    qtail = (qtail + 1) % KIDS;
    pthread_mutex_unlock(&qmtx);
    sem_post(qItems);
}
static int qpop(void) {
    sem_wait(qItems);
    pthread_mutex_lock(&qmtx);
    int id = qbuf[qhead];
    qhead = (qhead + 1) % KIDS;
    pthread_mutex_unlock(&qmtx);
    sem_post(qSpaces);
    return id;
}
static void print_day_begin(int day) { printf("This is day #%d of a day in the life of Mother Hubbard.\n", day); }
static void print_day_end(int day)   { printf("End of day #%d in the life of Mother Hubbard.\n", day); }

static void *mother_thread(void *arg) {
    for (int day = 1; day <= Ncycles; ++day) {
        sem_wait(motherGo);
        print_day_begin(day);
        printf("Mother is waking up to take care of the children.\n");
        pthread_mutex_lock(&qmtx); qhead = qtail = 0; pthread_mutex_unlock(&qmtx);
        for (int c = 1; c <= KIDS; ++c) {
            printf("Child #%d is being fed breakfast.\n", c); usleep(100);
            printf("Child #%d is being sent to school.\n", c); usleep(100);
            printf("Child #%d is being given dinner.\n", c);   usleep(100);
            printf("Child #%d is being given a bath by Mother.\n", c); usleep(100);
            qpush(c);
        }
        printf("Mother is going to sleep (nap) while Father reads and tucks kids.\n");
        sem_wait(nextDayReady);
        print_day_end(day);
        if (day < Ncycles) sem_post(motherGo);
    }
    return NULL;
}
static void *father_thread(void *arg) {
    for (int day = 1; day <= Ncycles; ++day) {
        int tucked = 0, first = 1;
        while (tucked < KIDS) {
            int childId = qpop();
            if (first) { printf("Father is waking up Mother->Father handoff: Father is now taking care of the children.\n"); first = 0; }
            printf("Child #%d is being read a book by Father.\n", childId); usleep(100);
            printf("Child #%d is being tucked in bed by Father.\n", childId);
            ++tucked;
        }
        printf("Father is going to sleep and waking up Mother to take care of the children.\n");
        sem_post(nextDayReady);
    }
    return NULL;
}
int main(int argc, char **argv) {
    if (argc != 2) { fprintf(stderr, "Usage: %s <Ncycles>\n", argv[0]); return 1; }
    Ncycles = atoi(argv[1]); if (Ncycles <= 0) { fprintf(stderr, "Ncycles must be a positive integer.\n"); return 1; }

    pid_t pid = getpid();
    snprintf(name_qItems, sizeof(name_qItems), "/mh_qItems_%d", pid);
    snprintf(name_qSpaces, sizeof(name_qSpaces), "/mh_qSpaces_%d", pid);
    snprintf(name_motherGo, sizeof(name_motherGo), "/mh_motherGo_%d", pid);
    snprintf(name_nextDayReady, sizeof(name_nextDayReady), "/mh_nextDayReady_%d", pid);
    sem_unlink(name_qItems); sem_unlink(name_qSpaces); sem_unlink(name_motherGo); sem_unlink(name_nextDayReady);

    qItems = must_sem_open(name_qItems, 0);
    qSpaces = must_sem_open(name_qSpaces, KIDS);
    motherGo = must_sem_open(name_motherGo, 1);
    nextDayReady = must_sem_open(name_nextDayReady, 0);

    pthread_t mom, dad;
    if (pthread_create(&mom, NULL, mother_thread, NULL) != 0) { perror("pthread_create mother"); return 1; }
    if (pthread_create(&dad, NULL, father_thread, NULL) != 0) { perror("pthread_create father"); return 1; }
    pthread_join(mom, NULL); pthread_join(dad, NULL);

    sem_close(qItems); must_sem_unlink(name_qItems);
    sem_close(qSpaces); must_sem_unlink(name_qSpaces);
    sem_close(motherGo); must_sem_unlink(name_motherGo);
    sem_close(nextDayReady); must_sem_unlink(name_nextDayReady);
    return 0;
}
