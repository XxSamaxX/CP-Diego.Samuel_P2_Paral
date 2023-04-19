#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi/mpi.h>

int numprocs;

void inicializaCadena(char *cadena, int n){
    int i;
    for(i=0; i<n/2; i++){
        cadena[i] = 'A';
    }
    for(i=n/2; i<3*n/4; i++){
        cadena[i] = 'C';
    }
    for(i=3*n/4; i<9*n/10; i++){
        cadena[i] = 'G';
    }
    for(i=9*n/10; i<n; i++){
        cadena[i] = 'T';
    }
}

void MPI_BinomialBcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm){
    int rank, numprocs;
    MPI_Comm_size(comm, &numprocs);
    MPI_Comm_rank(comm, &rank);
    int k = ceil(log2(numprocs));

    for(int i = 0; i < k; i++){
        int partner = rank ^ (1 << i); // Calcula el partner para esta ronda
        if(rank < partner && partner < numprocs){
            MPI_Send(buffer, count, datatype, partner, 0, comm); // Envía a partner
        }
        else if(rank > partner){
            MPI_Recv(buffer, count, datatype, partner, 0, comm, MPI_STATUS_IGNORE); // Recibe de partner
        }
    }
}

void MPI_FlattreeColectiva(void *buffer, void * recvbuff , int count , MPI_Datatype datatype , MPI_Op op , int root , MPI_Comm comm){
    int elem = 0;
    int rank;
    MPI_Comm_rank(comm, &rank);
    if(rank == root){
        for(int i = 0;i < numprocs;i++){
            if(i != root){
                int buf;
                MPI_Recv(&buf, count, datatype, i, 0, comm, MPI_STATUS_IGNORE);
                printf("buffer: %d\n", buf);
                *(int *)recvbuff = *(int *)recvbuff + buf;
            }
        }
    }
    else{
        MPI_Send(buffer, count, datatype, root, 0, comm);
    }
}

int main(int argc, char *argv[]){
    // Inicializacion de MPI
    MPI_Init(&argc, &argv);
    if(argc != 3){
        printf("Numero incorrecto de parametros\nLa sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n  n es el tamaño de la cadena a generar\n  L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
        exit(1);
    }

    // Declaracion de variables MPI
    int rank, namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    int i, n, count=0, buffcount=0;
    char *cadena;
    char L;

    int num;
    char letra;

    // Obtenemos el numero de procesos, el rango del proceso y el nombre del procesador
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processor_name, &namelen);

    if(rank == 0) {
        L = *argv[2];
        n = atoi(argv[1]);
    }

    MPI_BinomialBcast(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD);//MPI_Bcast(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
    MPI_BinomialBcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);//MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    cadena = (char *) malloc(n*sizeof(char));
    inicializaCadena(cadena, n);

    for(i=rank*n/numprocs; i<(rank+1)*n/numprocs; i++){//otra opcion es i=rank; i<n; i+=numprocs
        if(cadena[i] == L){
            count++;
        }
    }

    MPI_FlattreeColectiva(&count, &buffcount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);//MPI_Reduce(&count, &buffcount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        printf("El numero de apariciones de la letra %c es %d\n", L, buffcount);
    }
    free(cadena);
    MPI_Finalize();
    exit(0);
}
