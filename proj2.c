# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <strings.h>
# include <netdb.h>
# include <sys/socket.h>
# include <netinet/in.h>

# define VALID_URL "http://"
# define URL_CASE_CNT 7
# define SET_FLAG 1
# define REMOVE_FLAG 0
char *url = NULL;
char *hostname = NULL;
char *webfile = NULL;
char *output_file = NULL;
unsigned short u_flag = 0;
unsigned short d_flag = 0;
unsigned short q_flag = 0;
unsigned short r_flag = 0;
unsigned short o_flag = 0;
unsigned short total_flags = 0;

void parseurl()
{
	char* token = strtok(url, "/");
	token = strtok(NULL, "/");
	hostname = token;
	token = strtok(NULL, "/");
	webfile = token;
}

void parseargs (int argc, char *argv [])
{
	int opt;
	extern char *optarg;
	extern int optind;

	while ((opt = getopt (argc, argv, "u:dqro:")) != -1)
	{
		switch (opt)
		{
			case 'u':
				u_flag += SET_FLAG;
				url = optarg;
				break;
			case 'd':
				d_flag += SET_FLAG;
				break;
			case 'o':
				o_flag += SET_FLAG;
				output_file = optarg;
				break;
			case 'q':
				q_flag += SET_FLAG;
				break;
		}
	}

	/* Throw an error if no url is given (mandatory argument) */
	if (u_flag == REMOVE_FLAG)
	{
		fprintf (stderr, "Error: no url given\n");
		exit (1);
	}

	/* Throw an error if no output file is given (mandatory argument) */
	if (o_flag == REMOVE_FLAG)
	{
		fprintf (stderr, "Error: no output file given\n");
		exit (1);
	}

	else if ((optind+1)>argc)
	{

	}
}

int main (int argc, char *argv [])
{
	parseargs (argc,argv);
	total_flags = d_flag + q_flag;

	/* Handles the base case of a valid URL */
	if (u_flag == SET_FLAG && strncasecmp(url, VALID_URL, URL_CASE_CNT) == 0)
	{
		u_flag = REMOVE_FLAG;
	}

	/* Handles the case of invalid URL given */
	else if (u_flag == SET_FLAG)
	{
		u_flag = REMOVE_FLAG;
		fprintf (stderr, "Error: invalid url given\n");
		exit (1);
	}

	parseurl();

	for (int i = 0; i < total_flags; i++)
	{

		/* Handles -d flag, and prints debugging information */
		if (d_flag == SET_FLAG)
		{
			d_flag = REMOVE_FLAG;
			fprintf (stdout, "DBG: host: %s\n", hostname);
			fprintf (stdout, "DBG: webfile: %s\n", webfile);
			fprintf (stdout, "DBG: output_file: %s\n", output_file);
		}

		/*  Handles -q flag, and prints the HTTP request sent by the web client */
		else if (q_flag == SET_FLAG)
		{
			q_flag = REMOVE_FLAG;
			fprintf (stdout, "OUT: GET %s HTTP/1.0\r\n", webfile);
			fprintf (stdout, "OUT: Host: %s/\r\n", hostname);
			fprintf (stdout, "OUT: User-Agent: CWRU CSDS 325 SimpleClient 1.0\r\n");
			fprintf (stdout, "\r\n");
		}
	}
	exit (0);
}
