/*
	Test program for BERT parallellizing converter.
	Calculates Mandelbrot set.
	After processing resulting pattern will be written into file
	MANDEL.PCX.

	(BERT77, version 1.05. Copyright (C) 1991-1998 Paralogic, Inc.)
*/

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <mpi.h>

#define XMAX	3840
#define YMAX	2880

#define XBEG	(-3.0)
#define XSTEP	0.0075
#define YBEG	3.0
#define YSTEP	0.02

#define quit() { \
	MPI_Abort(MPI_COMM_WORLD, EXIT_FAILURE); \
    return EXIT_FAILURE; \
}

#define finalize() { \
	if (MPI_Finalize() != MPI_SUCCESS) { \
		fprintf(stderr, "ERROR: MPI_Finalize\n"); \
		quit(); \
	} \
}

#define exit_failure(error) { \
	fprintf(stderr, error); \
	free(screen); \
	finalize(); \
	return EXIT_FAILURE; \
}

int compute(int x, int y);
void savefile(int (*screen)[YMAX+1]);	

int main() {
	int task, rank, world_size;
	{ // Initialization
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
			finalize();
			return EXIT_FAILURE;
		}

		if (MPI_Comm_size(MPI_COMM_WORLD, &world_size) != MPI_SUCCESS) {
			fprintf(stderr, "ERROR: MPI_Comm_size\n");
			finalize();
			return EXIT_FAILURE;
		}
	}
		
	int (*screen)[YMAX+1] = malloc((XMAX + 1) * (YMAX + 1) * sizeof(int));

	if (screen == NULL) {
		fprintf(stderr, "Memory allocation error\n");
		finalize();
		return EXIT_FAILURE;
	}

	int Ny = YMAX / world_size + (rank == 0 ? YMAX % world_size + 1 : 0);

	int Ydispl = rank == 0 ? 0 : rank * (YMAX / world_size) + YMAX % world_size + 1;

	int x, y;
	for (x = 0; x < XMAX; x++) {
		for(y = 0; y <= Ny; y++) {
			screen[x][y + Ydispl] = compute(x, y + Ydispl);
		}
	}
	
	MPI_Datatype MyVectorType;
	if (MPI_Type_vector(world_size - 1, YMAX / world_size, YMAX, MPI_INT, &MyVectorType) != MPI_SUCCESS) {
		exit_failure("ERROR: MPI_Type_vector\n");
	}
	if (MPI_Type_commit(&MyVectorType) != MPI_SUCCESS) {
		exit_failure("ERROR: MPI_Type_commit\n");
	}
	
	if (rank == 0) {
		Ydispl += Ny;
		for (size_t i = 1; i < world_size; ++i) {
			if (MPI_Recv(screen + Ydispl, 1, MyVectorType, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
				exit_failure("ERROR: MPI_Recv\n");
			}
			Ydispl += YMAX / world_size;
		}
		printf("SAVE RESULT\n");
		savefile(screen);
	} else {
		if (MPI_Send(screen + Ydispl, 1, MyVectorType, 0, 0, MPI_COMM_WORLD) != MPI_SUCCESS) {
			exit_failure("ERROR: MPI_Send\n");
		}
	}
	
	
	if (MPI_Type_free(&MyVectorType) != MPI_SUCCESS) {
		exit_failure("ERROR: MPI_Type_free\n");
	}
	free(screen);
	finalize();
	return EXIT_SUCCESS;
}

/*
	Function to calculate Mandelbrot set.
*/

#define bc (YBEG - y * YSTEP)
#define ac (XBEG + x * XSTEP)

int compute(int x, int y){

	double size = 0.0;
	double a2, b2;
	double a = ac, b = bc;
	int	 i, count = 0;

	if (fabs(a) > 1e19) a2 = 1e38;
	else	a2 = a * a;

	if(fabs(b) > 1e19) b2 = 1e38;
	else	b2 = b * b;

	for(i = 0;  i <= 100; i++){
		b = 2.0 * a * b + bc;
		if(fabs(b) > 1e38) b2 = 3e38*(b/fabs(b));
		a = a2 - b2 + ac;
		
		if(fabs(a) > 1e19) a2 = 2e38;
		else	a2 = a * a;

		if(fabs(b) > 1e19) b2 = 2e38;
		else	b2 = b * b;
		size = a2 + b2;
		if(size <= 500.0) count = i;
	} 
	return count;
}


/*
	Function for saving PCX-file.
*/


#define X0 0
#define Y0 0

#define XSIZEBYTE 80
#define NUMPLAN	4

void savefile(int (*screen)[YMAX+1]){
	void writeheader(int fd);
	void writebyte  (int fd, unsigned char byte);
	void writerep	(int fd, int count, unsigned char byte);
	unsigned char getbyte	 (int i, int j, int k, int (*screen)[YMAX+1]);

	int 	count, i, j, k, zero = 0, fd;
	unsigned char 	lastbyte, currbyte;

	if((fd = open("mymandel.pcx",O_CREAT|O_WRONLY,0666)) < 0){
		printf("Can\'t open file\n"); 
		free(screen); exit(-1);
	}

	writeheader(fd);

	for(j = Y0; j < Y0+350; j++){
		 for (k = 0; k < NUMPLAN; k++){
		  lastbyte = getbyte(zero,j,k,screen);
		  count = 0;
			  if(lastbyte >= 192) count = 193;
		  for (i = 1; i < XSIZEBYTE; i++){
			currbyte = getbyte(i,j,k,screen);
			if(currbyte == lastbyte){
				if(count > 0) count = count + 1;
				else	count = 194;
			} else {
				if(count > 0) {
					 writerep(fd, count, lastbyte);
					 count = 0;
				} else
					 writebyte(fd, lastbyte);
			}
			lastbyte = currbyte;
		  }
		  
		  if(count > 0) 
				writerep(fd, count, lastbyte);
		  else
				writebyte(fd, lastbyte);
		 }
	}
	close(fd);  
}

/*
	Function for writing byte in PCX-file.
*/
 	
void writebyte(int fd, unsigned char wrbyte){
	unsigned char tmpbyte;

	if(wrbyte >= 192){
		tmpbyte = 193;
		write(fd, &tmpbyte, 1);
	} 
		  write(fd, &wrbyte, 1);
}

/*
	  Function for repeat byte writing in PCX-file.
*/

void writerep(int fd, int num, unsigned char wrbyte){
	unsigned char tmpbyte;
	if(num <= 255){
		  tmpbyte = num;
		  write(fd, &tmpbyte, 1);
		  write(fd, &wrbyte, 1);
	} else {
		  tmpbyte = 255;
		  write(fd, &tmpbyte, 1);
		  write(fd, &wrbyte, 1);
		  tmpbyte = num - 63;
		  write(fd, &tmpbyte, 1);
			 write(fd, &wrbyte, 1);
	}
}

/*
		 Function for pixel construction.
*/

unsigned char getbyte(int xb, int y, int plan, int (*color)[YMAX+1]){

	int	i, j, tmp;
  	char ret;

	unsigned char pow2[8] = {1, 2, 4, 8, 16, 32, 64, 128};

	if((X0 + xb*8 + 7 > XMAX) || (y > YMAX)) return 0;

  	ret = 0; i = X0 + xb*8;
	for(j = 7; j >= 0; j--){ 
		  tmp = color [i][y] & pow2[plan];
		if(tmp > 0) ret = ret + pow2[j];
		  i = i+1;
	}
	return ret;
}

/*
		Function for PCX-file header writing.
*/

void writeheader(int fd){
 
	int i;
 	unsigned char head[128] = 
		  {10,5,1,1,0,0,0,0,127,2,93,1,128,2,94,1,
		0,0,0,0,0,170,85,85,85,170,170,170,170,0,0,255,
		255,85,170,85,0,170,170,170,85,85,85,85,85,255,85,255,
		85,85,85,255,255,85,85,255,85,255,255,255,85,255,255,
		255,0,4,80,0,1};

	for (i = 69; i < 129; i++) head[i] = 0;
	write (fd, head, 128);
}
