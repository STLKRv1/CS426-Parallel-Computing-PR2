#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include "utils.h"
#include <zconf.h>
#include <sys/time.h>
#include <time.h>

#define MASTER_TO_SLAVE_TAG 1   //tag for messages sent from master to slaves
#define MASTER_TO_SLAVE_TAG2 50   //tag for messages sent from master to slaves
#define SLAVE_TO_MASTER_TAG 100 //tag for messages sent from slaves to master


void kreduce(int * leastk, int * myids, int * myvals, int k, int world_size, int my_rank)
{
    int num_active_proc = world_size;
    int* other_vals = calloc(k, sizeof(int));
    int* other_ids = calloc(k, sizeof(int));
    //recursive doubling method until there is 4 processes active
    for (int i = 1; num_active_proc > 4; ++i)
    {
        //calculate the last consuming node
        int last_comp_node = num_active_proc % 2 == 0 ? (num_active_proc - 1) / 2: (num_active_proc ) / 2; //handle the case of odd number of processes
        //consuming node
        if ( (my_rank <= last_comp_node && num_active_proc % 2 == 0) ||
            (my_rank < last_comp_node && num_active_proc %2 == 1))
        {
            //Receive data from pair on the other half
            MPI_Recv(other_vals, k, MPI_INT, my_rank + last_comp_node + 1, SLAVE_TO_MASTER_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(other_ids, k, MPI_INT, my_rank + last_comp_node + 1, SLAVE_TO_MASTER_TAG + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int j = 0; j < k; ++j)
            {
                //get min k values
                if(myvals[k-1] > other_vals[j])
                {
                    myvals[k-1] = other_vals[j];
                    myids[k-1] = other_ids[j];
                    reorder_data(myvals, myids, k);
                }
            }
        }
        //Producer nodes
        else if ( my_rank > last_comp_node && my_rank < num_active_proc)//if there are odd number of nodes one of them should be idling since it has not pair to send/receive
        {                                   // O O O X | O O O "X" is idle until next iteration, "|" separates consuming nodes from producing nodes
            //send data to computing nodes
            MPI_Send( myvals, k, MPI_INT, my_rank - (last_comp_node + 1), SLAVE_TO_MASTER_TAG, MPI_COMM_WORLD );
            MPI_Send( myids, k, MPI_INT, my_rank - (last_comp_node + 1), SLAVE_TO_MASTER_TAG + 1, MPI_COMM_WORLD );
        }
        //Halve the remaining processes
        num_active_proc = num_active_proc % 2 == 0 ? (num_active_proc / 2) : (num_active_proc / 2) + 1;
    }

    //Final step: Master takes all data from all other active nodes
    if(my_rank == 0)//Master
    {
        //printf("active proc: %d \n", num_active_proc);
        for(int i = 1 ; i < num_active_proc; i++)
        {
            MPI_Recv(other_vals, k, MPI_INT, i, SLAVE_TO_MASTER_TAG + 5, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(other_ids, k, MPI_INT, i, SLAVE_TO_MASTER_TAG + 6, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //printf("[%d] final iteration recieved from proc: %d\n", my_rank, i );

            for (int j = 0; j < k; ++j)
            {
                //get min k values
                if(myvals[k-1] > other_vals[j])
                {
                    myvals[k-1] = other_vals[j];
                    myids[k-1] = other_ids[j];
                    reorder_data(myvals, myids, k);
                }
            }
        }
        for (int l = 0; l < k; ++l)
        {
            leastk[l] = myids[l];
            //printf("%d\n", leastk[l]);
        }
    }
    else if (my_rank < num_active_proc)//remaining nodes except master
    {
        //printf("[%d] arrived\n", my_rank);
        MPI_Send( myvals, k, MPI_INT, 0, SLAVE_TO_MASTER_TAG + 5, MPI_COMM_WORLD );
        MPI_Send( myids, k, MPI_INT, 0, SLAVE_TO_MASTER_TAG + 6, MPI_COMM_WORLD );
    }
    free(other_ids);
    free(other_vals);
}
int main(int argc, char* argv[])
{
    double t_serial, t_parallel, t_total, t_parallel_s, t_parallel_e, t_total_s, t_total_e;
    t_serial = t_parallel = t_total = 0.0;

    int k = 0, dic_size = 0;
    char* file_name;
    char* file_name2;

    if(argc>=1)
    {
        dic_size = atoi(argv[1]);
        k = atoi(argv[2]);
        file_name = argv[3];
        file_name2 = argv[4];
    }

    int rank = 0, num_proc = 1;
    MPI_Init(&argc,&argv);
    t_total_s = MPI_Wtime();

    MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int num_docs, docs_per_proc;
    int* my_vals = calloc(k, sizeof(int));
    int* my_ids = calloc(k, sizeof(int));
    document* docs;
    int* query;
    int* least_k;
    /*MPI_Datatype doc_type;
    defineMPIType( &doc_type );*/

    //Master
    if(rank == 0)
    {
        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC_RAW, &start);

        least_k = malloc(sizeof(int) * k);
        //read documents
        num_docs = read_documents(file_name, &docs, dic_size);
        //read query
        read_query(file_name2, dic_size, &query);
        clock_gettime(CLOCK_MONOTONIC_RAW, &end);
        t_serial = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

        t_parallel_s = MPI_Wtime();

        for (int i = 1; i < num_proc; i++)
        {
            docs_per_proc = (i == num_proc-1 ? num_docs - (i) * (num_docs / num_proc) : (num_docs / num_proc));
            MPI_Send( &docs_per_proc, 1, MPI_INT, i, MASTER_TO_SLAVE_TAG, MPI_COMM_WORLD );
            //distribute query
            MPI_Send( query, dic_size, MPI_INT, i, MASTER_TO_SLAVE_TAG + 1, MPI_COMM_WORLD );
        }
        docs_per_proc = num_docs/ num_proc;
        //distribute documents
        int dest;
        for (int i = num_docs/num_proc; i < num_docs; i++)
        {
            dest =  MIN(num_proc-1, i / (num_docs/ (num_proc)));
            //printf("i %d dest %d i - (dest-1) * (num_docs/ num_proc) %d\n",i, dest, i - (dest) * (num_docs/ num_proc));
            MPI_Send( &docs[i].doc_id, 1, MPI_INT, dest, MASTER_TO_SLAVE_TAG2 + i - (dest) * (num_docs/ num_proc) , MPI_COMM_WORLD );
            MPI_Send( docs[i].weights, dic_size, MPI_INT, dest, MASTER_TO_SLAVE_TAG2 + i - (dest) * (num_docs/ num_proc) + 1, MPI_COMM_WORLD );
        }
    }
    //Slaves
    else
    {
        //Receive data from master
        MPI_Recv(&docs_per_proc, 1, MPI_INT, 0, MASTER_TO_SLAVE_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        docs = malloc(sizeof(document) * docs_per_proc);
        query = malloc(dic_size * sizeof(int));
        MPI_Recv(query, dic_size, MPI_INT, 0, MASTER_TO_SLAVE_TAG + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        for (int i = 0; i < docs_per_proc; ++i)
        {
            MPI_Recv( &docs[i].doc_id, 1, MPI_INT, 0, MASTER_TO_SLAVE_TAG2 + i, MPI_COMM_WORLD, MPI_STATUS_IGNORE );
            docs[i].weights = malloc(sizeof(int) * dic_size);
            MPI_Recv( docs[i].weights, dic_size, MPI_INT, 0, MASTER_TO_SLAVE_TAG2 + i + 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE );

            //printf("id: %d : %d %d %d\n", docs[i].doc_id, docs[i].weights[0],  docs[i].weights[1], docs[i].weights[2]);
        }
    }


    for(int i = 0; i < k; i++)
    {
        my_vals[i] = INT_MAX;
        my_ids[i] = -1;
    }

    //Calculate Similarities
    for(int i = 0; i < docs_per_proc; i++)
    {
        int sim = calculate_similarity(docs[i], query, dic_size);

        //Insert if similarity is smaller than largest value
        if(sim < my_vals[k-1])
        {
            my_vals[k-1] = sim;
            my_ids[k-1] = docs[i].doc_id;
            //ensures the ascending order of my_val array
            reorder_data(my_vals, my_ids, k);
        }
    }

    kreduce(least_k, my_ids, my_vals, k, num_proc, rank);

    //Reductions required for accurate benchmarks
    t_parallel_e = MPI_Wtime();
    t_total_e = MPI_Wtime();
    double t0, t1, t2, t3;
    MPI_Reduce(&t_total_e, &t0, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&t_total_s, &t1, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);

    MPI_Reduce(&t_parallel_e, &t2, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    if(rank==0)//Master
    {
        t_total = (t0 - t1) * 1000;
        //t_parallel = (t2 - t_parallel_s) * 1000;
        t_parallel = t_total - t_serial;
        printf("Sequential Part: %f ms\n", t_serial);
        printf("Parallel Part: %f ms\n", t_parallel);
        printf("Total Time: %f ms\n", t_total);
        printf("Least k = %d ids:\n", k);
        for (int i = 0; i < k; ++i)
        {
            printf("%d\n", least_k[i]);
        }
        free(least_k);

        for (int i = 0; i < num_docs; ++i)
        {
            free(docs[i].weights);
        }
    }
    else
    {
        for (int i = 0; i < docs_per_proc; ++i)
        {
            free(docs[i].weights);
        }
    }
    free(my_ids);
    free(my_vals);
    free(query);
    free(docs);

    //MPI_Type_free( &doc_type );
    MPI_Finalize();
    return 0;
}