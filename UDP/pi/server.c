#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>

#include <math.h>

#define SERVER_PORT 51001

const double h = 1e-6;

double f(double x) {
	return sqrt(fabs(4 - x*x));
}

typedef struct {
	double begin;
	double end;
} message_t;

double Integral(double f(double), const double begin, const double end, const double step) {
	double I = 0.0;
	for (double x = begin; x < end; x += step) {
		I += step * (f(x) + f(x + step)) / 2;
	}
	return I;
}

int main() {
	const int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}
	
	message_t message;
	double I;
	struct sockaddr_in srv_addr, cli_addr;

	bzero(&srv_addr, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(SERVER_PORT);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0) {
		perror("bind");
		if (close(sockfd) < 0) {
			perror("close");
		}
		return EXIT_FAILURE;
	}

	int cli_len;

	while (true) {
		bzero(&message, sizeof(message_t));
		bzero(&I, sizeof(double));
		
		cli_len = sizeof(cli_addr);
		if (recvfrom(sockfd, &message, sizeof(message), 0, (struct sockaddr*) &cli_addr, &cli_len) < 0) {
			perror("recvfrom");
			break;
		}

		I = Integral(f, message.begin, message.end, h);
		
		if (printf("Integral from %lf to %lf of f(x)dx equals %lf\n", message.begin, message.end, I) < 0) {
			perror("printf");
			break;
		}

		if (sendto(sockfd, &I, sizeof(double), 0, (struct sockaddr*) &cli_addr, cli_len) < 0) {
			perror("sendto");
			break;
		}
	}

	if (close(sockfd) < 0) {
		perror("close");
	}
	return EXIT_FAILURE;
}