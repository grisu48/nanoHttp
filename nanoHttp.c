//
// webserver.c
//
// Simple HTTP server sample for sanos
//


// Multi-threading on
#define _NANOHTTP_THREADING 1

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#if _NANOHTTP_THREADING == 1
#include <pthread.h>
#endif

#define VERSION "0.2"

#define SERVER "webserver/1.0"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"
#define STR_BUF 4*1024

// Default port
static int port = 80;
// Working directory
static char working_dir[STR_BUF];

struct {
	int fd;
} typedef thread_args;


char *get_mime_type(char *name) {
	char *ext = strrchr(name, '.');
	if (!ext) return NULL;
	else if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
	else if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
	else if (strcmp(ext, ".gif") == 0) return "image/gif";
	else if (strcmp(ext, ".png") == 0) return "image/png";
	else if (strcmp(ext, ".css") == 0) return "text/css";
	else if (strcmp(ext, ".au") == 0) return "audio/basic";
	else if (strcmp(ext, ".wav") == 0) return "audio/wav";
	else if (strcmp(ext, ".avi") == 0) return "video/x-msvideo";
	else if (strcmp(ext, ".mpeg") == 0 || strcmp(ext, ".mpg") == 0) return "video/mpeg";
	else if (strcmp(ext, ".txt") == 0) return "text/plain";
	else if (strcmp(ext, ".xml") == 0) return "text/xml";
	else if (strcmp(ext, ".c") == 0) return "text/plain";
	else if (strcmp(ext, ".cpp") == 0) return "text/plain";
	else if (strcmp(ext, ".h") == 0) return "text/plain";
	else if (strcmp(ext, ".hpp") == 0) return "text/plain";
	else if (strcmp(ext, ".py") == 0) return "text/plain";
	else if (strcmp(ext, ".java") == 0) return "text/plain";
	else if (strcmp(ext, ".csv") == 0) return "text/plain";
	return NULL;
}

void send_headers(FILE *f, int status, char *title, char *extra, char *mime, int length, time_t date) {
  time_t now;
  char timebuf[128];

  fprintf(f, "%s %d %s\r\n", PROTOCOL, status, title);
  fprintf(f, "Server: %s\r\n", SERVER);
  now = time(NULL);
  strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
  fprintf(f, "Date: %s\r\n", timebuf);
  if (extra) fprintf(f, "%s\r\n", extra);
  if (mime) fprintf(f, "Content-Type: %s\r\n", mime);
  if (length >= 0) fprintf(f, "Content-Length: %d\r\n", length);
  if (date != -1) {
    strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&date));
    fprintf(f, "Last-Modified: %s\r\n", timebuf);
  }
  fprintf(f, "Connection: close\r\n");
  fprintf(f, "\r\n");
}

void send_error(FILE *f, int status, char *title, char *extra, char *text) {
  send_headers(f, status, title, extra, "text/html", -1, -1);
  fprintf(f, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n", status, title);
  fprintf(f, "<BODY><H4>%d %s</H4>\r\n", status, title);
  fprintf(f, "%s\r\n", text);
  fprintf(f, "</BODY></HTML>\r\n");
}

void send_file(FILE *f, char *path, struct stat *statbuf) {
  char data[4096];
  int n;

  FILE *file = fopen(path, "r");
  if (!file) {
    send_error(f, 403, "Forbidden", NULL, "Access denied.");
  } else {
    int length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
    send_headers(f, 200, "OK", NULL, get_mime_type(path), length, statbuf->st_mtime);

    while ((n = fread(data, 1, sizeof(data), file)) > 0) fwrite(data, 1, n, f);
    fclose(file);
  }
}

/* Process a request on the given stream */
int process(FILE *f) {
	char buf[STR_BUF];
	char *method;
	char path[STR_BUF];
	char *w_path;
	char *protocol;
	struct stat statbuf;
	char pathbuf[STR_BUF];

	int len;

	if (!fgets(buf, sizeof(buf), f)) return -1;
	printf("URL: %s", buf);

	method = strtok(buf, " ");
	w_path = strtok(NULL, " ");
	protocol = strtok(NULL, "\r");
	if (!method || !w_path || !protocol) return -1;
	if(w_path[0] == '/' && strlen(w_path)>1)
		snprintf(path, sizeof(path), "%s%s", working_dir, w_path+1);
	else
		snprintf(path, sizeof(path), "%s%s", working_dir, w_path);
	
	fseek(f, 0, SEEK_CUR); // Force change of stream direction
	if (strcasecmp(method, "GET") != 0) {
		send_error(f, 501, "Not supported", NULL, "Method is not supported.");
	} else if (stat(path, &statbuf) < 0) {
		send_error(f, 404, "Not Found", NULL, "File not found.");
	} else if (S_ISDIR(statbuf.st_mode)) {
		len = strlen(path);
	if (len == 0 || path[len - 1] != '/') {
		snprintf(pathbuf, sizeof(pathbuf), "Location: %s/", path);
		send_error(f, 302, "Found", pathbuf, "Directories must end with a slash.");
	} else {
		snprintf(pathbuf, sizeof(pathbuf), "%sindex.html", path);
		if (stat(pathbuf, &statbuf) >= 0) {
			send_file(f, pathbuf, &statbuf);
		} else {
			DIR *dir;
			struct dirent *de;

			send_headers(f, 200, "OK", NULL, "text/html", -1, statbuf.st_mtime);
			fprintf(f, "<HTML><HEAD><TITLE>Index of %s</TITLE></HEAD>\r\n<BODY>", path);
			fprintf(f, "<H4>Index of %s</H4>\r\n<PRE>\n", path);
			fprintf(f, "Name                             Last Modified              Size\r\n");
			fprintf(f, "<HR>\r\n");
			if (len > 1) fprintf(f, "<A HREF=\"..\">..</A>\r\n");

			dir = opendir(path);
			while ((de = readdir(dir)) != NULL) {
				char timebuf[32];
				struct tm *tm;

				strcpy(pathbuf, path);
				strcat(pathbuf, de->d_name);

				stat(pathbuf, &statbuf);
				tm = gmtime(&statbuf.st_mtime);
				strftime(timebuf, sizeof(timebuf), "%d-%b-%Y %H:%M:%S", tm);

				fprintf(f, "<A HREF=\"%s%s\">", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
				fprintf(f, "%s%s", de->d_name, S_ISDIR(statbuf.st_mode) ? "/</A>" : "</A> ");
				if (strlen(de->d_name) < 32) fprintf(f, "%*s", 32 - (int)strlen(de->d_name), "");
				if (S_ISDIR(statbuf.st_mode)) {
					fprintf(f, "%s\r\n", timebuf);
				} else {
					fprintf(f, "%s %10d\r\n", timebuf, (int)statbuf.st_size);
				}
			}
			closedir(dir);
			fprintf(f, "</PRE>\r\n<HR>\r\n<ADDRESS>%s</ADDRESS>\r\n</BODY></HTML>\r\n", SERVER);
			}
		}
	} else {
		send_file(f, path, &statbuf);
	}

	return 0;
}


#if _NANOHTTP_THREADING == 1
/* Process a socket */
void process_thread(thread_args *args) {
	FILE *file;
	int fd;
	
	fd = args->fd;
	free(args);
	
	file = fdopen(fd, "a+");
    process(file);
    fclose(file);
    close(fd);
}
#endif


int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in sin;

	if(getcwd(working_dir, sizeof(working_dir)) == NULL) {
		fprintf(stderr, "WARN: error getting working directory\n");
		strcpy(working_dir, "/var/www");
	}

	// Parse program arguments
	for(int i=1;i<argc;i++) {
		char* arg = argv[1];
		bool is_last = i >= argc-1;
		
		if(!strcmp("-h", arg) || !strcmp("--help",arg)) {
			printf("nanoHttp v %s ", VERSION);
#if _NANOHTTP_THREADING == 1
			printf(" (multithread)");
#endif
			printf("\n");
			
			printf("  Usage: %s [OPTIONS]\n", argv[0]);
			printf("OPTIONS:\n");
			printf("\t-h  --help          Print this help message\n");
			printf("\t-p  --port PORT     Define port\n");
			printf("\t-w  --cwd PATH      Define working directory\n");
			printf("\n2014, Felix Niederwanger\n");
			
			
			return EXIT_SUCCESS;
		} else if(!strcmp("-p", arg) || !strcmp("--port",arg)) {
			if(is_last) {
				fprintf(stderr, "Missing argument: port\n");
				return EXIT_FAILURE;
			}
			port = atoi(argv[++i]);
		} else if(!strcmp("-w", arg) || !strcmp("--cwd", arg)) {
			if(is_last) {
				fprintf(stderr, "Missing argument: port\n");
				return EXIT_FAILURE;
			} else {
				size_t len;
				arg = argv[++i];
				len = strlen(arg);
				if(len > sizeof(working_dir)-1) len = sizeof(working_dir)-1;
				strncpy(working_dir, arg, len);
				
			}
			
		} else {
			fprintf(stderr, "WARN: Unknown argument: %s \n", arg);
		}
	}
	
	// Assure, that the working dir ends with a /
	if(working_dir[strlen(working_dir)] != '/') {
		size_t len = strlen(working_dir);
		if(len >= sizeof(working_dir)-1) {
			fprintf(stderr, "Working directory exceeds buffer size\n");
			return EXIT_FAILURE;
		}
		working_dir[len] = '/';
		working_dir[len+1] = '\0';
	}
	printf("Working directory: %s\n", working_dir);
	
	

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0) {
	  fprintf(stderr, "Error setting up the socket: %s \n", strerror(errno));
	  return EXIT_FAILURE;
  }

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = htons(port);
  if(bind(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	  fprintf(stderr, "Error binding socket to port %d: %s \n", port, strerror(errno));
	  return EXIT_FAILURE;
  }

  if(listen(sock, 5) < 0) {
	  fprintf(stderr, "Error listening for new connections on port %d: %s \n", port, strerror(errno));
	  return EXIT_FAILURE;
  }
  printf("HTTP server listening on port %d\n", port);

  while (1) {
    int s;

    s = accept(sock, NULL, NULL);
    if (s < 0) {
		fprintf(stderr, "Error accepting new client: %s \n", strerror(errno));
		break; 
	} else {
#if _NANOHTTP_THREADING == 1
		pthread_t thread = 0;
		thread_args *args = (thread_args*)malloc(sizeof(thread_args));
		if(args == NULL) {
			fprintf(stderr, "Error allocating memory for thread: %s \n", strerror(errno));
			close(s);
			continue;
		} else {
			// Create Thread
			args->fd = s;
			int res = pthread_create(&thread, NULL, (void * (*)(void *))process_thread, args);
			if(res < 0) {
				fprintf(stderr, "Error creating thread: %s \n", strerror(errno));
				close(s);
				free(args);
				continue;
			}
		}
#else
		// Directly process request (single-thread)
		FILE *file;
		file = fdopen(s, "a+");
		process(file);
		fclose(file);
		close(s);
#endif
	  }
  }

  close(sock);
  printf("Bye\n");
  return 0;
}

