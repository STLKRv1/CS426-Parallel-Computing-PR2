#include "utils.h"


int vector_init(vector* v, size_t init_capacity)
{
    v->data = malloc(init_capacity * sizeof(int));
    if (!v->data) return -1;

    v->size = 0;
    v->capacity = init_capacity;

    return 0;
}

int vector_free(vector* v)
{
    free(v->data);
    //free(v);

    return 0;
}
int vector_push_back(vector* v, int elem)
{
    if (v->size +1 >= v->capacity)
    {
        int * newdata = malloc(sizeof(int) * (v->capacity * 2 ));
        v->capacity *= 2;
        for(int i =0; i < v->size; i++)
        {
            newdata[i] = v->data[i];
        }
        free(v->data);
        v->data = newdata;
        newdata = NULL;
    }

    v->data[v->size] = elem;
    //printf("pushed: %d \n", v->data[v->size]);
    v->size = v->size+1;
    //printf("size: %d, capacity: %d \n", v->size, v->capacity);

    return 0;
}

int read_documents(char* fileName, document** docs, int dic_size)
{
    int num, pos = 0, num_lines = 0, i=0 ;
    char line[400];
    char* sub_str;
    size_t len = 0;
    document* rtn;

    FILE* file = fopen(fileName, "r");
    document doc;

    //count the number of lines
    while ((fgets(line, 400, file)) != NULL)
    {
        num_lines++;
    }
    //printf("num lines: %d \n", num_lines);
    rtn = malloc(sizeof(document) * num_lines);
    fclose(file);

    //Read documents and store them in a struct
    file = fopen(fileName, "r");
    int row,col,c,count,inc;
    char ch;
    vector vec;

    while(EOF!=(inc=fscanf(file,"%d%c", &num, &ch)) && inc == 2){

        if (ch == '\n')
        {
            vector_push_back(&vec, num);
            //printf("%d ", vec.data[col++]);

            doc.weights = vec.data;
            rtn[i++] = doc;
            //printf("\n");
        }
        else if (ch == ':')
        {
            doc.doc_id = num;
            vector_init(&vec, dic_size);
            //printf("%d:", doc.doc_id);
            col = 0;
        }
        else if(ch == ' ')
        {
            vector_push_back(&vec, num);
            //printf("%d ", vec.data[col++]);
        }
    }

    *docs = rtn;
    fclose(file);
    return num_lines;
}

int read_query(char* file_name, int dic_size, int** query)
{
    int num, i = 0;
    int* rtn = malloc(sizeof(int) * dic_size);
    FILE* file = fopen(file_name, "r");
    while ( fscanf (file, "%d", &num) == 1 && i < dic_size)
    {
        rtn[i++] = num;
        //printf("%d ", rtn[i-1]);
    }
    fclose(file);

    *query = rtn;

    return 0;
}

/*void defineMPIType( MPI_Datatype* newType ) {
    struct document tmp[2];
    MPI_Aint extent = &tmp[1] - &tmp[0];

    MPI_Type_create_resized( MPI_2INT, 0, extent, newType );
    MPI_Type_commit( newType );
}*/

int calculate_similarity(document doc, int* query, int dic_size)
{
    int sim = 0;
    for(int i = 0; i < dic_size; i++)
    {
        sim += (int)pow((double)doc.weights[i], (double)query[i]);
        //printf("id %d weight: %d\n",doc.doc_id, doc.weights[i]);
    }
    return sim;
}

void reorder_data(int* my_vals, int* my_ids, int k)
{
    for(int i = k-1; i > 0; i--)
    {
        if( my_vals[i] < my_vals[i-1])
        {
            //swap ids and values
            int tmp = my_vals[i-1];
            my_vals[i-1] = my_vals[i];
            my_vals[i] = tmp;

            tmp = my_ids[i-1];
            my_ids[i-1] = my_ids[i];
            my_ids[i] = tmp;
            i++;
        }
    }
}