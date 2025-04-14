#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_DIRECTIONS 4

pthread_mutex_t intersection_lock;
pthread_mutex_t lane_locks[NUM_DIRECTIONS];
pthread_mutex_t status_lock;

const char *direction_names[] = {"North", "South", "East", "West"};

typedef struct {
    int id;
    int direction;
    int priority;
    int has_crossed;
} Car;

Car *cars[NUM_DIRECTIONS];
int lane_status[NUM_DIRECTIONS] = {0};
int deadlock_flag = 0;

void *car_thread(void *arg) {
    Car *car = (Car *)arg;
    int dir = car->direction;

    printf("🚗 Car %d from %s manually instructed to move.\n", car->id, direction_names[dir]);

    pthread_mutex_lock(&lane_locks[dir]);
    pthread_mutex_lock(&status_lock);
    lane_status[dir] = 1;
    pthread_mutex_unlock(&status_lock);

    if (pthread_mutex_trylock(&intersection_lock) == 0) {
        printf("🚦 Car %d ENTERED intersection from %s\n", car->id, direction_names[dir]);
        sleep(2);
        pthread_mutex_unlock(&intersection_lock);
        printf("✅ Car %d EXITED from %s\n", car->id, direction_names[dir]);
        car->has_crossed = 1;
    } else {
        printf("❌ Car %d BLOCKED at %s\n", car->id, direction_names[dir]);
        sleep(2);
    }

    pthread_mutex_unlock(&lane_locks[dir]);

    pthread_mutex_lock(&status_lock);
    lane_status[dir] = 0;
    pthread_mutex_unlock(&status_lock);

    pthread_exit(NULL);
}

void *deadlock_detector(void *arg) {
    sleep(3);

    pthread_mutex_lock(&status_lock);
    int stuck = 1;
    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        if (lane_status[i] == 0) {
            stuck = 0;
            break;
        }
    }

    if (stuck) {
        printf("\n🛑 DEADLOCK DETECTED!\n");
        deadlock_flag = 1;

        Car *priority_car = NULL;
        for (int i = 0; i < NUM_DIRECTIONS; i++) {
            if (!cars[i]->has_crossed) {
                if (priority_car == NULL || cars[i]->priority < priority_car->priority) {
                    priority_car = cars[i];
                }
            }
        }

        if (priority_car) {
            printf("🚓 Priority Car %d from %s is FORCING ENTRY\n", priority_car->id, direction_names[priority_car->direction]);

            pthread_mutex_lock(&intersection_lock);
            sleep(2);
            pthread_mutex_unlock(&intersection_lock);

            priority_car->has_crossed = 1;

            lane_status[priority_car->direction] = 0;

            printf("✅ Priority Car %d EXITED. Deadlock RESOLVED.\n", priority_car->id);
        }
    } else {
        printf("\n✅ No Deadlock Detected.\n");
    }

    pthread_mutex_unlock(&status_lock);
    pthread_exit(NULL);
}

int main() {
    pthread_t detector;
    pthread_mutex_init(&intersection_lock, NULL);
    pthread_mutex_init(&status_lock, NULL);
    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        pthread_mutex_init(&lane_locks[i], NULL);
    }

    for (int i = 0; i < NUM_DIRECTIONS; i++) {
        cars[i] = (Car *)malloc(sizeof(Car));
        cars[i]->id = i + 1;
        cars[i]->direction = i;
        cars[i]->priority = i;
        cars[i]->has_crossed = 0;
    }

    pthread_create(&detector, NULL, deadlock_detector, NULL);

    while (1) {
        printf("\nManual Control:\n");
        for (int i = 0; i < NUM_DIRECTIONS; i++) {
            printf("%d - Move car from %s\n", i + 1, direction_names[i]);
        }
        printf("5 - Exit program\n");
        printf("Enter your choice: ");

        int choice;
        scanf("%d", &choice);

        if (choice >= 1 && choice <= 4) {
            pthread_t tid;
            pthread_create(&tid, NULL, car_thread, (void *)cars[choice - 1]);
            pthread_join(tid, NULL);
        } else if (choice == 5) {
            break;
        } else {
            printf("❌ Invalid input. Try again.\n");
        }
    }

    pthread_join(detector, NULL);
    printf("\n📝 Simulation ended.\n");

    return 0;
}
