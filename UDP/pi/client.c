#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 51001

const double limits[2] = {0, 2}; 

typedef struct {
	double begin;
	double end;
} message_t;

int main(int argc, char* argv[]) {
	struct sockaddr_in srv_addr, cli_addr;

	bzero(&cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = htons(0);
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(SERVER_PORT);

	if (argc != 3) {
		printf("Invalid amount of arguments\n");
		return EXIT_FAILURE;
	}
	
	unsigned int N;
	sscanf(argv[2], "%ud", &N);
	
	if (inet_aton(argv[1], &srv_addr.sin_addr) == 0) {
		printf("Invalid IP address\n");
		return EXIT_FAILURE;
	}

	const int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	
	if (bind(sockfd, (struct sockaddr*) &cli_addr, sizeof(cli_addr)) < 0) {
		perror("bind");
		if (close(sockfd) < 0) {
			perror("close");
		}
		return EXIT_FAILURE;
	}

	double pi = 0.0;
	double I;
	
	double H = (limits[1] - limits[0]) / (double) N;

	message_t message = {0.0, H};

	for (int i = 0; i < N; ++i) {
		bzero(&I, sizeof(double));
		if (sendto(sockfd, &message, sizeof(message), 0, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0) {
			perror("sendto");
			if (close(sockfd) < 0) {
				perror("close");
			}
			return EXIT_FAILURE;
		}
		
		if (recvfrom(sockfd, &I, sizeof(double), 0, NULL, NULL) < 0) {
			perror("recvfrom");
			if (close(sockfd) < 0) {
				perror("close");
			}
			return EXIT_FAILURE;
		}
		
		pi += I;
		
		message.begin += H;
		message.end += H;
	}
	
	printf("Pi = %lf", pi);
	return EXIT_SUCCESS;
}
