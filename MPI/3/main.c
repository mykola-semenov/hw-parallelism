#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#define TIME_SERVER 0
#define NUMBER_OF_REQUESTS 2

int main() {
	int task, rank, world_size;
	if (MPI_Initialized(&task) != MPI_SUCCESS) {
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		return EXIT_FAILURE;
	}
	if (task == 0) {
		if (MPI_Init(NULL, NULL) != MPI_SUCCESS) {
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			return EXIT_FAILURE;
		}
	}
	if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS) {
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		return EXIT_FAILURE;
	}
	if (MPI_Comm_size(MPI_COMM_WORLD, &world_size) != MPI_SUCCESS) {
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		return EXIT_FAILURE;
	}
	char filename[10];
	sprintf(filename, "%d.dat", rank);
	FILE* file = fopen(filename, "w");
	if (file == NULL) {
		perror("fopen");
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		return EXIT_FAILURE;
	}
	
	srand(time(NULL) + rank);

	float T_UTC;
	
	if (rank == TIME_SERVER) {
		T_UTC = (float) (rand() % 1000);
		fprintf(file, "T = %f\n", T_UTC);
		int cli_rank;
		for (size_t i = 0; i < (world_size - 1) * NUMBER_OF_REQUESTS; ++i) {
			if (MPI_Recv(&cli_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				return EXIT_FAILURE;
			}
			if (MPI_Send(&T_UTC, 1, MPI_FLOAT, cli_rank, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				return EXIT_FAILURE;
			}
			fprintf(file, "Sent to %d\n", cli_rank);
			T_UTC += (float) (rand() % 10);
		}
	} else {
		float T_0 = (float) (rand() % 1000);
		float T_1;
		for (size_t i = 0; i < NUMBER_OF_REQUESTS; ++i) {
			fprintf(file, "REQUEST #%d\n", i);
			fprintf(file, "T_0 = %f\n", T_0);

			if (MPI_Send(&rank, 1, MPI_INT, TIME_SERVER, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				return EXIT_FAILURE;
			}

			T_1 = T_0 + (float) (rand() % 10);
			fprintf(file, "T_1 = %f\n", T_1);

			if (MPI_Recv(&T_UTC, 1, MPI_FLOAT, TIME_SERVER, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
				MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
				return EXIT_FAILURE;
			}

			fprintf(file, "T_UTC = %f\n", T_UTC);
			T_0 = T_1 == T_UTC + (T_1 - T_0) / 2 ? T_1 : T_1 + T_UTC - (T_1 + T_0)/2;
			fprintf(file, "T_new = %f\n\n", T_0);
		}
	}
	
	fclose(file);
	if (MPI_Finalize() != MPI_SUCCESS) {
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}