#include "../include/logger.h"
#include "../include/global.h"

/* Utility function for the submission to be graded */
void print_author(void) {
	const char team_name[] = "pa1-nveerava";
	cse4589_print_and_log("[%s:SUCCESS]\n", "AUTHOR");
	cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", team_name);
	cse4589_print_and_log("[%s:END]\n", "AUTHOR");
}

/* Utility funciton to print the IP address of the calling application's host */
void print_ip(void) {
	const char server_ip[INET_ADDRSTRLEN] = "8.8.8.8";
	const unsigned short server_port = 53;
	const char cmd[] = "IP";

	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		cse4589_print_and_log("[%s:ERROR]\n", cmd);
		cse4589_print_and_log("[%s:END]\n", cmd);
		return;
	}

	/* Fill address structure with server information */
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);

	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
		close(sockfd);
		cse4589_print_and_log("[%s:ERROR]\n", cmd);
		cse4589_print_and_log("[%s:END]\n", cmd);
		return;
	}

	/* Connect to the remote server */
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		close(sockfd);
		cse4589_print_and_log("[%s:ERROR]\n", cmd);
		cse4589_print_and_log("[%s:END]\n", cmd);
		return;
	}

	/* Extract client information from the UDP socket */
	struct sockaddr_in client_info;
	socklen_t client_info_len = sizeof(client_info);
	char ip_addr[INET_ADDRSTRLEN];
	if(getsockname(sockfd, (struct sockaddr *)&client_info, &client_info_len) == -1) {
		close(sockfd);
		cse4589_print_and_log("[%s:ERROR]\n", cmd);
		cse4589_print_and_log("[%s:END]\n", cmd);
		return;
	}

	
	/* Convert the IP address to print format */
	if (inet_ntop(AF_INET, &(client_info.sin_addr), ip_addr, sizeof(ip_addr)) == NULL) {
		close(sockfd);
		cse4589_print_and_log("[%s:ERROR]\n", cmd);
		cse4589_print_and_log("[%s:END]\n", cmd);
		return;
	}

	cse4589_print_and_log("[%s:SUCCESS]\n", cmd);
	cse4589_print_and_log("IP:%s\n", ip_addr);
	cse4589_print_and_log("[%s:END]\n", cmd);
	
	close(sockfd);
	return;
}

/* Same as the above function but returns the IP address of the client to the caller in the buffer provided as argument */
void get_self_ip(char *ip) {
	const char server_ip[INET_ADDRSTRLEN] = "8.8.8.8";
	const unsigned short server_port = 53;
	// const char cmd[] = "IP";

	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		return;
	}

	/* Fill address structure with server information */
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);

	if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
		close(sockfd);
		return;
	}

	/* Connect to the remote server */
	if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
		close(sockfd);
		return;
	}

	/* Extract client information from the UDP socket */
	struct sockaddr_in client_info;
	socklen_t client_info_len = sizeof(client_info);
	char ip_addr[INET_ADDRSTRLEN];
	if(getsockname(sockfd, (struct sockaddr *)&client_info, &client_info_len) == -1) {
		close(sockfd);
		return;
	}

	
	/* Convert the IP address to print format */
	if (inet_ntop(AF_INET, &(client_info.sin_addr), ip_addr, sizeof(ip_addr)) == NULL) {
		close(sockfd);
		return;
	}

	/* Need to ensure that the caller provided buffer is valid before copying client IP address */
	if (ip != NULL)
		strncpy(ip, ip_addr, INET_ADDRSTRLEN);
	else {
		fprintf(stderr, "Invalid buffer passed to get_self_ip by the caller\n");
		close(sockfd);
		return;
	}
	
	close(sockfd);
	return;
}

/* Utility funciton that prints the port number of the input parameter socket */
void print_port(int fd) {
	struct sockaddr_in client_info;
	socklen_t client_info_len = sizeof(client_info);
	
	if (getsockname(fd, (struct sockaddr *)&client_info, &client_info_len) == -1) {
		cse4589_print_and_log("[PORT:ERROR]\n");
		cse4589_print_and_log("[PORT:END]\n");
	} else {
		cse4589_print_and_log("[PORT:SUCCESS]\n");
		cse4589_print_and_log("PORT:%d\n", ntohs(client_info.sin_port));
		cse4589_print_and_log("[PORT:END]\n");
	}
}

/* Utility function to compare to users by their port number */
int compare_users(const void *a, const void *b) {
    const struct user *user_a = *(const struct user **)a;
    const struct user *user_b = *(const struct user **)b;

     if (user_a == NULL && user_b == NULL) {
        return 0;
    } else if (user_a == NULL) {
        return 1; // NULL is considered greater than a valid user
    } else if (user_b == NULL) {
        return -1; // Valid user is considered less than NULL
    }

    // Comparing port numbers
    return user_a->port - user_b->port;
}
