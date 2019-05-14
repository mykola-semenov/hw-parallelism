#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <time.h>
#include <signal.h>
#include <unistd.h>

int rank, world_size;
bool is_delay;
int witness = 2;

void leave(int sig) {
	is_delay = false;
}

void quit() {
	MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE);
	exit(EXIT_FAILURE);	
}

void init() {
	int task;
	if (MPI_Initialized(&task) != MPI_SUCCESS) {
		fprintf(stderr, "ERROR: MPI_Initialized\n");
		quit();
	}
	if (task == 0) {
		if (MPI_Init(NULL, NULL) != MPI_SUCCESS) {
			fprintf(stderr, "ERROR: MPI_Init\n");
			quit();
		}
	}
	if (MPI_Comm_rank(MPI_COMM_WORLD, &rank) != MPI_SUCCESS) {
		fprintf(stderr, "ERROR: MPI_Comm_rank\n");
		quit();
	}
	if (MPI_Comm_size(MPI_COMM_WORLD, &world_size) != MPI_SUCCESS) {
		fprintf(stderr, "ERROR: MPI_Comm_size\n");
		quit();
	} else if (world_size < witness + 1) {
		fprintf(stderr, "ERROR: -np must be at least %u\n", witness + 1);
		if (MPI_Finalize() != MPI_SUCCESS) {
			fprintf(stderr, "ERROR: MPI_Finalize\n");
			quit();
		}
		exit(EXIT_FAILURE);
	}
	srand(time(NULL) + rank);
	signal(SIGALRM, leave);
}

void finalize() {
	if (MPI_Finalize() != MPI_SUCCESS) {
		fprintf(stderr, "ERROR: MPI_Finalize\n");
		quit();
	}
	exit(EXIT_SUCCESS);
}

void send_to(int addr, int tag) {
	MPI_Request request;
	if (MPI_Isend(NULL, 0, MPI_INT, addr, tag, MPI_COMM_WORLD, &request) != MPI_SUCCESS) {
		fprintf(stderr, "ERROR: MPI_Send\n");
		quit();
	}
}

void receive(int tag, MPI_Status* status) {
	if (MPI_Recv(NULL, 0, MPI_INT, MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, status) != MPI_SUCCESS) {
		fprintf(stderr, "ERROR: MPI_Recv\n");
		quit();
	}
}

bool is_sent(int tag) {
	int flag;
	if (MPI_Iprobe(MPI_ANY_SOURCE, tag, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
		fprintf(stderr, "ERROR: MPI_Iprobe\n");
		quit();
	}
	if (flag) {
		return true;
	}
	return false;
}

unsigned int max_delay = 2;

int vote_tag = 0;
int ok_tag = 1;
int leader_tag = 2;

bool is_leader() {
	is_delay = true;
	for (int i = rank + 1; i < world_size; ++i) {
		send_to(i, vote_tag);
		printf("%d: sent 'vote' to %d\n", rank, i);
	}
	alarm(max_delay);
	MPI_Status status;
	while (is_delay) {
		if (is_sent(ok_tag)) {
			printf("%d: received 'ok'\n", rank);
			return false;
		} else if (is_sent(vote_tag)) {
			receive(vote_tag, &status);
			printf("%d: received 'vote' from %d\n", rank, status.MPI_SOURCE);
			send_to(status.MPI_SOURCE, ok_tag);
			printf("%d: sent 'ok' to %d\n", rank, status.MPI_SOURCE);
		}
	}
	return true;
}

int main() {
	init();
	if (rank == witness) {
		if (is_leader()) {
			printf("%d: leader\n", rank);
			for (int i = 0; i < world_size; ++i) {
				send_to(i, leader_tag);
			}
		} else {
			printf("%d: not leader\n", rank);
		} 
	} else if (rand() % 2) {
		printf("%d: died\n", rank);
	} else {
		MPI_Status status;
		bool voted = false;
		do {
			receive(MPI_ANY_TAG, &status);
			if (status.MPI_TAG == vote_tag) {
				printf("%d: received 'vote' from %d\n", rank, status.MPI_SOURCE);
				send_to(status.MPI_SOURCE, ok_tag);
				printf("%d: sent 'ok' to %d\n", rank, status.MPI_SOURCE);
				if (!voted) {
					if (is_leader()) {
						printf("%d: leader\n", rank);
						for (int i = 0; i < world_size; ++i) {
							send_to(i, leader_tag);
						}
						break;
					} else {
						printf("%d: not leader\n", rank);
					} 
					voted = true;
				}
			}
		} while(status.MPI_TAG != leader_tag);
	}
	finalize();
}
