#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#define TIME_SERVER 0

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
	float T = (float) (rand() % 1000);
	fprintf(file, "T = %f\n", T);
	
	float correction = 0;
	if (rank == TIME_SERVER) {
		float* deltaT = (float*) malloc((size_t) world_size * sizeof(float));
		if (MPI_Gather(&T, 1, MPI_FLOAT, deltaT, 1, MPI_FLOAT, TIME_SERVER, MPI_COMM_WORLD) != MPI_SUCCESS) {
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			return EXIT_FAILURE;
		}
		
		float averageDelta = 0;
		fprintf(file, "\n");
		for (size_t i = 0; i < world_size; ++i) {
			fprintf(file, "T_%d = %f\n", i, deltaT[i]);
			deltaT[i] -= T;
			averageDelta += deltaT[i] / world_size;
		}
		fprintf(file, "\n");
		for (size_t i = 0; i < world_size; ++i) {
			fprintf(file, "deltaT_%d = %f\n", i, deltaT[i]);
		}
		
		fprintf(file, "\naverage = %f\n\n", averageDelta);
		
		for (size_t i = 0; i < world_size; ++i) {
			deltaT[i] = averageDelta - deltaT[i];
			fprintf(file, "correction_%d = %f\n", i, deltaT[i]);
		}
		
		fprintf(file, "\n");
		
		if (MPI_Scatter(deltaT, 1, MPI_FLOAT, &correction, 1, MPI_FLOAT, TIME_SERVER, MPI_COMM_WORLD) != MPI_SUCCESS) {
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			return EXIT_FAILURE;
		}
		free(deltaT);
	} else {
		if (MPI_Gather(&T, 1, MPI_FLOAT, NULL, 1, MPI_FLOAT, TIME_SERVER, MPI_COMM_WORLD) != MPI_SUCCESS) {
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			return EXIT_FAILURE;
		}
		if (MPI_Scatter(NULL, 1, MPI_FLOAT, &correction, 1, MPI_FLOAT, TIME_SERVER,		MPI_COMM_WORLD) != MPI_SUCCESS) {
			MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
			return EXIT_FAILURE;
		}
		fprintf(file, "correction = %f\n", correction);
	}
	T += correction;
	fprintf(file, "T_new = %f\n", T);
	fclose(file);
	if (MPI_Finalize() != MPI_SUCCESS) {
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}