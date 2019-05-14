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

#include <fcntl.h>

#include <errno.h>

#define RECEIVER_PORT 51000
#define SESSION_FILE "session"

#define IP_LENGTH 16
#define IP_INPUT_FORMAT "%15s"
#define MESSAGE_LENGTH 1001
#define MESSAGE_INPUT_FORMAT "%1000[^\n]"

void flush_stdin() {
	int ch;
	do {
		ch = getchar();
	} while(ch != EOF && ch != '\n');
	clearerr(stdin); /* Clear EOF and ERR state */
}

int main() {
	const int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}

	struct sockaddr_in receiver_addr, sender_addr;

	bzero(&sender_addr, sizeof(sender_addr));
	sender_addr.sin_family = AF_INET;
	sender_addr.sin_port = htons(0);
	sender_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr*) &sender_addr, sizeof(sender_addr)) < 0) {
		perror("bind");
		if (close(sockfd) < 0) {
			perror("close");
		}
		return EXIT_FAILURE;
	}

	bzero(&receiver_addr, sizeof(receiver_addr));
	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_port = htons(RECEIVER_PORT);

	char message[MESSAGE_LENGTH], ip[IP_LENGTH];

	const int sessionfd = creat(SESSION_FILE, (mode_t) 0644);
	if (sessionfd < 0) {
		perror("creat");
		if (close(sockfd) < 0) {
			perror("close");
		}
		return EXIT_FAILURE;
	}

	if (close(sessionfd) < 0) {
		perror("close");
		if (close(sockfd) < 0) {
			perror("close");
		}
		if (unlink(SESSION_FILE) < 0) {
			perror("unlink");
		}
		return EXIT_FAILURE;
	}

	while (true) {
		scanf(IP_INPUT_FORMAT, ip);

		if (strcmp(ip, "0.0.0.0") == 0) {
			printf("Exiting...");
			if (close(sockfd) < 0) {
				perror("close");
				if (unlink(SESSION_FILE) < 0) {
					perror("unlink");
				}
				return EXIT_FAILURE;
			}
			if (unlink(SESSION_FILE) < 0) {
				perror("unlink");
				return EXIT_FAILURE;
			}
			return EXIT_SUCCESS;
		}

		if (inet_aton(ip, &receiver_addr.sin_addr) == 0) {
			printf("Invalid IP address\n");
			flush_stdin();
			continue;
		}

		bzero(message, MESSAGE_LENGTH);
		scanf(" "MESSAGE_INPUT_FORMAT, message);

		if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr*) &receiver_addr, sizeof(receiver_addr)) < 0) {
			perror("sendto");
			if (close(sockfd) < 0) {
				perror("close");
			}
			if (unlink(SESSION_FILE) < 0) {
				perror("unlink");
			}
			return EXIT_FAILURE;
		}
	}
}
