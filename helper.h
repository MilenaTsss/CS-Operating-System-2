//
// Created by Milena Tsarakhova on 23.04.2023.
//

#ifndef UNTITLED2_HELPER_H
#define UNTITLED2_HELPER_H

#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void system_error(char *msg) {
    puts(msg);
    exit(1);
}

char *memory_name = "shared memory";
int num_of_guns;
int field_size;

#define EMPTY_POINT 0
#define USED_POINT 1
#define ALIVE_GUN 2
#define DEAD_GUN 3
#define FINISH 4


typedef struct {
    int type;
    int target_coordinate;
    sem_t gun_semaphore;
    sem_t manager_semaphore;
} point_t;

point_t *first_field = NULL;// address of the first field in shared memory
point_t *second_field = NULL;

int shmid = -1;
void (*prev)(int);

char *get_help_message() {
    return "Incorrect command line arguments!\nPlease enter size of field and number of guns";
}

char *get_point_type(int type) {
    switch (type) {
        case 0:
            return "empty";
        case 1:
            return "used";
        case 2:
            return "alive gun";
        default:
            return "dead gun";
    }
}

char get_point_char(int type) {
    switch (type) {
        case 0:
            return '*';
        case 1:
            return 'x';
        case 2:
            return 'G';
        default:
            return 'D';
    }
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
            int num = (int) rand() % (field_size * field_size);// NOLINT(cert-msc50-cpp)
            while (another_point[num].type == USED_POINT || another_point[num].type == DEAD_GUN) {
                num = rand() % (field_size * field_size);// NOLINT(cert-msc50-cpp)
            }
            point[i].target_coordinate = num;
        }
    }
}

void fill_field(point_t *field) {
    for (int i = 0; i < field_size * field_size; ++i) {
        field[i].type = EMPTY_POINT;
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

const char *gun_semaphore_name = "/gun-semaphore";
const char *manager_semaphore_name = "/manager-semaphore";

sem_t **gun_semaphores_pointer_first = NULL;
sem_t **gun_semaphores_pointer_second = NULL;
sem_t **manager_semaphores_pointer_first = NULL;
sem_t **manager_semaphores_pointer_second = NULL;


char* get_semaphore_name(int index, int type_of_field, const char *semaphore_prefix) {
    char num[20];
    num[0] = '0' + type_of_field;
    num[1] = '\0';
    sprintf(num + 1, "%d", index);
    char* name = malloc(strlen(semaphore_prefix) + strlen(num));
    name[0] = '\0';
    strcat(name, semaphore_prefix);
    strcat(name, num);
    return name;
}

#endif//UNTITLED2_HELPER_H
