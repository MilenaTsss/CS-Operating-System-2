//
// Created by Milena Tsarakhova on 23.04.2023.
//

#ifndef UNTITLED2_HELPER_H
#define UNTITLED2_HELPER_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/types.h>
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
    sem_t child_sem;
    sem_t parent_sem;
} point_t;

point_t *first_field = NULL; // address of the first field in shared memory
point_t *second_field = NULL;

int shmid = -1;
void (*prev)(int);

char *get_help_message() {
    return "Incorrect command line arguments!\nPlease enter size of field and number of guns";
}

char *get_point_type(int type) {
    switch (type) {
        case 0: return "empty";
        case 1: return "used";
        case 2: return "alive gun";
        default: return "dead gun";
    }
}

char get_point_char(int type) {
    switch (type) {
        case 0: return '*';
        case 1: return 'x';
        case 2: return 'G';
        default: return 'D';
    }
}

#endif//UNTITLED2_HELPER_H
