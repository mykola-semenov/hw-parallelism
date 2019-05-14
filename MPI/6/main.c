#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define TIME_SERVER 0

int rank;
unsigned short t[] = {0, 0, 0};

void send_to(int addr) {
	t[rank]++;
	if (MPI_Send(t, 3, MPI_UNSIGNED_SHORT, addr, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		exit(EXIT_FAILURE);
	}
	printf("P%d: send to %d, t = [%d, %d, %d]\n", rank, addr, t[0], t[1], t[2]);
}

void recv_from(int addr) {
	unsigned short t_temp[3];
	if (MPI_Recv(t_temp, 3, MPI_UNSIGNED_SHORT, addr, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		exit(EXIT_FAILURE);
	}
	for (size_t i = 0; i < 3; ++i) {
		if (t[i] < t_temp[i] && i != rank) {
			t[i] = t_temp[i];
		}
	}
	t[rank]++;
	printf("P%d: recv from %d, t = [%d, %d, %d]\n", rank, addr, t[0], t[1], t[2]);
}

void local() {
	t[rank]++;
	printf("P%d: local, t = [%d, %d, %d]\n", rank, t[0], t[1], t[2]);
}

int main() {
	int task, world_size;
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
	} else if (world_size < 3) {
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		return EXIT_FAILURE;
	}
	
	if (rank == 0) {
		local();
		recv_from(1);
		recv_from(1);
		send_to(2);
	} else if (rank == 1) {
		send_to(0);
		local();
		send_to(0);
		recv_from(2);
		recv_from(2);
		local();
	} else if (rank == 2) {
		local();
		send_to(1);
		local();
		recv_from(0);
		send_to(1);
	}

	if (MPI_Finalize() != MPI_SUCCESS) {
		MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}