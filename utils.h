#ifndef CS426_PR2_UTILS_H
#define CS426_PR2_UTILS_H

#include <bits/types/FILE.h>
#include <stdio.h>
#include <malloc.h>
#include <mpi.h>
#include <math.h>
#include <memory.h>
#include <stdlib.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

typedef struct document
{
    int doc_id;
    int* weights;
}document;

typedef struct dynamic_array_struct
{
    int* data;
    size_t capacity; /* total capacity */
    int size; /* number of elements in vector */
} vector;

int vector_init(vector* v, size_t init_capacity);

int vector_free(vector* v);

int vector_push_back(vector* v, int elem);

int read_documents(char* fileName, document** docs, int dic_size);

int read_query(char* fileName, int dic_size, int** query);

//void defineMPIType( MPI_Datatype* newType );//Source:https://stackoverflow.com/questions/32928523/create-mpi-type-for-struct-containing-dynamic-array

int calculate_similarity(document doc, int* query, int dic_size);

void reorder_data(int* my_vals, int* my_ids, int k);

#endif //CS426_PR2_UTILS_H
