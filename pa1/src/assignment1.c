/**
 * @assignment1
 * @author  Naveen Veeravalli (nveerava)
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * This contains the main function. Add further description here....
 */

#include "global.h"
#include "logger.h"

int server(const char *port);
int client(const char *port);

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
	/*Init. Logger*/
	cse4589_init_log(argv[2]);

	/*Clear LOGFILE*/
	fclose(fopen(LOGFILE, "w"));

	/*Start Here*/
	if (argc != 3) {
		fprintf(stderr, "Usage ./assignment1 <mode> <port_number>\n"
			"Acceptable modes:\n"
			"s - to run the application as a server\n"
			"c - to run the application as a client\n");
		exit(EXIT_FAILURE);	
	}

	const char *mode[2] = {"s", "c"};

	if ((strncmp(argv[1], mode[0], strlen(argv[1])) != 0) && (strncmp(argv[1], mode[1], strlen(argv[1])) != 0)) {
		fprintf(stderr, "Only acceptable modes are:\n"
			"s - to run the application as a server\n"
			"c - to run the application as a client\n");
		exit(EXIT_FAILURE);
	}
	
	int  port = atoi(argv[2]);
	
	if ((port > 65535) || (port < 1024)) {
		fprintf(stderr, "Port should be between 1024 and 65535\n");
		exit(EXIT_FAILURE);
	}
	

	switch (argv[1][0]) {
		case 's':
			if (server(argv[2])) {
				fprintf(stderr, "Server could not exit properly\n");
			}
			break;	
		case 'c':
			if (client(argv[2])) {
				fprintf(stderr, "Client could not exit properly!\n");
			}
			break;
	}
	
	return EXIT_SUCCESS;
}
