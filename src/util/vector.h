
#ifndef QBN_VECTOR_H
#define QBN_VECTOR_H

#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "std.h"

#define UTIL_VECTOR_DEFAULT_CAPACITY 20

typedef struct {
    void** data;
    size_t element_size;
    size_t length;
    size_t capacity;
} UtilVector;

void util_vector_no_memory() {
    fprintf(stderr, "could not acquire memory");
    exit(1);
}

UtilVector* util_vector_new(size_t element_size, size_t capacity, void** data_pointer) {
    UtilVector* vec = (UtilVector*) malloc(sizeof(UtilVector));
    if (!vec) {
        util_vector_no_memory();
    }
    if (capacity == 0) {
        capacity = UTIL_VECTOR_DEFAULT_CAPACITY;
    }
    vec->data = data_pointer;
    *vec->data = malloc(element_size * capacity);
    if (!*vec->data) {
        util_vector_no_memory();
    }
    vec->element_size = element_size;
    vec->length = 0;
    vec->capacity = capacity;
    return vec;
}

void util_vector_grow(UtilVector* vec, size_t count) {
    if (vec->length + count > vec->capacity) {
        size_t new_capacity = vec->capacity + vec->capacity / 2;
        void* new_data = malloc(vec->element_size * new_capacity);
        if (!new_data) {
            util_vector_no_memory();
        }
        memcpy(new_data, *vec->data, vec->length * vec->element_size);
        free(*vec->data);
        *vec->data = new_data;
        vec->capacity = new_capacity;
    }
    vec->length += count;
}

void util_vector_shrink(UtilVector* vec, size_t count) {
    count = MIN(MAX(0, count), vec->length);
    vec->length -= count;
}

void util_vector_clear(UtilVector* vec) {
    vec->length = 0;
}

void util_vector_free(UtilVector* vec) {
    free(*vec->data);
    *vec->data = NULL;
    free(vec);
}

#endif //QBN_VECTOR_H
