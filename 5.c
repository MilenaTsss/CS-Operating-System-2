//
// Created by milen on 23.04.2023.
//

#include "helper.h"

void destroy() {
    char* semaphore_name;
    for (int i = 0; gun_semaphores_pointer_first != NULL && i < field_size * field_size; ++i) {
        sem_destroy(gun_semaphores_pointer_first[i]);
        semaphore_name = get_semaphore_name(i, 1, gun_semaphore_name);
        if (sem_unlink(semaphore_name) == -1) {
            perror("sem_unlink");
            system_error("Error getting pointer to semaphore");
        }
        free(semaphore_name);
    }

    for (int i = 0; gun_semaphores_pointer_second != NULL && i < field_size * field_size; ++i) {
        sem_destroy(gun_semaphores_pointer_second[i]);
        semaphore_name = get_semaphore_name(i, 2, gun_semaphore_name);
        if (sem_unlink(semaphore_name) == -1) {
            perror("sem_unlink");
            system_error("Error getting pointer to semaphore");
        }
        free(semaphore_name);
    }

    printf("Gun semaphores are closed\n");

    for (int i = 0; manager_semaphores_pointer_first != NULL && i < field_size * field_size; ++i) {
        sem_destroy(manager_semaphores_pointer_first[i]);
        semaphore_name = get_semaphore_name(i, 1, manager_semaphore_name);
        if (sem_unlink(semaphore_name) == -1) {
            perror("sem_unlink");
            system_error("Error getting pointer to semaphore");
        }
        free(semaphore_name);
    }
    for (int i = 0; manager_semaphores_pointer_second != NULL && i < field_size * field_size; ++i) {
        sem_destroy(manager_semaphores_pointer_second[i]);
        semaphore_name = get_semaphore_name(i, 2, manager_semaphore_name);
        if (sem_unlink(semaphore_name) == -1) {
            perror("sem_unlink");
            system_error("Error getting pointer to semaphore");
        }
        free(semaphore_name);
    }

    printf("Manager semaphores are closed\n");


    if ((shmid = shm_open(memory_name, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        if (shm_unlink(memory_name) == -1) {
            perror("shm_unlink");
            system_error("Error getting pointer to shared memory");
        }
    }
    printf("Shared memory is unlinked\n");
}

void gun(int id, int num_of_field, sem_t *gun_semaphore, sem_t *manager_semaphore) {
    point_t *current_field = num_of_field == 1 ? first_field : second_field;
    point_t *another_field = num_of_field == 2 ? first_field : second_field;

    while (1) {
        sem_wait(gun_semaphore);
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
        sem_post(manager_semaphore);
    }
    printf("Point: %d from field %d is finished\n", id, num_of_field);
    close(shmid);
    sem_post(manager_semaphore);
}

void manager(sem_t **gun_semaphores_first, sem_t **gun_semaphores_second,
             sem_t **manager_semaphores_first, sem_t **manager_semaphores_second) {
    int status;
    while (1) {
        generate_targets(1);
        printf("Generated targets for first side\n");
        for (int i = 0; i < field_size * field_size; ++i) {
            if (first_field[i].type != EMPTY_POINT && first_field[i].type != USED_POINT) {
                sem_post(gun_semaphores_first[i]);
            }
        }

        for (int i = 0; i < field_size * field_size; ++i) {
            if (first_field[i].type != EMPTY_POINT && first_field[i].type != USED_POINT) {
                sem_wait(manager_semaphores_first[i]);
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
                sem_post(gun_semaphores_second[i]);
            }
        }

        for (int i = 0; i < field_size * field_size; ++i) {
            if (second_field[i].type != EMPTY_POINT && second_field[i].type != USED_POINT) {
                sem_wait(manager_semaphores_second[i]);
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
            sem_post(gun_semaphores_first[i]);
        }
        if (second_field[i].type == ALIVE_GUN || second_field[i].type == DEAD_GUN) {
            second_field[i].type = FINISH;
            sem_post(gun_semaphores_second[i]);
        }
    }

    for (int i = 0; i < field_size * field_size; ++i) {
        if (first_field[i].type == FINISH) {
            sem_wait(manager_semaphores_first[i]);
        }
        if (second_field[i].type == FINISH) {
            sem_wait(manager_semaphores_second[i]);
        }
    }
}

void handler(int signal) {
    printf("Received signal %d\n", signal);
    destroy();
    exit(0);
}

int main(int argc, char **argv) {
    srand(time(NULL));// NOLINT(cert-msc51-cpp)
    if (argc != 3) {
        printf("%s", get_help_message());
    }
    prev = signal(SIGINT, handler);

    // get field_size and num_of_guns from args
    field_size = (int) strtol(argv[1], NULL, 10);
    num_of_guns = (int) strtol(argv[2], NULL, 10);

    printf("Field size = %d\nNumber of guns = %d\n", field_size, num_of_guns);

    // open memory
    if ((shmid = shm_open(memory_name, O_CREAT | O_RDWR, S_IRWXU)) == -1) {
        perror("shm_open");
        system_error("Memory opening error");
    } else {
        printf("Memory opened: name = %s, id = 0x%x\n", memory_name, shmid);
    }

    // set memory size
    if (ftruncate(shmid, (int) sizeof(point_t) * field_size * field_size * 2) == -1) {
        perror("ftruncate");
        system_error("Setting memory size error");
    } else {
        printf("Memory size set - %d\n", (int) sizeof(point_t) * field_size * field_size * 2);
    }

    first_field = mmap(0, sizeof(point_t) * field_size * field_size * 2,
                       PROT_WRITE | PROT_READ, MAP_SHARED, shmid, 0);
    second_field = first_field + field_size * field_size;

    fill_field(first_field);
    fill_field(second_field);

    sem_t *gun_semaphores_first[field_size * field_size];
    sem_t *gun_semaphores_second[field_size * field_size];
    for (int i = 0; i < field_size * field_size; ++i) {
        char* semaphore_name = get_semaphore_name(i, 1, gun_semaphore_name);
        gun_semaphores_first[i] = sem_open(semaphore_name, O_CREAT, 0666, 0);
        free(semaphore_name);

        semaphore_name = get_semaphore_name(i, 2, gun_semaphore_name);
        gun_semaphores_second[i] = sem_open(semaphore_name, O_CREAT, 0666, 0);
        free(semaphore_name);
    }
    gun_semaphores_pointer_first = gun_semaphores_first;
    gun_semaphores_pointer_second = gun_semaphores_second;

    sem_t *manager_semaphores_first[field_size * field_size];
    sem_t *manager_semaphores_second[field_size * field_size];
    for (int i = 0; i < field_size * field_size; ++i) {
        char* semaphore_name = get_semaphore_name(i, 1, manager_semaphore_name);
        manager_semaphores_first[i] = sem_open(semaphore_name, O_CREAT, 0666, 0);
        free(semaphore_name);

        semaphore_name = get_semaphore_name(i, 2, manager_semaphore_name);
        manager_semaphores_second[i] = sem_open(semaphore_name, O_CREAT, 0666, 0);
        free(semaphore_name);
    }

    manager_semaphores_pointer_first = manager_semaphores_first;
    manager_semaphores_pointer_second = manager_semaphores_second;
    printf("All semaphores are inited\n\n");

    // creating all guns
    for (int i = 0; i < field_size * field_size; ++i) {
        if (first_field[i].type == ALIVE_GUN) {
            if (fork() == 0) {
                signal(SIGINT, prev);
                printf("Gun from first side in coordinate: %d\n", i);
                gun(i, 1, gun_semaphores_first[i], manager_semaphores_first[i]);
                exit(0);
            }
        }
        if (second_field[i].type == ALIVE_GUN) {
            if (fork() == 0) {
                signal(SIGINT, prev);
                printf("Gun from second side in coordinate: %d\n", i);
                gun(i, 2, gun_semaphores_second[i], manager_semaphores_second[i]);
                exit(0);
            }
        }
    }

    manager(gun_semaphores_first, gun_semaphores_second, manager_semaphores_first, manager_semaphores_second);
    destroy();
    return 0;
}