#include "logger.h"
#include "global.h"

// #include <ctype.h>

int is_valid_ascii(const char *str);

/**
 * * @section 
 * * 
 * * CLIENT function:
 * * This is the main function that drives the client application.
 * *
 * * Roles:
 * * Initiates connection to the remote server
 * * Process user commands
 * * Exchanges packets with the server using a pre-defined protocol
 * *
 * * @param  port Client's listening socket port number
 * * @return 0 EXIT_SUCCESS
 * */
int client(const char *port) {
	struct user *users[MAX_USERS] = {NULL};
	ssize_t user_count = 0;
	int client_socket, head_fd, selret, fd_index;
	fd_set master_list, watch_list;
	char *blocked_ips[MAX_LIVE_USERS] = {NULL};
	char host_ip[INET_ADDRSTRLEN] = {'\0'};
	get_self_ip(host_ip);
	char *remote_ip;
	short remote_port;

	CircularBuffer *cb;

reset:
	remote_ip = NULL;
	cb = NULL;
	client_socket = socket(AF_INET, SOCK_STREAM, 0); 
	if (client_socket < 0) {
        	fprintf(stderr, "Failed to create socket");
		exit(EXIT_FAILURE);
	}

	/* Setting the port number to be reusable immediately */
	int reuse = TRUE;
	setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

	/* Binding the client socket to the port */
	struct sockaddr_in client_addr;
	// socklen_t caddr_len;
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = INADDR_ANY;
	client_addr.sin_port = htons(atoi(port));
	if (bind(client_socket, (struct sockaddr *)&client_addr, sizeof(client_addr)) != 0) {
		fprintf(stderr, "Error in binding client port");
		exit(EXIT_FAILURE);
	}

	int logged_in = FALSE;
	int login_count = 0;

	/* Initializing the socket lists to be monitored by select call */
	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);

	/* Registering client_socket and STDIN to be monitored by select */
	FD_SET(STDIN_FILENO, &master_list);

	head_fd = STDIN_FILENO;

	while (TRUE) {
		memcpy(&watch_list, &master_list, sizeof(master_list));

		if (logged_in)
			head_fd = client_socket;
		else
			head_fd = STDIN_FILENO;

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
						if(user_input[strlen(user_input) - 1] == '\n') {
							user_input[strlen(user_input) - 1] = '\0';
						}

						/* Processing AUTHOR command */
						if (strncmp(user_input, "AUTHOR", strlen("AUTHOR")) == 0 && (strlen(user_input) == strlen("AUTHOR")))
							print_author();

						/* Processing IP command */
						if (strncmp(user_input, "IP", strlen("IP")) == 0 && (strlen(user_input) == strlen("IP")))
							print_ip();

						/* Processing PORT command */
						if (strncmp(user_input, "PORT", strlen("PORT")) == 0 && (strlen(user_input) == strlen("PORT")))
							print_port(client_socket);

						/* Processing LOGIN command */
						if (strncmp(user_input, "LOGIN", strlen("LOGIN")) == 0) {
							if (logged_in) {
								fprintf(stderr, "%d\n", 119);
								cse4589_print_and_log("[%s: ERROR]\n", "LOGIN");
								cse4589_print_and_log("[%s: END]\n", "LOGIN");
								free(user_input);
								continue;
							}

							char *token;
							char *s_ip;
							long s_port;
							
							/* Tokenizing the user input to extract server's IP address and port number */
							token = strtok(user_input, " ");
							if (token != NULL) {
								if (!strncmp(token, "LOGIN", strlen("LOGIN")) && strlen(token) != strlen("LOGIN")) {
									fprintf(stderr, "%d\n", 134);
									cse4589_print_and_log("[%s: ERROR]\n", "LOGIN");
									cse4589_print_and_log("[%s: END]\n", "LOGIN");
									free(user_input);
									continue;
								}

								s_ip = strtok(NULL, " ");
								if (s_ip != NULL) {
									token = strtok(NULL, " ");
									if (token != NULL) {
										char *endptr = NULL;
										// printf ("%s", token);
										s_port = strtol(token, &endptr, 10);
										if (endptr != NULL && (*endptr != 0)) {
											cse4589_print_and_log("[%s: ERROR]\n", "LOGIN");
											cse4589_print_and_log("[%s: END]\n", "LOGIN");
											free(user_input);
											continue;
										}
									} else {
										fprintf(stderr, "%d\n", 155);
										cse4589_print_and_log("[%s: ERROR]\n", "LOGIN");
										cse4589_print_and_log("[%s: END]\n", "LOGIN");
										free(user_input);
										continue;
									}
								} else {
									fprintf(stderr, "%d\n", 162);
									cse4589_print_and_log("[%s: ERROR]\n", "LOGIN");
									cse4589_print_and_log("[%s: END]\n", "LOGIN");
									free(user_input);
									continue;
								}
							}

							/* Validating Port Number and IP address from the input */
							unsigned char buf[sizeof(struct in_addr)];
						    	if (inet_pton(AF_INET, s_ip, buf) == 0) {
									fprintf(stderr, "%d\n", 173);
									cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
									cse4589_print_and_log("[%s:END]\n", "LOGIN");
									free(user_input);
									continue;
						    	}

							if (s_port > 65535 || s_port < 1024) {
								fprintf(stderr, "%d\n", 181);
								cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
								cse4589_print_and_log("[%s:END]\n", "LOGIN");
								free(user_input);
								continue;
							}

							if (!login_count) {
								/* The client is connecting to the server for the first time */
								struct sockaddr_in serv_addr;
								memset(&serv_addr, 0, sizeof(serv_addr));
								serv_addr.sin_family = AF_INET;
								inet_pton(AF_INET, s_ip, &serv_addr.sin_addr);
								serv_addr.sin_port = htons((short)s_port);
								
								cb = create_buffer();
								if (connect(client_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) {
									fprintf(stderr, "Unable to connect to the remote server represented by file descriptor %d\n", client_socket);
									free(user_input);
									free_buffer(cb);
									continue;
								}

								int flags = fcntl(client_socket, F_GETFL, 0);
								if (flags < 0) {
									fprintf(stderr, "fcntl failed. Try logging in later.\n");
									free(user_input);
									free_buffer(cb);
									continue;
								}

								flags |= O_NONBLOCK;
								if (fcntl(client_socket, F_SETFL, flags) < 0) {
									fprintf(stderr, "Unable to set flags on client_socket. Try again later.\n");
									free(user_input);
									free_buffer(cb);
									continue;
								}

								FD_SET(client_socket, &master_list);
								// if (client_socket > head_fd) {
								// 	head_fd = client_socket;
								// }

								logged_in = TRUE;
								++login_count;
								remote_ip = (char *)malloc(INET_ADDRSTRLEN);
								memset(remote_ip, 0, INET_ADDRSTRLEN);
								strncpy(remote_ip, s_ip, INET_ADDRSTRLEN);
								remote_port = (short)s_port;

								free(user_input);
								continue;
							} else {
								if (strcmp(s_ip, remote_ip) && (short)s_port != remote_port) {
									fprintf(stderr, "%d\n", 217);
									cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
									cse4589_print_and_log("[%s:END]\n", "LOGIN");
									free(user_input);
									continue;
								}

								Packet *pkt = NULL;
								size_t pkt_len = make_packet("RELOGIN", strlen("RELOGIN"), NULL, 0, &pkt);

								if (!pkt_len) {
									fprintf(stderr, "Unable to create a RELOGIN packet\n");
									free(user_input);
									continue;
								}

								if (send_packet(client_socket, pkt) != get_length(pkt)) {
									fprintf(stderr, "%d\n", 235);
									cse4589_print_and_log("[%s:ERROR]\n", "LOGIN");
									cse4589_print_and_log("[%s:END]\n", "LOGIN");
									free_packet(pkt);
									free(user_input);
									continue;	
								}

								int flags = fcntl(client_socket, F_GETFL, 0);
								if (flags < 0) {
									fprintf(stderr, "fcntl failed. Try logging in later.\n");
									free_packet(pkt);
									free(user_input);
									continue;
								}

								flags |= O_NONBLOCK;
								if (fcntl(client_socket, F_SETFL, flags) < 0) {
									fprintf(stderr, "Unable to set flags on client_socket\n");
									free_packet(pkt);
									free(user_input);
									continue;
								}

								FD_SET(client_socket, &master_list);
								// if (client_socket > head_fd) {
								// 	head_fd = client_socket;
								// }

								logged_in = TRUE;
								++login_count;
								free_packet(pkt);
								free(user_input);
								continue;
							}
						}

						/* Processing LIST command */
						if (strncmp(user_input, "LIST", strlen("LIST")) == 0 && logged_in == TRUE) {
                            // printf("user count is %d\n", user_count);
                            list_users(users, user_count);
                        }

						/* Processing REFRESH command */
						if (strncmp(user_input, "REFRESH", strlen("REFRESH")) == 0 && logged_in == TRUE) {
							Packet *pkt = NULL;
							size_t pkt_len = make_packet("REFRESH", strlen("REFRESH"), NULL, 0, &pkt);

							if (!pkt_len) {
								fprintf(stderr, "Unable to create a REFRESH packet\n");
								free(user_input);
								continue;
							}

							if (send_packet(client_socket, pkt) != get_length(pkt)) {
								fprintf(stderr, "%d\n", 235);
								cse4589_print_and_log("[%s:ERROR]\n", "REFRESH");
								cse4589_print_and_log("[%s:END]\n", "REFRESH");
								free_packet(pkt);
								free(user_input);
								continue;	
							}
						}
						
						/* Processing SEND command */
						if (strncmp(user_input, "SEND", strlen("SEND")) == 0 && logged_in == TRUE) {
							char *token;
							char *s_ip;
							char *msg_to_send;
						
							/* Get the first token ("SEND") */
							token = strtok(user_input, " ");
							if (token != NULL) {
								if (strcmp(token, "SEND") != 0) {
									cse4589_print_and_log("[%s:ERROR]\n", "SEND");
									cse4589_print_and_log("[%s:END]\n", "SEND");
									free(user_input);
									continue;
								}
								/* Get the IP address */
								s_ip = strtok(NULL, " ");
								if (s_ip != NULL) {
									/* Ensure it is valid IP and not the host's own IP address */
									unsigned char buf[sizeof(struct in_addr)];
									if (inet_pton(AF_INET, s_ip, buf) == 0) {
										cse4589_print_and_log("[%s:ERROR]\n", "SEND");
										cse4589_print_and_log("[%s:END]\n", "SEND");
										free(user_input);
										continue;
									}
						
									if (strcmp(host_ip, s_ip) == 0) {
										cse4589_print_and_log("[%s:ERROR]\n", "SEND");
										cse4589_print_and_log("[%s:END]\n", "SEND");
										free(user_input);
										continue;
									}
						
									/* Ensure that IP address is already present in the client's Users list */
									long int rec_loc = -1;
									// Getting the receiver's location of user in users
									for (size_t i = 0; i < MAX_USERS; i++) {
										if (users[i] != NULL && strncmp(users[i]->ip, s_ip, INET_ADDRSTRLEN) == 0) {
											rec_loc = i;
											break;
										}
									}
						
									if (rec_loc != -1) {
										msg_to_send = strtok(NULL, "");
										if (msg_to_send != NULL) {
											
											/* Ensuring that user message is valid ascii text and no more than 256 characters long */
											if (strlen(msg_to_send) <= 256 && is_valid_ascii(msg_to_send)) {
												int msg_len;
												char *msg;
												if (!(msg_len = generate_msg_string(&msg, s_ip, msg_to_send))) {
													fprintf(stderr, "Something is wrong with generating message string to SEND\n");
												}
												
												Packet *pkt = NULL;
												size_t pkt_len = make_packet("SEND", strlen("SEND"), msg, strlen(msg), &pkt);
						
												if (!pkt_len) {
													cse4589_print_and_log("[%s:ERROR]\n", "SEND");
													cse4589_print_and_log("[%s:END]\n", "SEND");
													free(msg);
													free(user_input);
													continue;
												}
						
												if (send_packet(client_socket, pkt) != get_length(pkt)) {
													cse4589_print_and_log("[%s:ERROR]\n", "SEND");
													cse4589_print_and_log("[%s:END]\n", "SEND");
													free(msg);
													free_packet(pkt);
													free(user_input);
													continue;
												} else {
													cse4589_print_and_log("[%s:SUCCESS]\n", "SEND");
													cse4589_print_and_log("[%s:END]\n", "SEND");
												}
						
												free(msg);
												free_packet(pkt);
												free(user_input);
												continue;
											} else { /* Input message does not meet the requirements */
												cse4589_print_and_log("[%s:ERROR]\n", "SEND");
												cse4589_print_and_log("[%s:END]\n", "SEND");
												free(user_input);
												continue;
											}
										} else {
											cse4589_print_and_log("[%s:ERROR]\n", "SEND");
											cse4589_print_and_log("[%s:END]\n", "SEND");
											free(user_input);
											continue;
										}
									} else { /* Input IP is not in the list of active users */
										cse4589_print_and_log("[%s:ERROR]\n", "SEND");
										cse4589_print_and_log("[%s:END]\n", "SEND");
										free(user_input);
										continue;
									}
								}
							}
						}
						
						/* Processing BROADCAST command */
						if (strncmp(user_input, "BROADCAST", strlen("BROADCAST")) == 0 && logged_in == TRUE) {
							char *token;
                            char *msg_to_broadcast;

							token = strtok(user_input, " ");
							if (token != NULL) {
								if (strcmp(token, "BROADCAST") != 0) {
									cse4589_print_and_log("[%s:ERROR]\n", "SEND");
                                    cse4589_print_and_log("[%s:END]\n", "SEND");
									free(user_input);
									continue;
								}

								msg_to_broadcast = strtok(NULL, "");
								if (msg_to_broadcast != NULL) {
									if (strlen(msg_to_broadcast) <= 256 && is_valid_ascii(msg_to_broadcast)) {
										Packet *pkt = NULL;
										size_t pkt_len = make_packet("BROADCAST", strlen("BROADCAST"), msg_to_broadcast, strlen(msg_to_broadcast), &pkt);

										if (!pkt_len) {
											fprintf(stderr, "Unable to create a BROADCAST packet\n");
											free(user_input);
											continue;
										}

										if (send_packet(client_socket, pkt) != get_length(pkt)) {
											cse4589_print_and_log("[%s:ERROR]\n", "BROADCAST");
											cse4589_print_and_log("[%s:END]\n", "BROADCAST");
											free_packet(pkt);
											free(user_input);
											continue;	
										} else {
											cse4589_print_and_log("[%s:SUCCESS]\n", "BROADCAST");
											cse4589_print_and_log("[%s:END]\n", "BROADCAST");
										}

										free_packet(pkt);
										free(user_input);
										continue;
									} else {
										cse4589_print_and_log("[%s:ERROR]\n", "BROADCAST");
										cse4589_print_and_log("[%s:END]\n", "BROADCAST");
										free(user_input);
										continue;
									}
								} else { /* It is an error to BROADCAST empty message */
									cse4589_print_and_log("[%s:ERROR]\n", "BROADCAST");
									cse4589_print_and_log("[%s:END]\n", "BROADCAST");
									free(user_input);
									continue;
								}
							}

							free(user_input);
							continue;
						}
						
						/* Processing BLOCK command */
						if ((strncmp(user_input, "BLOCK", strlen("BLOCK")) == 0) && logged_in == TRUE) {
							char *token;
                            char *client_ip;

							token = strtok(user_input, " ");
                            if (token != NULL && strcmp(token, "BLOCK") == 0) {
								client_ip = strtok(NULL, " ");
                                if (client_ip != NULL) {
									unsigned char buf[sizeof(struct in_addr)];
                                    if (inet_pton(AF_INET, client_ip, buf) != 1) {
                                        cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
                                        cse4589_print_and_log("[%s:END]\n", "BLOCK");
                                        free(user_input);
										continue;
                                    }
                                    else if (strcmp(host_ip, client_ip) == 0)
                                    {
                                        cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
                                        cse4589_print_and_log("[%s:END]\n", "BLOCK");
                                        free(user_input);
										continue;
                                    }

									int user_exists = -1;
									int already_blocked = -1;

									for (int i = 0; i < MAX_USERS; i++) {
										if (users[i] != NULL && strcmp(users[i]->ip, client_ip) == 0) {
											user_exists = 1;
											break;
										}
									}

									for (int i = 0; i < MAX_LIVE_USERS; i++) {
										if (blocked_ips[i] != NULL && strcmp(blocked_ips[i], client_ip) == 0) {
											already_blocked = 1;
											break;
										}
									}

									if (user_exists == 1 && already_blocked == -1) {
										Packet *pkt = NULL;
										size_t pkt_len = make_packet("BLOCK", strlen("BLOCK"), client_ip, strlen(client_ip), &pkt);

										if (!pkt_len) {
											fprintf(stderr, "Unable to create a BLOCK packet\n");
											free(user_input);
											continue;
										}

										if (send_packet(client_socket, pkt) != get_length(pkt)) {
											cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
											cse4589_print_and_log("[%s:END]\n", "BLOCK");
											free(user_input);
											free_packet(pkt);
											continue;
										} else {
											for (int i = 0; i < MAX_LIVE_USERS; i++) {
												if (blocked_ips[i] == NULL) {
													blocked_ips[i] = (char *)malloc(sizeof(char) * INET_ADDRSTRLEN);
													memset(blocked_ips[i], '\0', sizeof(char) * INET_ADDRSTRLEN);
													strcpy(blocked_ips[i], client_ip);
													break;
												}
											}
											cse4589_print_and_log("[%s:SUCCESS]\n", "BLOCK");
											cse4589_print_and_log("[%s:END]\n", "BLOCK");
										}

										free_packet(pkt);
									} else {
										cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
                                        cse4589_print_and_log("[%s:END]\n", "BLOCK");
                                        free(user_input);
										continue;
									}
								} else {
									cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
                                    cse4589_print_and_log("[%s:END]\n", "BLOCK");
									free(user_input);
									continue;
								}

								/* Need to ensure that there is only one IP in the input */
								token = strtok(NULL, " ");
								if (token) {
									cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
                                    cse4589_print_and_log("[%s:END]\n", "BLOCK");
									free(user_input);
									continue;
								}
							}
						}

						/* Processing UNBLOCK command */
						if ((strncmp(user_input, "UNBLOCK", strlen("UNBLOCK")) == 0) && logged_in == TRUE) {
							char *token;
                            char *client_ip;

							token = strtok(user_input, " ");
                            if (token != NULL && strcmp(token, "UNBLOCK") == 0) {
								client_ip = strtok(NULL, " ");
                                if (client_ip != NULL) {
									unsigned char buf[sizeof(struct in_addr)];
                                    if (inet_pton(AF_INET, client_ip, buf) != 1) {
                                        cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
                                        cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
                                        free(user_input);
										continue;
                                    }
                                    else if (strcmp(host_ip, client_ip) == 0)
                                    {
                                        cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
                                        cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
                                        free(user_input);
										continue;
                                    }

									int user_exists = -1;
									int already_blocked = -1;

									for (int i = 0; i < MAX_USERS; i++) {
										if (users[i] != NULL && strcmp(users[i]->ip, client_ip) == 0) {
											user_exists = 1;
											break;
										}
									}

									for (int i = 0; i < MAX_LIVE_USERS; i++) {
										if (blocked_ips[i] != NULL && strcmp(blocked_ips[i], client_ip) == 0) {
											already_blocked = 1;
											break;
										}
									}

									if (user_exists == 1 && already_blocked == 1) {
										Packet *pkt = NULL;
										size_t pkt_len = make_packet("UNBLOCK", strlen("UNBLOCK"), client_ip, strlen(client_ip), &pkt);

										if (!pkt_len) {
											fprintf(stderr, "Unable to create a UNBLOCK packet\n");
											free(user_input);
											continue;
										}

										if (send_packet(client_socket, pkt) != get_length(pkt)) {
											cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
											cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
											free(user_input);
											free_packet(pkt);
											continue;
										} else {
											for (int i = 0; i < MAX_LIVE_USERS; i++) {
												if (blocked_ips[i] != NULL && (strncmp(blocked_ips[i], client_ip, INET_ADDRSTRLEN) == 0)) {
													free(blocked_ips[i]);
													blocked_ips[i] = NULL;
													break;
												}
											}
											cse4589_print_and_log("[%s:SUCCESS]\n", "UNBLOCK");
											cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
										}

										free_packet(pkt);
									} else {
										cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
                                        cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
                                        free(user_input);
										continue;
									}
								} else {
									cse4589_print_and_log("[%s:ERROR]\n", "UNBLOCK");
                                    cse4589_print_and_log("[%s:END]\n", "UNBLOCK");
									free(user_input);
									continue;
								}

								/* Need to ensure that there is only one IP in the input */
								token = strtok(NULL, " ");
								if (token) {
									cse4589_print_and_log("[%s:ERROR]\n", "BLOCK");
                                    cse4589_print_and_log("[%s:END]\n", "BLOCK");
									free(user_input);
									continue;
								}
							}
						}

						/* Processing LOGOUT command */
						if ((strncmp(user_input, "LOGOUT", strlen("LOGOUT")) == 0) && logged_in == TRUE) {
							char *token = strtok(user_input, " ");

							if (token != NULL) {
								if (strcmp(token, "LOGOUT") != 0) {
									cse4589_print_and_log("[%s:ERROR]\n", "LOGOUT");
									cse4589_print_and_log("[%s:END]\n", "LOGOUT");
									free(user_input);
									continue;
								}
							}

							token = strtok(NULL, " ");
							if (token != NULL) {
								cse4589_print_and_log("[%s:ERROR]\n", "LOGOUT");
								cse4589_print_and_log("[%s:END]\n", "LOGOUT");
								free(user_input);
								continue;
							}

							Packet *pkt = NULL;
							size_t pkt_len = make_packet("LOGOUT", strlen("LOGOUT"), NULL, 0, &pkt);

							if (!pkt_len) {
								fprintf(stderr, "Unable to create a LOGOUT packet\n");
								free(user_input);
								continue;
							}

							if (send_packet(client_socket, pkt) != get_length(pkt)) {
								fprintf(stderr, "%d\n", 235);
								cse4589_print_and_log("[%s:ERROR]\n", "LOGOUT");
								cse4589_print_and_log("[%s:END]\n", "LOGOUT");
								free_packet(pkt);
								free(user_input);
								continue;	
							}

							logged_in = FALSE;
							free_packet(pkt);
							cse4589_print_and_log("[%s:SUCCESS]\n", "LOGOUT");
							cse4589_print_and_log("[%s:END]\n", "LOGOUT");
							free(user_input);
							continue;
							
						}

						/* Processing EXIT command */
						if ((strncmp(user_input, "EXIT", strlen("EXIT")) == 0)) {
							/* Client is not connected to the server */
							if (logged_in == 0 && login_count == 0)  { /* Client has never connected to a server */
                                close(client_socket);

                                cse4589_print_and_log("[%s:SUCCESS]\n", "EXIT");
                                cse4589_print_and_log("[%s:END]\n", "EXIT");
								for (int i = 0; i < MAX_LIVE_USERS; i++) {
									if (blocked_ips[i] != NULL) {
										free(blocked_ips[i]);
										blocked_ips[i] = NULL;
									}
								}

                                free(user_input);
								free_buffer(cb);
								free(remote_ip);
								close(client_socket);
								exit(EXIT_SUCCESS);
                            } else if (logged_in == 0 && login_count > 0) { /* Client has previously logged in but is currently logged out */
								Packet *pkt = NULL;
								size_t pkt_len = make_packet("EXIT", strlen("EXIT"), NULL, 0, &pkt);

								if (!pkt_len) {
									fprintf(stderr, "Unable to create a EXIT packet\n");
									free(user_input);
									continue;
								}

								if (send_packet(client_socket, pkt) != get_length(pkt)) {
									fprintf(stderr, "%d\n", 235);
									cse4589_print_and_log("[%s:ERROR]\n", "EXIT");
									cse4589_print_and_log("[%s:END]\n", "EXIT");
									free_packet(pkt);
									free(user_input);
									continue;	
								}

								free(pkt);
								free(user_input);
								cse4589_print_and_log("[%s:SUCCESS]\n", "EXIT");
                                cse4589_print_and_log("[%s:END]\n", "EXIT");
								for (int i = 0; i < MAX_USERS; i++) {
									if (users[i] != NULL) {
										free(users[i]);
										users[i] = NULL;
									}
								}
								for (int i = 0; i < MAX_LIVE_USERS; i++) {
									if (blocked_ips[i] != NULL) {
										free(blocked_ips[i]);
										blocked_ips[i] = NULL;
									}
								}

								free_buffer(cb);
								free(remote_ip);
								close(client_socket);
								return(EXIT_SUCCESS);
							} else { /* Client is currently logged in */
								Packet *pkt = NULL;
								size_t pkt_len = make_packet("EXIT", strlen("EXIT"), NULL, 0, &pkt);

								if (!pkt_len) {
									fprintf(stderr, "Unable to create a EXIT packet\n");
									free(user_input);
									continue;
								}

								if (send_packet(client_socket, pkt) != get_length(pkt)) {
									fprintf(stderr, "%d\n", 235);
									cse4589_print_and_log("[%s:ERROR]\n", "EXIT");
									cse4589_print_and_log("[%s:END]\n", "EXIT");
									free_packet(pkt);
									free(user_input);
									continue;	
								}

								free(pkt);
								free(user_input);
								cse4589_print_and_log("[%s:SUCCESS]\n", "EXIT");
                                cse4589_print_and_log("[%s:END]\n", "EXIT");
								for (int i = 0; i < MAX_USERS; i++) {
									if (users[i] != NULL) {
										free(users[i]);
										users[i] = NULL;
									}
								}
								for (int i = 0; i < MAX_LIVE_USERS; i++) {
									if (blocked_ips[i] != NULL) {
										free(blocked_ips[i]);
										blocked_ips[i] = NULL;
									}
								}

								free_buffer(cb);
								free(remote_ip);
								close(client_socket);
								return(EXIT_SUCCESS);
							}
						}
						
						free(user_input);

					} else if (fd_index == client_socket) { /* Received data from the server to process */
						Packet **pkt_arr = NULL;

						ssize_t num_pkts = receive_packets(client_socket, &pkt_arr, cb);
						if ((!num_pkts && !pkt_arr) || (num_pkts == -1 && !pkt_arr)) { /* Unexpected System Error */
							continue;
						} else if (num_pkts == -2 && !pkt_arr) { /* Dealing with unexpected server shutdown */
							for (int i = 0; i < MAX_USERS; i++) {
								if (users[i] != NULL) {
									free(users[i]);
									users[i] = NULL;
								}
							}
							for (int i = 0; i < MAX_LIVE_USERS; i++) {
								if (blocked_ips[i] != NULL) {
									free(blocked_ips[i]);
									blocked_ips[i] = NULL;
								}
							}

							free_buffer(cb);
							free(remote_ip);
							/* Clear client socket from watch list */
							FD_CLR(client_socket, &master_list);
							logged_in = FALSE;
							login_count = 0;
							close(client_socket);
							goto reset;
						} else { /* Processing received packets */
							// printf("Received %d packets\n", num_pkts);
							for (int i = 0; i < num_pkts; i++) {
								char *cmd, *msg;
								ssize_t cmd_len = get_command(pkt_arr[i], &cmd);
								if (cmd_len == -1) {
									cse4589_print_and_log("[%s:ERROR]\n", cmd);
								}
								ssize_t msg_len = get_message(pkt_arr[i], &msg);

								if (!strncmp(cmd, "LOGIN", strlen("LOGIN")) || !strncmp(cmd, "REFRESH", strlen("REFRESH"))) {
									user_count = parse_users_string(msg, msg_len, users);
									if (user_count != -1) {
										cse4589_print_and_log("[%s:SUCCESS]\n", cmd);
                                    	cse4589_print_and_log("[%s:END]\n", cmd);
									} else {
										fprintf(stderr, "%d\n", 316);
										cse4589_print_and_log("[%s:ERROR]\n", cmd);
                                    	cse4589_print_and_log("[%s:END]\n", cmd);
									}
								} else if (!strncmp(cmd, "BACKLOG", strlen("BACKLOG"))) {
									parse_backlog_string (msg);
								} else if (!strncmp(cmd, "SEND", strlen("SEND"))) {
									const char delimiter[2] = {30, 0};

									char *token = strtok(msg, delimiter);
                                    char *s_ip = NULL;
                                    char *msg_to_send = NULL;

                                    if (token != NULL) {
                                        s_ip = token;
                                        token = strtok(NULL, "");
                                        if (token != NULL) {
                                            msg_to_send = token;
                                        }
                                    }
                                    if (s_ip && msg_to_send) {
                                        cse4589_print_and_log("[%s:SUCCESS]\n", "RECEIVED");
                                        cse4589_print_and_log("msg from:%s\n[msg]:%s\n", s_ip, msg_to_send);
                                        cse4589_print_and_log("[%s:END]\n", "RECEIVED");
                                    }
                                    else {
                                        fprintf(stderr, "Failed to parse IP or message.\n");
                                    }
								}
							}
						}

						for (int i = 0; i < num_pkts; ++i) {
							free_packet(pkt_arr[i]);
						}

						free(pkt_arr);
					}
				}
			}
		}
	}

	free_buffer(cb);
	free(remote_ip);
	close(client_socket);
	exit(EXIT_SUCCESS);
}

int is_valid_ascii(const char *str) {
    while (*str) {
        if ((unsigned char)*str > 127) {
            return FALSE;
        }
        str++;
    }
    return TRUE;
}
