#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#include <math.h>

#define SETTINGS_FILE "settings.ini"
#define ROOT_PROC 0

double f(double x) {
	return sqrt(fabs(4 - x*x));
}

double integrate(double f(double), const double begin, const double end, const double step) {
	double sum = (f(begin) + f(end)) / 2;
	for (double x = begin + step; x < end; x += step) {
		sum += f(x);
	}
	return sum * step;
}

double integrate_Simpson(double f(double), const double begin, const double end, const double step) {
	double sum = 0.0;
	for (double x = begin + step; x < end; x += 2 * step) {
		sum += f(x - step) + 4 * f(x) + f(x + step);
	}
	return (sum / 3.0) * step;
}

int main() {
	int task, rank, world_size;
	if (MPI_Initialized(&task) != MPI_SUCCESS) {
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

	double msg[3];
	if (rank == ROOT_PROC) {
		FILE* fs = fopen("settings.ini", "r");
		if (fs == NULL) {
			// TODO
		}
		fscanf(fs, "%lf %lf %lf", &msg[0], &msg[1], &msg[2]);
		fclose(fs);
	}
	MPI_Bcast(&msg, 3, MPI_DOUBLE, ROOT_PROC, MPI_COMM_WORLD);	

	double H = (msg[1] - msg[0]) / (double) world_size;

	double I = integrate_Simpson(f, msg[0] + H * (double) rank, msg[0] + H * (double) (rank + 1), msg[2]);
	
	double pi;
	MPI_Reduce(&I, &pi, 1, MPI_DOUBLE, MPI_SUM, ROOT_PROC, MPI_COMM_WORLD);
//	MPI_Allreduce(&I, &pi, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

	if (rank == ROOT_PROC) {	
		printf("%1.10lf\n", pi);
	}
	
	MPI_Finalize();
	return EXIT_SUCCESS;
}
