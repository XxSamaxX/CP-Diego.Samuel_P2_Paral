#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi/mpi.h>



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

void MPI_BinomialBcast(void *buffer, int count, MPI_Datatype datatype, MPI_Comm comm){
    int rank, numprocs;
    MPI_Comm_size(comm, &numprocs);
    MPI_Comm_rank(comm, &rank);
    int k = ceil(log2(numprocs));// Calcula el número de rondas totales

    for(int i = 0; i < k; i++){
        int partner = rank ^ (1 << i); // Calcula el partner para esta ronda
        if(rank < partner && partner < numprocs){//si el partner es menor que el numero de procesos y mayor que el rank
            MPI_Send(buffer, count, datatype, partner, 0, comm); // Envía a partner
        }
        else if(rank > partner){//si el rank es mayor que el partner
            MPI_Recv(buffer, count, datatype, partner, 0, comm, MPI_STATUS_IGNORE); // Recibe de partner
        }
    }
}

void MPI_FlattreeColectiva(void *buffer, void * recvbuff , int count , MPI_Datatype datatype , MPI_Op op , int root , MPI_Comm comm){
    int rank, numprocs;
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &numprocs);
    if(rank == root){//si es el root recibe de todos los procesos
        for(int i = 0;i < numprocs;i++){//para cada proceso
            if(i != root){
                MPI_Recv(buffer, count, datatype, i, 0, comm, MPI_STATUS_IGNORE);
            }
            *(int *)recvbuff = *(int *)recvbuff + *(int *)buffer;//suma los valores recibidos
        }
    }
    else{//si no es el root envia al root
        MPI_Send(buffer, count, datatype, root, 0, comm);
    }
}

int main(int argc, char *argv[]){
    // Inicializacion de MPI
    MPI_Init(&argc, &argv);
    if(argc != 3){
        printf("Numero incorrecto de parametros\n"
               "La sintaxis debe ser: program n L\n  program es el nombre del ejecutable\n"
               "  n es el tamaño de la cadena a generar\n  "
               "L es la letra de la que se quiere contar apariciones (A, C, G o T)\n");
        exit(1);
    }

    // Declaracion de variables MPI
    int rank, numprocs, namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    int i, n, count=0, buffcount=0;
    char *cadena;
    char L;

    // Obtenemos el numero de procesos, el rango del proceso y el nombre del procesador
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(processor_name, &namelen);

    if(rank == 0) {
        L = *argv[2];
        n = atoi(argv[1]);
    }


    //Opcion de arbol binomial
    MPI_BinomialBcast(&L, 1, MPI_CHAR, MPI_COMM_WORLD);
    MPI_BinomialBcast(&n, 1, MPI_INT, MPI_COMM_WORLD);

    //Opcion de arbol plano
    //MPI_Bcast(&L, 1, MPI_CHAR, 0, MPI_COMM_WORLD);
    //MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);


    cadena = (char *) malloc(n*sizeof(char));
    inicializaCadena(cadena, n);

    for(i=rank*n/numprocs; i<(rank+1)*n/numprocs; i++){//otra opcion es i=rank; i<n; i+=numprocs
        if(cadena[i] == L){
            count++;
        }
    }


    //Opcion de arbol binomial
    MPI_FlattreeColectiva(&count, &buffcount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    //Opcion de arbol plano
    //MPI_Reduce(&count, &buffcount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);


    if (rank == 0) {
        printf("El numero de apariciones de la letra %c es %d\n", L, buffcount);
    }
    free(cadena);
    MPI_Finalize();
    exit(0);
}
