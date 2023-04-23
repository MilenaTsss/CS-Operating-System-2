#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

#include "helper.h"

void destroy() {
    // destroying semaphores
    for (int i = 0; first_field != NULL && second_field != NULL && i < field_size * field_size; ++i) {
        if (first_field[i].type == ALIVE_GUN || first_field[i].type == DEAD_GUN || first_field[i].type == FINISH) {
            sem_destroy(&first_field[i].child_sem);
            sem_destroy(&first_field[i].parent_sem);
        }

        if (second_field[i].type == ALIVE_GUN || second_field[i].type == DEAD_GUN || second_field[i].type == FINISH) {
            sem_destroy(&second_field[i].child_sem);
            sem_destroy(&second_field[i].parent_sem);
        }
    }

    printf("Closed all semaphores\n");

    if ((shmid = shm_open(memory_name, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        if (shm_unlink(memory_name) == -1) {
            perror("shm_unlink");
            system_error("Error getting pointer to shared memory");
        }
    }
    printf("Shared memory is unlinked\n");
}

void print_field(int type_of_field) {
    point_t *field = type_of_field == 1 ? first_field : second_field;
    printf("Side number %d", type_of_field);
    for (int i = 0; i < field_size * field_size; ++i) {
        if (i % field_size == 0) {
            printf("\n");
        }
        printf("%c", get_point_char(field[i].type));
    }
    printf("\n");
}

void gun(int id, int num_of_field) {
    point_t *current_field = num_of_field == 1 ? first_field : second_field;
    point_t *another_field = num_of_field == 2 ? first_field : second_field;

    while (1) {
        sem_wait(&current_field[id].child_sem);
        if (current_field[id].type == FINISH) {
            break;
        }
        if (current_field[id].target_coordinate != -1) {
            printf("Making shot from %d: into %d by %d side\n",
                   id, current_field[id].target_coordinate, num_of_field);
            if (another_field[current_field[id].target_coordinate].type == ALIVE_GUN) {
                another_field[current_field[id].target_coordinate].type = DEAD_GUN;
            }
            if (another_field[current_field[id].target_coordinate].type == EMPTY_POINT) {
                another_field[current_field[id].target_coordinate].type = USED_POINT;
            }
            sleep(current_field[id].target_coordinate / 30);
        }
        sem_post(&current_field[id].parent_sem);
    }
    printf("Point: %d is dead\n", id);
    close(shmid);
    sem_post(&current_field[id].parent_sem);
}

int check_status() {
    int result = 0;
    for (int i = 0; i < field_size * field_size; ++i) {
        if (first_field[i].type == ALIVE_GUN) {
            result++;
        }
    }
    if (!result) {
        return 0;
    }

    result = 0;
    for (int i = 0; i < field_size * field_size; ++i) {
        if (second_field[i].type == ALIVE_GUN) {
            result++;
        }
    }
    return result == 0 ? 0 : 1;
}

void generate_targets(int type_of_field) {
    point_t *point = type_of_field == 1 ? first_field : second_field;
    point_t *another_point = type_of_field == 2 ? first_field : second_field;

    for (int i = 0; i < field_size * field_size; ++i) {
        if (point[i].type == ALIVE_GUN) {
            int num = (int)rand() % (field_size * field_size);// NOLINT(cert-msc50-cpp)
            while (another_point[num].type == USED_POINT || another_point[num].type == DEAD_GUN) {
                num = rand() % (field_size * field_size);// NOLINT(cert-msc50-cpp)
            }
            point[i].target_coordinate = num;
        }
    }
}

void manager() {
    int status;
    while (1) {
        generate_targets(1);
        printf("Generated targets for first side\n");
        for (int i = 0; i < field_size * field_size; ++i) {
            if (first_field[i].type != EMPTY_POINT && first_field[i].type != USED_POINT) {
                sem_post(&first_field[i].child_sem);
            }
        }

        for (int i = 0; i < field_size * field_size; ++i) {
            if (first_field[i].type != EMPTY_POINT && first_field[i].type != USED_POINT) {
                sem_wait(&first_field[i].parent_sem);
                printf("Shot made by first side from %d into %d. Now this point is %s\n",
                       i, first_field[i].target_coordinate,
                       get_point_type(second_field[first_field[i].target_coordinate].type));
            }
        }

        status = check_status();
        if (status == 0) {
            printf("FIRST SIDE IS WINNER\n");
            break;
        }

        printf("\n\n");

        generate_targets(2);
        printf("Generated targets for second side\n");
        for (int i = 0; i < field_size * field_size; ++i) {
            if (second_field[i].type != EMPTY_POINT && second_field[i].type != USED_POINT) {
                sem_post(&second_field[i].child_sem);
            }
        }

        for (int i = 0; i < field_size * field_size; ++i) {
            if (second_field[i].type != EMPTY_POINT && second_field[i].type != USED_POINT) {
                sem_wait(&second_field[i].parent_sem);
                printf("Shot made by second side from %d into %d. Now this point type is %s\n",
                       i, second_field[i].target_coordinate,
                       get_point_type(first_field[second_field[i].target_coordinate].type));
            }
        }

        status = check_status();
        if (status == 0) {
            printf("SECOND SIDE IS WINNER\n");
            break;
        }
        printf("\n\n");
    }

    print_field(1);
    print_field(2);

    for (int i = 0; i < field_size * field_size; ++i) {
        if (first_field[i].type == ALIVE_GUN || first_field[i].type == DEAD_GUN) {
            first_field[i].type = FINISH;
            sem_post(&first_field[i].child_sem);
        }
        if (second_field[i].type == ALIVE_GUN || second_field[i].type == DEAD_GUN) {
            second_field[i].type = FINISH;
            sem_post(&second_field[i].child_sem);
        }
    }

    for (int i = 0; i < field_size * field_size; ++i) {
        if (first_field[i].type == FINISH) {
            sem_wait(&first_field[i].parent_sem);
        }
        if (second_field[i].type == FINISH) {
            sem_wait(&second_field[i].parent_sem);
        }
    }
}

void handler(int signal){
    printf("Received signal %d\n", signal);
    destroy();
    exit(0);
}

void fill_field(point_t *field) {
    for (int i = 0; i < field_size * field_size; ++i) {
        field[i].type = 0;
        field[i].target_coordinate = -1;
    }

    for (int i = 0; i < num_of_guns; ++i) {
        long num = rand() % (field_size * field_size);// NOLINT(cert-msc50-cpp)
        while (field[num].type != EMPTY_POINT) {
            num = rand() % (field_size * field_size);// NOLINT(cert-msc50-cpp)
        }
        field[num].type = ALIVE_GUN;
    }
}

// first arg = size of field, second arg = number of guns
int main(int argc, char **argv) {
    srand(time(NULL)); // NOLINT(cert-msc51-cpp)
    if (argc != 3) {
        printf("%s", get_help_message());
    }
    prev = signal(SIGINT, handler);

    // get field_size and num_of_guns from args
    field_size = (int)strtol(argv[1], NULL, 10);
    num_of_guns = (int)strtol(argv[2], NULL, 10);

    printf("Field size = %d\nNumber of guns = %d\n", field_size, num_of_guns);

    // open memory
    if ((shmid = shm_open(memory_name, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        perror("shm_open");
        system_error("Memory opening error");
    } else {
        printf("Memory opened: name = %s, id = 0x%x\n", memory_name, shmid);
    }

    // set memory size
    if (ftruncate(shmid, (int)sizeof(point_t) * field_size * field_size * 2) == -1) {
        perror("ftruncate");
        system_error("Setting memory size error");
    } else {
        printf("Memory size set - %d\n", (int)sizeof(point_t) * field_size * field_size * 2);
    }

    first_field = mmap(0, sizeof(point_t) * field_size * field_size * 2,
                       PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0);
    second_field = first_field + field_size * field_size;

    fill_field(first_field);
    fill_field(second_field);

    // init all semaphores for first and second field
    for (int i = 0; i < field_size * field_size; ++i) {
        if (first_field[i].type == ALIVE_GUN) {
            if (sem_init(&first_field[i].child_sem, 1, 0) == -1) {
                system_error("Creating gun semaphore for first went wrong");
            }
            if (sem_init(&first_field[i].parent_sem, 1, 0) == -1) {
                system_error("Creating gun semaphore for first went wrong");
            }
        }

        if (second_field[i].type == ALIVE_GUN) {
            if (sem_init(&second_field[i].child_sem, 1, 0) == -1) {
                system_error("Creating gun semaphore for second went wrong");
            }
            if (sem_init(&second_field[i].parent_sem, 1, 0) == -1) {
                system_error("Creating gun semaphore for second went wrong");
            }
        }
    }
    printf("All semaphores are inited\n\n");

    // creating all guns
    for (int i = 0; i < field_size * field_size; ++i) {
        if (first_field[i].type == ALIVE_GUN) {
            if (fork() == 0) {
                signal(SIGINT, prev);
                printf("Gun from first side in coordinate: %d\n", i);
                gun(i, 1);
                exit(0);
            }
        }
        if (second_field[i].type == ALIVE_GUN) {
            if (fork() == 0) {
                signal(SIGINT, prev);
                printf("Gun from second side in coordinate: %d\n", i);
                gun(i, 2);
                exit(0);
            }
        }
    }

    manager();
    destroy();
    return 0;
}