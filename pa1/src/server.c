#include "logger.h"
#include "global.h"

// #include <ctype.h>

#define BACKLOG 5 /* The maximum simultaneous requests the server can listen to at any given time */

/**
 * * @section 
 * * 
 * * SERVER function:
 * * This is the main function that drives the server application.
 * * Initializes the list of all the users connected to server since it started.
 * * Set up the listening socket.
 * *
 * * Roles:
 * * Accepts connections
 * * Process user (usually the server adminstrator) commands
 * * Serves client requests
 * *
 * * @param  port Server's listening socket port number
 * * @return 0 EXIT_SUCCESS
 * */
int server(const char *port) {
	struct user * users[MAX_USERS] = {NULL};
	size_t user_count = 0;

	int server_socket, head_fd, selret, fd_index, fdaccept = 0;
	socklen_t clientaddr_len;
	struct sockaddr_in client_addr;
	struct addrinfo hints, *res;
	fd_set master_list, watch_list;

	/* Setting up the address structure for the server_socket */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(NULL, port, &hints, &res) != 0) {
		exit(EXIT_FAILURE);
	}

	/* Creating a listeining socket for the server */
	server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (server_socket < 0) {
		freeaddrinfo(res);
		exit(EXIT_FAILURE);
	}

	/* Setting the port number to be reusable immediately */
	int reuse = TRUE;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	/* Bining the server_socket to the port number */
	if (bind(server_socket, res->ai_addr, res->ai_addrlen) < 0) {
		freeaddrinfo(res);
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	/* Starting the server_socket to accept incoming connections */
	if (listen(server_socket, BACKLOG) < 0) {
		freeaddrinfo(res);
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	/* Initializing the socket lists to be monitored by select call */
	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);

	/* Registering server_socket and STDIN to be monitored by select */
	FD_SET(server_socket, &master_list);
	FD_SET(STDIN_FILENO, &master_list);

	head_fd = server_socket;


	while (TRUE) {
		memcpy(&watch_list, &master_list, sizeof(master_list));

		selret = select(head_fd + 1, &watch_list, NULL, NULL, NULL);

		if (selret < 0) {
			for (fd_index = 0; fd_index <= head_fd; ++fd_index) {
				close(fd_index);
			}
			exit(EXIT_FAILURE);
		}

		if (selret > 0) {
			/* Loop through socket descriptors to check which ones are ready */
			for (fd_index = 0; fd_index <= head_fd; ++fd_index) {
				if (FD_ISSET(fd_index, &watch_list)) {
					if (fd_index == STDIN_FILENO) {	/* Received data at input to process */
						char *user_input = (char *) malloc(sizeof(char) * USER_INPUT_SIZE);
						memset(user_input, '\0', USER_INPUT_SIZE);

						if (fgets(user_input, USER_INPUT_SIZE - 1, stdin) == NULL) { 
							free(user_input);
							exit(EXIT_FAILURE);
						}

						/* replacing newline character with null terminator */
						char *ptr_input = user_input;
						while (*ptr_input != '\0') {
							if (*ptr_input == '\n') {
								*ptr_input = '\0';
								break;
							}
							ptr_input++;
						}

						/* Processing AUTHOR command */
						if (strncmp(user_input, "AUTHOR", strlen("AUTHOR")) == 0 && (strlen(user_input) == strlen("AUTHOR")))
							print_author();

						/* Processing IP command */
						if (strncmp(user_input, "IP", strlen("IP")) == 0 && (strlen(user_input) == strlen("IP")))
							print_ip();

						/* Processing PORT command */
						if (strncmp(user_input, "PORT", strlen("PORT")) == 0 && (strlen(user_input) == strlen("PORT")))
							print_port(server_socket);

						/* Processing LIST command */
						if (strncmp(user_input, "LIST", strlen("LIST")) == 0 && (strlen(user_input) == strlen("LIST")))
							list_users(users, user_count);

						/* Processing STATISTICS command */
						if (strncmp(user_input, "STATISTICS", strlen("STATISTICS")) == 0 && (strlen(user_input) == strlen("STATISTICS"))) {
							int count = 0;

							// Sorting the list before displaying STATISTICS
							qsort(users, user_count, sizeof(struct user *), compare_users);

							if (user_count != 0) {
								cse4589_print_and_log("[%s:SUCCESS]\n", user_input);
								for (int i = 0; i < MAX_USERS; i++) {
									if (users[i] != NULL)
									{
										count++;
										// char *status;
										if (users[i] != NULL && users[i]->status == ONLINE)
											cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", count, users[i]->hostname, users[i]->msg_sent, users[i]->msg_recv, "logged-in");
										else if (users[i] != NULL && users[i]->status == OFFLINE)
											cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n", count, users[i]->hostname, users[i]->msg_sent, users[i]->msg_recv, "logged-out");
									}
								}
								cse4589_print_and_log("[%s:END]\n", user_input);
							} else {
								cse4589_print_and_log("[%s:ERROR]\n", "STATISTICS");
								cse4589_print_and_log("[%s:END]\n", "STATISTICS");
							}
						}

						/* Processing BLOCKED command */
						if (strncmp(user_input, "BLOCKED", strlen("BLOCKED")) == 0) {
							char *token;
							char *client_ip;

							token = strtok(user_input, " ");
							if (token != NULL) {
								if (strlen(token) != strlen("BLOCKED")) {
									free(user_input);
									continue;
								}

								client_ip = strtok(NULL, " ");
								if (client_ip != NULL) {
									unsigned char buf[sizeof(struct in_addr)];
									if (inet_pton(AF_INET, client_ip, buf) == 0)
									{
										cse4589_print_and_log("[%s:ERROR]\n", "BLOCKED");
										cse4589_print_and_log("[%s:END]\n", "BLOCKED");
										free(client_ip);
										free(user_input);
										continue;
									}

									ssize_t loc = 0;
									for (size_t i = 0; i < MAX_USERS; i++) {
										if (users[i] != NULL && (strncmp(users[i]->ip, client_ip, INET_ADDRSTRLEN) == 0)) {
											loc = i;
											break;
										}
									}
									if (loc == -1) {
										cse4589_print_and_log("[%s:ERROR]\n", "BLOCKED");
										cse4589_print_and_log("[%s:END]\n", "BLOCKED");
										free(user_input);
										continue;
									}

									/* Retreiving the list of blocked IPs for the client */
									size_t blocked_count = 0;
									if (users[loc] != NULL) {
										for (size_t i = 0; i < MAX_LIVE_USERS; i++) {
											if (users[loc] != NULL && users[loc]->blocked_list[i] != NULL) {
												blocked_count++;
											}
										}

										if (blocked_count > 0) {
											struct user **blocked_users = (struct user **)malloc((sizeof(struct user *)) * blocked_count);
											for (size_t index = 0; index < blocked_count; index++) {
												blocked_users[index] = NULL;
											}

											cse4589_print_and_log("[%s:SUCCESS]\n", "BLOCKED");
											for (size_t i = 0; i < MAX_LIVE_USERS; i++) {
												for (size_t j = 0; j < MAX_USERS; j++) {
													if (users[j] != NULL && users[loc]->blocked_list[i] != NULL && (strncmp(users[j]->ip, users[loc]->blocked_list[i], INET_ADDRSTRLEN) == 0)) {
														for (size_t k = 0; k < blocked_count; k++) {
															if (blocked_users[k] == NULL) {
																blocked_users[k] = users[j];
																break;
															}
														}
														break;
													}
												}
											}

											/* Sorting the blocked users list before printing them */
											qsort(blocked_users, blocked_count, sizeof(struct user *), compare_users);

											size_t count = 0;
											for (size_t index = 0; index < blocked_count; index++) {
												if (blocked_users[index] != NULL) {
													cse4589_print_and_log("%-5d%-35s%-20s%-8d\n", ++count, blocked_users[index]->hostname, blocked_users[index]->ip, blocked_users[index]->port);
												}
											}

											cse4589_print_and_log("[%s:END]\n", "BLOCKED");
											free(blocked_users);
											continue;
										}
									}
								} else {
									cse4589_print_and_log("[%s:ERROR]\n", "BLOCKED");
									cse4589_print_and_log("[%s:END]\n", "BLOCKED");
									free(user_input);
									continue;
								}
							}
						}

						free(user_input);

					} else if (fd_index == server_socket) { /* Received a new connection request to process */
						clientaddr_len = sizeof(struct sockaddr);
						fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, &clientaddr_len);
						if (fdaccept < 0) {
							fprintf(stderr, "Accept failed\n");
							continue;
						}
						
						/* Extracting the IP address and host name of the client from client_addr */
						char client_ip[INET_ADDRSTRLEN];
						if (inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN) == 0) {
							fprintf(stderr, "Could not extract client IP\n");
						}

						/* Extracting the client's port from client_addr */
						unsigned short client_port;
						if ((client_port = ntohs(client_addr.sin_port)) == 0) {
							fprintf(stderr, "Could not extract client port\n");
						}

						/* Getting the host name for the client's IP address */
						char hostname[NI_MAXHOST];
						if (getnameinfo((const struct sockaddr *)&client_addr, sizeof(client_addr), hostname, NI_MAXHOST, NULL, 0, NI_NAMEREQD) != 0) {
							fprintf(stderr, "Could not resolve the host name from client's IP address\n");
						}
						int location;
						if ((location = old_user(users, client_ip, client_port, hostname)) != -1) {
							fprintf(stderr, "Old user reconnecting\n");
							close(fdaccept);
						} else {
							/* Filling in the details of the new connection to the users array */
							for (int i = 0; i < MAX_USERS; i++) {
								if (users[i] == NULL) {
									users[i] = malloc(sizeof(struct user));
									if (users[i] == NULL) {
										fprintf(stderr, "allocating memory for user object failed\n");
										continue;
									}
									users[i]->fd = fdaccept;
									users[i]->cb = create_buffer();
									strncpy(users[i]->ip, client_ip, INET_ADDRSTRLEN);
									users[i]->port = client_port;
									users[i]->status = ONLINE;
									strncpy(users[i]->hostname, hostname, NI_MAXHOST);
									users[i]->msg_sent = 0;
									users[i]->msg_recv = 0;
									users[i]->backlog_count = 0;
									for (int index = 0; index < BACKLOG_BUFFER_SIZE; index++) {
										users[i]->backlog_list[index] = NULL;
									}
									// printf("After looping backlog list\n");
									for (int j = 0; j < MAX_LIVE_USERS; j++) {
										users[i]->blocked_list[j] = NULL;
									}
									user_count++;
									break;
								}
							}
						}

						size_t live_users = 0;
						for (int i = 0; i < MAX_USERS; i++) {
							if (users[i] != NULL && users[i]->status == ONLINE) {
								live_users++;
							}
						}

						int msg_len;
						char *msg;
						if (!(msg_len = generate_users_string(&msg, live_users, users))) {
							fprintf(stderr, "Something is wrong with generating live users string for a new incoming connection\n");
						}
						
						Packet *pkt = NULL;
						size_t pkt_len = make_packet("LOGIN", strlen("LOGIN"), msg, msg_len, &pkt);
						if (!pkt_len) {
							cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
							cse4589_print_and_log("[%s:END]\n", "LOGIN");
							free(msg);
							continue;
						}

						if (send_packet(fdaccept, pkt) != get_length(pkt)) {
							cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
							cse4589_print_and_log("[%s:END]\n", "LOGIN");
							free_packet(pkt);
							free(msg);	
						} else {
							FD_SET(fdaccept, &master_list);
							if (fdaccept > head_fd)
								head_fd = fdaccept;
						}

						int flags = fcntl(fdaccept, F_GETFL, 0);
						if (flags < 0) {
							fprintf(stderr, "fcntl failed. Try logging in later.\n");
						}

						flags |= O_NONBLOCK;
						if (fcntl(fdaccept, F_SETFL, flags) < 0) {
							fprintf(stderr, "Unable to set flags on client_socket. Try again later.\n");
						}

						free(msg);
						free_packet(pkt);
					} else { /* Received data from one of the clients to process */
						/* First find out which user's is sending data */
						ssize_t user_index;
						for (user_index = 0; user_index < MAX_USERS; user_index++) {
							if (users[user_index]!= NULL && (users[user_index]->fd == fd_index))
								break;
						}

						/* Now we can receive packeges into the user's circular buffer */
						Packet **pkt_arr = NULL;

						ssize_t num_pkts = receive_packets(fd_index, &pkt_arr, users[user_index]->cb);
						// printf("received packets, processing them...\n");
						if ((!num_pkts && !pkt_arr) || (num_pkts == -1 && !pkt_arr)) { /* Unexpected system problem */
							fprintf(stderr, "Unable to allocate memory when creating packet array\n");
							continue;
						} else if (num_pkts == -2 && !pkt_arr) { /* Dealing with unexpected shutdown */
							/* Ensuring that there are no backlog messages to be sent before logging out the user */
							size_t count = clear_blist(users[user_index]);

							if (count != users[user_index]->backlog_count) {
								fprintf(stderr, "Error encountered when clearing the backlog list of logged out user\n");
							}

							/* Clearing the blocked list of the user */
							for (int i = 0; i < MAX_LIVE_USERS; ++i) {
								if (users[user_index]->blocked_list[i] != NULL) {
									free(users[user_index]->blocked_list[i]);
									users[user_index]->blocked_list[i] = NULL;
								}
							}

							users[user_index]->status = OFFLINE;
							free_buffer(users[user_index]->cb);
							close(users[user_index]->fd);

							free(users[user_index]);
							users[user_index] = NULL;
							--user_count;

							/* Removing the socket from Select watch list */
							FD_CLR(fd_index, &master_list);
							continue;
						} else { /* Processing Client Packets */
							// printf("Received %d packets\n", num_pkts);
							for (int i = 0; i < num_pkts; i++) {
								char *cmd, *msg;
								/*ssize_t cmd_len =*/ get_command(pkt_arr[i], &cmd);
								/*ssize_t msg_len =*/ get_message(pkt_arr[i], &msg);

								/* Processing REFRESH request */
								if (strncmp(cmd, "REFRESH", strlen("REFRESH")) == 0) {
									size_t live_users = 0;
									for (int i = 0; i < MAX_USERS; i++) {
										if (users[i] != NULL && users[i]->status == ONLINE)
											live_users++;
									}

									int msg_len;
									char *outgoing_msg;
									if (!(msg_len = generate_users_string(&outgoing_msg, live_users, users))) {
										fprintf(stderr, "Something is wrong with generating live users string for a new incoming connection\n");
										continue;
									}

									Packet *pkt = NULL;
									size_t pkt_len = make_packet("REFRESH", strlen("REFRESH"), outgoing_msg, strlen(outgoing_msg), &pkt);
									if (!pkt_len) {
										cse4589_print_and_log("[%s:ERROR]\n", "REFRESH");
										cse4589_print_and_log("[%s:END]\n", "REFRESH");
										free(outgoing_msg);
										continue;
									}

									if (send_packet(fd_index, pkt) != get_length(pkt)) {
										cse4589_print_and_log("[%s:ERROR]\n", "REFRESH");
										cse4589_print_and_log("[%s:END]\n", "REFRESH");
										free_packet(pkt);
										free(outgoing_msg);
										continue;
									}

									free(outgoing_msg);
									free_packet(pkt);
									continue;
								}
							
								/* Processing RELOGIN command */
								if (strncmp(cmd, "RELOGIN", strlen("RELOGIN")) == 0) {
									/* Doing sanity check first */
									clientaddr_len = sizeof(struct sockaddr);

									if (getpeername(fd_index, (struct sockaddr *)&client_addr, &clientaddr_len) != 0) {
										fprintf(stderr, "Couldn't extract returning user's info from getpeername().\n");
									}

									char client_ip[INET_ADDRSTRLEN];
									if (inet_ntop(AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN) == 0) {
										fprintf(stderr, "Could not extract client IP\n");
									}

									unsigned short client_port;
									if ((client_port = ntohs(client_addr.sin_port)) == 0) {
										fprintf(stderr, "Could not extract client port\n");
									}

									char hostname[NI_MAXHOST];
									if (getnameinfo((const struct sockaddr *)&client_addr, sizeof(client_addr), hostname, NI_MAXHOST, NULL, 0, NI_NAMEREQD) != 0) {
										fprintf(stderr, "Could not resolve the host name from client's IP address\n");
									}

									ssize_t location = -1;
									location = old_user(users, client_ip, client_port, hostname);

									if (location != user_index) {
										fprintf(stderr, "User location from old_user() does not match the user_index extracted from the start of the else clause\n");
									}

									/* Setting the user as ONLINE */
									users[user_index]->status = ONLINE;

									/* Sending active users list to the client */
									size_t live_users = 0;
									for (int i = 0; i < MAX_USERS; i++) {
										if (users[i] != NULL && users[i]->status == ONLINE)
											live_users++;
									}

									int msg_len;
									char *outgoing_msg;
									if (!(msg_len = generate_users_string(&outgoing_msg, live_users, users))) {
										fprintf(stderr, "Something is wrong with generating live users string for returning client\n");
									}

									Packet *pkt = NULL;
									size_t pkt_len = make_packet("LOGIN", strlen("LOGIN"), outgoing_msg, strlen(outgoing_msg), &pkt);
									if (!pkt_len) {
										cse4589_print_and_log("[%s:ERROR]\n", "RELOGIN");
										cse4589_print_and_log("[%s:END]\n", "RELOGIN");
										free(outgoing_msg);
										continue;
									}

									if (send_packet(fd_index, pkt) != get_length(pkt)) {
										cse4589_print_and_log("[%s:ERROR]\n", "RELOGIN");
										cse4589_print_and_log("[%s:END]\n", "RELOGIN");
										free_packet(pkt);
										free(outgoing_msg);
										continue;
									}

									free(outgoing_msg);
									free_packet(pkt);

									/* Sending backlog messages if any to the client */
									if (!(msg_len = generate_backlog_string(&outgoing_msg, users[user_index]))) {
										continue;
									}

									pkt_len = make_packet("BACKLOG", strlen("BACKLOG"), outgoing_msg, strlen(outgoing_msg), &pkt);
									if (!pkt_len) {
										cse4589_print_and_log("[%s:ERROR]\n", "RELOGIN");
										cse4589_print_and_log("[%s:END]\n", "RELOGIN");
										free(outgoing_msg);
										continue;
									}

									if (send_packet(fd_index, pkt) != get_length(pkt)) {
										cse4589_print_and_log("[%s:ERROR]\n", "RELOGIN");
										cse4589_print_and_log("[%s:END]\n", "RELOGIN");
										free_packet(pkt);
										free(outgoing_msg);
										continue;
									} else {
										for (size_t i = 0; i < BACKLOG_BUFFER_SIZE; i++) {
											if (users[location] != NULL && users[location]->backlog_list[i] != NULL) {
												if (users[location]->backlog_list[i]->is_broadcast == 1) {
													cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
													cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", users[location]->backlog_list[i]->sender_ip, "255.255.255.255", users[location]->backlog_list[i]->message);
													cse4589_print_and_log("[%s:END]\n", "RELAYED");
												}
												else {
													cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
													cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", users[location]->backlog_list[i]->sender_ip, users[location]->ip , users[location]->backlog_list[i]->message);
													cse4589_print_and_log("[%s:END]\n", "RELAYED");
												}
											}
										}

										/* Clear the backlog messages list */
										size_t count = 0;
										if ((count = clear_blist(users[location])) != users[location]->backlog_count) {
											fprintf(stderr, "Unable to clear backlog messages after sending them to newly logged in client\n");
										}
										else {
											users[location]->backlog_count = 0;
										}
									}

									free(outgoing_msg);
									free_packet(pkt);
								}

								/* Processing LOGOUT command */
								if (strncmp(cmd, "LOGOUT", strlen("LOGOUT")) == 0) {
									/* Ensuring that there are no backlog messages to be sent before logging out the user */
									size_t count = clear_blist(users[user_index]);

									if (count != users[user_index]->backlog_count) {
										fprintf(stderr, "Error encountered when clearing the backlog list of logged out user\n");
									}

									users[user_index]->status = OFFLINE;
									continue;
								}

								/* Processing EXIT command */
								if (strncmp(cmd, "EXIT", strlen("EXIT")) == 0) {
									/* Ensuring that there are no backlog messages to be sent before logging out the user */
									size_t count = clear_blist(users[user_index]);

									if (count != users[user_index]->backlog_count) {
										fprintf(stderr, "Error encountered when clearing the backlog list of logged out user\n");
									}

									/* Clearing the blocked list of the user */
									for (int i = 0; i < MAX_LIVE_USERS; ++i) {
										if (users[user_index]->blocked_list[i] != NULL) {
											free(users[user_index]->blocked_list[i]);
											users[user_index]->blocked_list[i] = NULL;
										}
									}

									users[user_index]->status = OFFLINE;
									free_buffer(users[user_index]->cb);
									close(users[user_index]->fd);

									free(users[user_index]);
									users[user_index] = NULL;
									--user_count;

									/* Removing the socket from Select watch list */
									FD_CLR(fd_index, &master_list);
									continue;
								}

								/* Processing SEND command */
								if (strncmp(cmd, "SEND", strlen("SEND")) == 0) {

									char *rec_ip = NULL;
									char *user_msg = NULL;
									int user_msg_len;

									/* Extracting the IP address and message from the packet */
									if ((user_msg_len = parse_msg_string(msg, &rec_ip, &user_msg, TRUE)) == -1) {
										fprintf(stderr, "Unable to extract receiver's IP address and User message from the packet\n");
										free(rec_ip);
										free(user_msg);
										continue;
									}

									/* Find the user_index of the receiver in User's list */
									int rec_loc;
									for (rec_loc = 0; rec_loc < MAX_USERS; ++rec_loc) {
										if (users[rec_loc]) {
											if (!strncmp(users[rec_loc]->ip, rec_ip, INET_ADDRSTRLEN))
												break;
										}
									}

									if (rec_loc == MAX_USERS) {
										cse4589_print_and_log("[%s:ERROR]\n", "SEND");
										cse4589_print_and_log("[%s:END]\n", "SEND");
										free(rec_ip);
										free(user_msg);
										continue;
									}

									/* Check if the receiver has not blocked the sender */
									int is_blocked = FALSE;
									for (int i = 0; i < MAX_LIVE_USERS; ++i) {
										if (users[rec_loc]->blocked_list[i] != NULL) {
											if (!strncmp(users[rec_loc]->blocked_list[i], users[user_index]->ip, INET_ADDRSTRLEN)) {
												is_blocked = TRUE;
												break;
											}
										}
									}

									/* Send the message or add to receiver's backlog list while updating sender/receiver statistics */
									if (is_blocked) {
										users[user_index]->msg_sent++;
										free(rec_ip);
										free(user_msg);
										continue;
									} else {
										if (users[rec_loc]->status) {
											char *outgoing_msg;
											int outgoing_msg_len;
											if (!(outgoing_msg_len = generate_msg_string(&outgoing_msg, users[user_index]->ip, user_msg))) {
												fprintf(stderr, "Unable to generate outgoing message string\n");
												free(rec_ip);
												free(user_msg);
												continue;
											}

											Packet *pkt = NULL;
											size_t pkt_len = make_packet("SEND", strlen("SEND"), outgoing_msg, strlen(outgoing_msg), &pkt);
											if (!pkt_len) {
												cse4589_print_and_log("[%s:ERROR]\n", "SEND");
												cse4589_print_and_log("[%s:END]\n", "SEND");
												free(rec_ip);
												free(user_msg);
												free(outgoing_msg);
												continue;
											} else {
												if (send_packet(users[rec_loc]->fd, pkt) != get_length(pkt)) {
													cse4589_print_and_log("[%s:ERROR]\n", "SEND");
													cse4589_print_and_log("[%s:END]\n", "SEND");
													free(rec_ip);
													free(user_msg);
													free_packet(pkt);
													free(outgoing_msg);
													continue;
												}
												users[user_index]->msg_sent++;
												users[rec_loc]->msg_recv++;

												cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
												cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", users[user_index]->ip, rec_ip, user_msg);
												cse4589_print_and_log("[%s:END]\n", "RELAYED");

												free(outgoing_msg);
												free_packet(pkt);
											}
										} else {
											if (add_msg_blist(users[user_index]->ip, user_msg, users[rec_loc], 0)) {
												cse4589_print_and_log("[%s:ERROR]\n", "SEND");
												cse4589_print_and_log("[%s:END]\n", "SEND");
												free(user_msg);
												free(rec_ip);
												continue;
											} else {
												users[user_index]->msg_sent++;
												users[rec_loc]->msg_recv++;
											}
										}
									}

									free(user_msg);
									free(rec_ip);
									continue;
								}

								/* Processing BORADCAST command */
								if (strncmp(cmd, "BROADCAST", strlen("BROADCAST")) == 0) {
									char *user_msg = NULL;
									int user_msg_len;

									/* Extracting the IP address and message from the packet */
									if ((user_msg_len = parse_msg_string(msg, NULL, &user_msg, FALSE)) == -1) {
										fprintf(stderr, "Unable to extract User message from the packet\n");
										free(user_msg);
										continue;
									}

									int rec_loc;
									int live_count = 0;
									for (rec_loc = 0; rec_loc < MAX_USERS; ++rec_loc) {
										if (rec_loc == user_index || !users[rec_loc]) continue;
										/* Check if the receiver has not blocked the sender */
										int is_blocked = FALSE;
										for (int i = 0; i < MAX_LIVE_USERS; ++i) {
											if (users[rec_loc]->blocked_list[i] != NULL) {
												if (!strncmp(users[rec_loc]->blocked_list[i], users[user_index]->ip, INET_ADDRSTRLEN)) {
													is_blocked = TRUE;
													break;
												}
											}
										}

										/* Send the message or add to receiver's backlog list while updating sender/receiver statistics */
										if (is_blocked) {
											users[user_index]->msg_sent++;
											continue;
										} else {
											if (users[rec_loc]->status) {
												char *outgoing_msg;
												int outgoing_msg_len;
												if (!(outgoing_msg_len = generate_msg_string(&outgoing_msg, users[user_index]->ip, user_msg))) {
													fprintf(stderr, "Unable to generate outgoing message string\n");
													continue;
												}

												Packet *pkt = NULL;
												size_t pkt_len = make_packet("SEND", strlen("SEND"), outgoing_msg, strlen(outgoing_msg), &pkt);
												if (!pkt_len) {
													cse4589_print_and_log("[%s:ERROR]\n", "SEND");
													cse4589_print_and_log("[%s:END]\n", "SEND");
													free(outgoing_msg);
													continue;
												} else {
													if (send_packet(users[rec_loc]->fd, pkt) != get_length(pkt)) {
														cse4589_print_and_log("[%s:ERROR]\n", "SEND");
														cse4589_print_and_log("[%s:END]\n", "SEND");
														free_packet(pkt);
														free(outgoing_msg);
														continue;
													}
													users[user_index]->msg_sent++;
													users[rec_loc]->msg_recv++;
													++live_count;

													free(outgoing_msg);
													free_packet(pkt);
												}
											} else {
												if (add_msg_blist(users[user_index]->ip, user_msg, users[rec_loc], TRUE)) {
													cse4589_print_and_log("[%s:ERROR]\n", "SEND");
													cse4589_print_and_log("[%s:END]\n", "SEND");
													continue;
												} else {
													users[user_index]->msg_sent++;
													users[rec_loc]->msg_recv++;
												}
											}
										}
									}

									/* Print event message here if live_count is > 0 */
									if (live_count) {
										cse4589_print_and_log("[%s:SUCCESS]\n", "RELAYED");
										cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n", users[user_index]->ip, "255.255.255.255", user_msg);
										cse4589_print_and_log("[%s:END]\n", "RELAYED");
									}

									free(user_msg);
									continue;
								}

								/* Processing BLOCK command */
								if (strncmp(cmd, "BLOCK", strlen("BLOCK")) == 0) {
									/* Validate the IP address in the message */
									char *token;
									token = strtok(msg, " ");
									if (token != NULL) {
										unsigned char buf[sizeof(struct in_addr)];
										if (inet_pton(AF_INET, token, buf) == 0) {
											cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
											cse4589_print_and_log("[%s:END]\n", "BLOCK");
											continue;
										}
										
										if (block(token, users[user_index])) {
											cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
											cse4589_print_and_log("[%s:END]\n", "BLOCK");
											continue;
										}
									} else {
										cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
										cse4589_print_and_log("[%s:END]\n", "BLOCK");
										continue;
									}

									/* Validating that the user message contains only one IP address */
									if ((token = strtok(NULL, " ")) != NULL) {
										cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
										cse4589_print_and_log("[%s:END]\n", "BLOCK");
										continue;
									}
								}

								/* Processing UNBLOCK command */
								if (strncmp(cmd, "UNBLOCK", strlen("UNBLOCK")) == 0) {
									/* Validate the IP address in the message */
									char *token;
									token = strtok(msg, " ");
									if (token != NULL) {
										unsigned char buf[sizeof(struct in_addr)];
										if (inet_pton(AF_INET, token, buf) == 0) {
											cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
											cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
											continue;
										}

										if (unblock(token, users[user_index])) {
											cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
											cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
											continue;
										}
									} else {
										cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
										cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
										continue;
									}

									/* Validating that the user message contains only one IP address */
									if ((token = strtok(NULL, " ")) != NULL) {
										cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
										cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
										continue;
									}
								}
							}
						}

						// printf("Packets processed, freeing them now\n");
						for (int i = 0; i < num_pkts; ++i) {
							free_packet(pkt_arr[i]);
						}

						// printf("Freeing packet array\n");
						free(pkt_arr);
					}
				}
			}
		}
	}

	close(server_socket);
	return EXIT_SUCCESS;
}
