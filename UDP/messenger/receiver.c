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

#include <time.h>

#include <fcntl.h>

#include <errno.h>

#define RECEIVER_PORT 51000
#define SESSION_FILE "session"
#define ANSWERING_MESSAGE "I'm sorry, but I'm currently not available."

#define MESSAGE_LENGTH 1001

int main() {
	const int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return EXIT_FAILURE;
	}
	int sessionfd;

	char message[MESSAGE_LENGTH];
	struct sockaddr_in receiver_addr, sender_addr;

	bzero(&receiver_addr, sizeof(receiver_addr));
	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_port = htons(RECEIVER_PORT);
	receiver_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (struct sockaddr*) &receiver_addr, sizeof(receiver_addr)) < 0) {
		perror("bind");
		if (close(sockfd) < 0) {
			perror("close");
		}
		return EXIT_FAILURE;
	}

	time_t timer;
	struct tm* time_info;

	int sender_len, n;

	while (true) {
		bzero(message, MESSAGE_LENGTH);

		sender_len = sizeof(sender_addr);
		n = recvfrom(sockfd, message, MESSAGE_LENGTH - 1, 0, (struct sockaddr*) &sender_addr, &sender_len);
		if (n < 0) {
			perror("recvfrom");
			break;
		}

		sessionfd = open(SESSION_FILE, 0 | O_RDONLY);
		if (sessionfd < 0) {
			if (errno == ENOENT) {
				strcpy(message, ANSWERING_MESSAGE);
				sender_addr.sin_family = AF_INET;
				sender_addr.sin_port = htons(51000);
				if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr*) &sender_addr, sender_len) < 0) {
					perror("sendto");
					break;
				}
			} else {
				perror("open");
				break;
			}
		} else {
			close(sessionfd);
			if (time(&timer) == (time_t) -1) {
				perror("time");
				break;
			}

			time_info = localtime(&timer);
			if (time_info == NULL) {
				perror("localtime");
				break;
			}

			message[n] = '\n';
			if (printf("%02d:%02d:%02d [%s]\t%s",
					time_info->tm_hour, time_info->tm_min, time_info->tm_sec,
					inet_ntoa(sender_addr.sin_addr), message) < 0) {
				perror("printf");
				break;
			}
		}
	}

	if (close(sockfd) < 0) {
		perror("close");
	}
	return EXIT_FAILURE;
}