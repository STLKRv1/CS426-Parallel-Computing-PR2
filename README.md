# -CS426-Parallel-Computing-PR2
Parallel Open-MPI implementation of simplified Supervised Search

To compile: "mpicc –o documentSearch main.c utils.h utils.c –lm" to linux terminal.

To execute: mpirun –n X ./documentSearch dictionarySize kValue documents.txt query.txt

X is number of threads

dictionarySize is int that denotes the number of weigths for each row

kValues is the number of desired outputs that has the least k relation to query.

documents.txt is the path to documents that are in the format of "id: w1 w2 w3 ... wdictionarySize"

query.txt is the path to the query that is in the format of "v1 v2 v3 ... vdictonarySize"
