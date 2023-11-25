/*
Name: Stephen Hogeman
CaseID: ssh115
Filename: proj3.c
Date: 10/05/2023
Description: This source file is the main file for project 3.
When run, it accepts the -n, -d, and -a arguments to open the server.
On the client end, it accepts GET and SHUTDOWN requests, and sends adequate HTTP responses.
*/

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <strings.h>
# include <netdb.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <sys/types.h>

# define ERROR 1
# define SUCCESS 0
# define PROTOCOL "tcp"
# define QLEN 1
# define REQLEN 1024
# define BUFLEN 1024
# define RESPLEN 1024
# define MALFORM_REQUEST 400
# define UNSUPPORTED_PROTOCOL 501
# define UNSUPPORTED_METHOD 405
# define ACCEPTED_REQUEST 200
# define FILE_NOT_FOUND 404
# define INVALID_FILENAME 406
# define FORBIDDEN_OPERATION 403
# define GET "GET"
# define SHUTDOWN "SHUTDOWN"
# define VALID_PROTOCOL "HTTP/"
# define PROTOCOL_CASE_CNT 5
# define SET_FLAG 1
# define REQ_SPACES 3

char *port;
char *root_directory;
char *auth_token;
char *argument;
char response[RESPLEN];
char request[REQLEN];
char buffer[BUFLEN];
char response_template[] = "HTTP/1.1 %d %s\r\n\r\n";
int sd, sd2;
int n_flag, d_flag, a_flag;

/* Handles basic errors and exits the program */
int errExit (char *format, char *arg)
{
	fprintf (stderr, format, arg);
	fprintf (stderr,"\n");
	exit (ERROR);
}

/* Parses command line arguments */
void parseArgs (int argc, char *argv [])
{
	int opt;
	extern char *optarg;

	while ((opt = getopt (argc, argv, "n:d:a:")) != -1)
	{
		switch (opt)
	 	{
			case 'n':
				port = optarg;
				n_flag += SET_FLAG;
				break;
			case 'd':
				root_directory = optarg;
				d_flag += SET_FLAG;
				break;
			case 'a':
				auth_token = optarg;
				a_flag += SET_FLAG;
				break;
			case '?':
				errExit ("Error: invalid argument passed", NULL);
				break;
		}
	}
}

/* Evaluates whether all flags -a -n -d are present */
void mandatoryFlags()
{
	if (n_flag != SET_FLAG)
	{
		errExit ("Error: mandatory -n flag not set", NULL);
	}

	else if (d_flag != SET_FLAG)
	{
		errExit ("Error: mandatory -d flag not set", NULL);
	}

	else if (a_flag != SET_FLAG)
	{
		errExit ("Error: mandatory -a flag not set", NULL);
	}
}

/* Handles errors with HTTP requests, closes sockets, and exits the program */
void errResponse (int response_code, char *response_msg)
{
	int len = snprintf (response, sizeof (response), response_template, response_code, response_msg);
	write (sd2, response, len);
	close (sd2);
}

/* Handles successful HTTP requests for GET and SHUTDOWN */
void successResponse (int response_code, char *response_msg)
{
	int len = snprintf (response, sizeof (response), response_template, response_code, response_msg);
	write (sd2, response, len);
}

/* Handles GET Requests */
void getRequest()
{
	FILE *file;
	char *request_str;

	/* Evaluates whether the first character of the input file is / */
	if (argument[0] != '/')
	{
		errResponse (INVALID_FILENAME, "Invalid Filename");
	}

	if (strcmp (argument, "/") == 0)
	{
		argument = "/homepage.html";
	}

	request_str = (char *)malloc (strlen (root_directory) + strlen (argument) + 1);
	if (request_str != NULL)
	{
		strcpy (request_str, root_directory);
		strcat (request_str, argument);
	}

	else
	{
		errExit ("Error: concatenating argument and directory", NULL);
	}
	file = fopen (request_str, "r");

	/* Evaluate whether the input file actually exists on the server */
	if (file == NULL)
	{
		errResponse (FILE_NOT_FOUND, "File Not Found");
		close (sd2);
	}

	else
	{
		successResponse (ACCEPTED_REQUEST, "OK");
		size_t bytesRead;
		while ((bytesRead = fread (buffer, 1, sizeof (buffer), file)) > 0)
		{
			write (sd2, buffer, bytesRead);
		}
		fclose (file);
		close (sd2);
	}
	free (request_str);
}

/* Handles SHUTDOWN Requests */
void shutdownRequest ()
{
	if (strcmp (argument, auth_token) == 0)
	{
		successResponse (ACCEPTED_REQUEST, "Server Shutting Down");
		close (sd2);
		close (sd);
		exit (SUCCESS);
	}

	else
	{
		errResponse (FORBIDDEN_OPERATION, "Operation Forbidden");
	}
}

/* Parses HTTP Request */
void parseRequest()
{
	char *method;
	char *version;
	char *token = strtok (request, " ");
	int num_spaces = 0;
	while (num_spaces < REQ_SPACES)
	{
		if (num_spaces == 0)
		{
			method = token;
			token = strtok (NULL, " ");
			num_spaces++;
		}

		else if (num_spaces == 1)
		{
			argument = token;
			token = strtok (NULL, " ");
			num_spaces++;
		}

		else
		{
			version = token;
			token = strtok(NULL, " ");
			num_spaces++;
		}
	}

	/* Conditional evaluates whether given request conforms to rules */
	if (num_spaces == REQ_SPACES && strstr (version, "HTTP/1.1\r\n") == NULL)
	{
		errResponse (MALFORM_REQUEST, "Malformed Request");
	}

	/* Evaluates whether given protocol is valid */
	else if (strncmp (version, VALID_PROTOCOL, PROTOCOL_CASE_CNT) != 0)
	{
		errResponse (UNSUPPORTED_PROTOCOL, "Protocol Not Implemented");
	}

	/* Evaluates whether given METHOD is GET */
	else if (strcmp (method, GET) == 0)
	{
		getRequest();
	}

	/* Evaluates whether given METHOD is SHUTDOWN */
	else if (strcmp (method, SHUTDOWN) == 0)
	{
		shutdownRequest();
	}

	/* Valid format, valid protocol, but unsupported method (not GET or SHUTDOWN) */
	else
	{
		errResponse (UNSUPPORTED_METHOD, "Unsupported Method");
	}
}

/* Setting up socket connection */
void socketConnection()
{
	struct sockaddr_in sin;
	struct sockaddr addr;
	struct protoent *protoinfo;
	unsigned int addrlen;
	int bytes_read;

	if ((protoinfo = getprotobyname (PROTOCOL)) == NULL)
	{
		errExit ("Error: cannot find protocol info for %s", PROTOCOL);
	}

	/* setup endpoint info */
	memset ((char *)&sin,0x0,sizeof (sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons ((u_short) atoi (port));

	/* Allocate a socket */
	sd = socket (PF_INET, SOCK_STREAM, protoinfo->p_proto);
	if (sd < 0)
	{
		errExit ("Error: cannot create socket", NULL);
	}

	/* Bind the socket */
	if (bind (sd, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		errExit ("Error: cannot bind to port %s", port);
	}

	while (1)
	{
		/* Listen for incoming connections */
		if (listen (sd, QLEN) < 0)
		{
			errExit ("Error: cannot listen on port %s", port);
		}

		/* Accept a connection */
		addrlen = sizeof (addr);
		sd2 = accept (sd,&addr,&addrlen);
		if (sd2 < 0)
		{
			errExit ("Error: accepting connection", NULL);
		}

		/* Reading request from socket */
		bytes_read = read (sd2, request, REQLEN);
		if (bytes_read < 0)
		{
			errExit ("Error: reading from socket", NULL);
		}

		parseRequest();
	}
}

/* Main method, takes in command line args and connects socket */
int main (int argc, char *argv[])
{
	parseArgs (argc, argv);
	mandatoryFlags();
	socketConnection();
}

