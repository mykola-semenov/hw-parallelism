#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
	MPI_Init(NULL, NULL);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	int msg;
	if (rank != 0) {
		MPI_Recv(&msg, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		printf("%d: %d\n", rank, msg);
	} else {
		msg = 1;
	}

	msg += rank;
	if (rank + 1 < world_size) {
		MPI_Send(&msg, 1, MPI_INT, (rank + 1) % world_size, 0, MPI_COMM_WORLD);
	}
	MPI_Finalize();
	return EXIT_SUCCESS;
}
