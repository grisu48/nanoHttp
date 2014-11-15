//
// webserver.c
//
// Simple HTTP server sample for sanos
//

#include "nanoHttp.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include <fcntl.h>
#if _NANOHTTP_THREADING == 1
#include <pthread.h>
#endif

/* ==== GLOBAL SETTINGS ===================================================== */
#define VERSION "0.3"

#define SERVER "nanoHttp/1.1"
#define PROTOCOL "HTTP/1.0"
#define RFC1123FMT "%a, %d %b %Y %H:%M:%S GMT"

// Default port
static int port = DEFAULT_PORT;
// Working directory
static char working_dir[STR_BUF];
// Server socket
static int sock;

#if _NANOHTTP_THREADING == 1
struct {
	int fd;
	int thread;
} typedef thread_args;

volatile pthread_t threads[THREAD_COUNT];
#endif


static ssize_t strwrite(int fd, char* str) {
	size_t len = strlen(str);
	if(fd < 0 || len <= 0) return 0;
	return write(fd, str, len);
}

static char* removeDoubleSlash(char* buf) {
	const size_t len = strlen(buf)-1;
	for(size_t i = 0; i < len;i++) {
		if(buf[i] == '/' && buf[i+1] == '/') {
			// Shift
			for(size_t j = i;j<len;j++)
				buf[j] = buf[j+1];
		}
	}
	return buf;
}

static size_t fdgets(int fd, char* buf, int size) {
	size_t i = 0;
	char c;
	
	while(i < size) {
		if (read(fd, &c, 1) <= 0) break;
		if(c == EOF) break;
		buf[i++] = c;
		if (c == '\n') break;
	}
	
	return i;
}



static char *get_mime_type(char *name) {
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

void send_headers(int fd, int status, char *title, char *extra, char *mime, int length, time_t date) {
	time_t now;
	char timebuf[128];
	char strbuf[16*1024];
	
	sprintf(strbuf, "%s %d %s\r\n", PROTOCOL, status, title);
	strwrite(fd, strbuf);
	sprintf(strbuf, "Server: %s\r\n", SERVER);
	strwrite(fd, strbuf);
	now = time(NULL);
	strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&now));
	sprintf(strbuf, "Date: %s\r\n", timebuf);
	strwrite(fd, strbuf);
	if (extra) {
		sprintf(strbuf, "%s\r\n", extra);
		strwrite(fd, strbuf);
	}
	if (mime) {
		sprintf(strbuf, "Content-Type: %s\r\n", mime);
		strwrite(fd, strbuf);
	}
	if (length >= 0) {
		sprintf(strbuf, "Content-Length: %d\r\n", length);
		strwrite(fd, strbuf);
	}
	if (date != -1) {
		strftime(timebuf, sizeof(timebuf), RFC1123FMT, gmtime(&date));
		sprintf(strbuf, "Last-Modified: %s\r\n", timebuf);
		strwrite(fd, strbuf);
	}
	strwrite(fd, "Connection: close\r\n");
	strwrite(fd, "\r\n");
}

void send_error(int fd, int status, char *title, char *extra, char *text) {
	FILE* f = fdopen(fd, "a+");
	send_headers(fd, status, title, extra, "text/html", -1, -1);
	fprintf(f, "<HTML><HEAD><TITLE>%d %s</TITLE></HEAD>\r\n", status, title);
	fprintf(f, "<BODY><H4>%d %s</H4>\r\n", status, title);
	fprintf(f, "%s\r\n", text);
	fprintf(f, "</BODY></HTML>\r\n");
}


int send_file(int fd, char *path, struct stat *statbuf) {
	
	char data[READ_BUF];
	int n;

	int file_fd = open(path, O_RDONLY);
	if (file_fd < 0) {
		send_error(fd, 403, "Forbidden", NULL, "Access denied.");
		return -403;
	} else {
		size_t length = S_ISREG(statbuf->st_mode) ? statbuf->st_size : -1;
		size_t sent = 0;
		send_headers(fd, 200, "OK", NULL, get_mime_type(path), length, statbuf->st_mtime);
		printf("Sending file to client ... (%ld bytes) \n", length);
		while ((n = read(file_fd, data, sizeof(char) * READ_BUF)) > 0) {
			size_t b_sent = write(fd, data, sizeof(char) * n);
			if((int)b_sent < 0) {
				if(errno == 0)
					fprintf(stderr, "Error writing to socket: Unknown error\n");
				else
					fprintf(stderr, "Error writing to socket: %s \n", strerror(errno));
				break;
			} else if((int)b_sent == 0) 
				break;
			else
				sent += b_sent;
    }
    close(file_fd);
    
    if(sent == length) {
	    return 0;
	} else {
		printf("Incomplete sent (%ld of %ld bytes)\n", sent, length);
		return -1;
	}
  }
  
  return -2;
}

/* Process a request on the given stream */
int process(int fd) {
	char buf[STR_BUF];
	char *method;
	char path[STR_BUF];
	char *w_path;
	char *protocol;
	struct stat statbuf;
	char pathbuf[STR_BUF];

	int len;
	
	
	if (fdgets(fd, buf, sizeof(buf)) <= 0) return -1;
	removeDoubleSlash(buf);
	printf("GET: %s", buf);

	method = strtok(buf, " ");
	w_path = strtok(NULL, " ");
	protocol = strtok(NULL, "\r");
	if (!method || !w_path || !protocol) return -1;
	if(w_path[0] == '/' && strlen(w_path)>1)
		snprintf(path, sizeof(path), "%s%s", working_dir, w_path+1);
	else
		snprintf(path, sizeof(path), "%s%s", working_dir, w_path);
	
	//fseek(f, 0, SEEK_CUR); // Force change of stream direction
	if (strcasecmp(method, "GET") != 0) {
		send_error(fd, 501, "Not supported", NULL, "Method is not supported.");
		fprintf(stderr, "Method '%s' not supported \n", method);
		return -2;
	} else if (stat(path, &statbuf) < 0) {
		send_error(fd, 404, "Not Found", NULL, "File not found.");
		fprintf(stderr, "File '%s' not found \n", path);
		return -404;
	} else if (S_ISDIR(statbuf.st_mode)) {
		len = strlen(path);
		if (len == 0 || path[len - 1] != '/') {
			snprintf(pathbuf, sizeof(pathbuf), "Location: %s/", path);
			send_error(fd, 302, "Found", pathbuf, "Directories must end with a slash.");
		} else {
			snprintf(pathbuf, sizeof(pathbuf), "%sindex.html", path);
			if (stat(pathbuf, &statbuf) >= 0) {
				int res = send_file(fd, pathbuf, &statbuf);
				if(res != 0) return res;
			} else {
				// Remove double slash at end, if so
				size_t slash = (len-1);
				while(slash > 0 && path[slash] == '/') {
					path[slash+1] = '\0';
					slash--;
				}
				
				char strbuf[STR_BUF];
				DIR *dir;
				struct dirent *de;

				send_headers(fd, 200, "OK", NULL, "text/html", -1, statbuf.st_mtime);
				
				sprintf(strbuf, "<HTML><HEAD><TITLE>Index of %s</TITLE></HEAD>\r\n<BODY>", path);
				strwrite(fd, strbuf);
				sprintf(strbuf, "<H4>Index of %s</H4>\r\n<PRE>\n", path);
				strwrite(fd, strbuf);
				sprintf(strbuf, "Name                             Last Modified              Size\r\n");
				strwrite(fd, strbuf);
				sprintf(strbuf, "<HR>\r\n");
				strwrite(fd, strbuf);
				if (len > 1) {
					sprintf(strbuf, "<A HREF=\"..\">..</A>\r\n");
					strwrite(fd, strbuf);
				}

				dir = opendir(path);
				while ((de = readdir(dir)) != NULL) {
					char timebuf[32];
					struct tm *tm;

					strcpy(pathbuf, path);
					strcat(pathbuf, de->d_name);

					stat(pathbuf, &statbuf);
					tm = gmtime(&statbuf.st_mtime);
					strftime(timebuf, sizeof(timebuf), "%d-%b-%Y %H:%M:%S", tm);

					sprintf(strbuf, "<A HREF=\"%s%s\">", de->d_name, S_ISDIR(statbuf.st_mode) ? "/" : "");
					strwrite(fd, strbuf);
					sprintf(strbuf, "%s%s", de->d_name, S_ISDIR(statbuf.st_mode) ? "/</A>" : "</A> ");
					strwrite(fd, strbuf);
					if (strlen(de->d_name) < 32) {
						sprintf(strbuf, "%*s", 32 - (int)strlen(de->d_name), "");
						strwrite(fd, strbuf);
					}
					if (S_ISDIR(statbuf.st_mode)) {
						sprintf(strbuf, "%s\r\n", timebuf);
						strwrite(fd, strbuf);
					} else {
						sprintf(strbuf, "%s %10d\r\n", timebuf, (int)statbuf.st_size);
						strwrite(fd, strbuf);
					}
				}
				closedir(dir);
				sprintf(strbuf, "</PRE>\r\n<HR>\r\n<ADDRESS>%s</ADDRESS>\r\n</BODY></HTML>\r\n", SERVER);
				strwrite(fd, strbuf);
				}
			}
	} else {
		send_file(fd, path, &statbuf);
	}

	return 0;
}


#if _NANOHTTP_THREADING == 1
/* Process a socket */
void process_thread(thread_args *args) {
	int fd, thread;
	int res;
	
	fd = args->fd;
	thread = args->thread;
	free(args);
	
	threads[thread] = pthread_self();
    res = process(fd);
    shutdown(fd, SHUT_RDWR);
    close(fd);
    threads[thread] = 0;
    
    if(res != 0) 
    	printf("Returned status %d \n", res);
    
}
#endif


static void sig_handler(int signo) {
	switch(signo) {
	case SIGINT:
		fprintf(stderr, "SIGINT received\n");
		exit(EXIT_FAILURE);
		break;
	case SIGUSR1:
		fprintf(stderr, "SIGUSR1 received\n");
		exit(EXIT_FAILURE);
		break;
	case SIGUSR2:
		fprintf(stderr, "SIGUSR2 received\n");
		exit(EXIT_FAILURE);
		break;
	case SIGALRM:
		fprintf(stderr, "SIGALRM received\n");
		exit(EXIT_FAILURE);
		break;
	case SIGSEGV:
		fprintf(stderr, "SIGSEGV received\n");
		exit(EXIT_FAILURE);
		break;
	}
}

void cleanup(void) {
	if(sock > 0) close(sock);
	sock = 0;
#if _NANOHTTP_THREADING == 1
	for(int i=0;i<THREAD_COUNT;i++) {
		pthread_t pid = threads[i];
		if(pid <= 0) continue;
		
		pthread_kill(pid, SIGINT);
	}
#endif
}

int main(int argc, char *argv[]) {
  struct sockaddr_in sin;

	if(getcwd(working_dir, sizeof(working_dir)) == NULL) {
		fprintf(stderr, "WARN: error getting working directory\n");
		strcpy(working_dir, "/var/www");
	}

	// Parse program arguments
	for(int i=1;i<argc;i++) {
		char* arg = argv[i];
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
				working_dir[len] = '\0';
				
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
	
	
	// Set signals
	signal(SIGINT, sig_handler);
	signal(SIGUSR1, sig_handler);
	signal(SIGUSR2, sig_handler);
	signal(SIGALRM, sig_handler);
	signal(SIGSEGV, sig_handler);
	atexit(cleanup);
	
#if _NANOHTTP_THREADING == 1
	for(int i=0;i<THREAD_COUNT;i++) 
		threads[i] = 0;
#endif
	

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
			// Get a thread that is not occupied
			int x = -1;
			
			// XXX: Busy waiting possible :-(
			while(x < 0) {
				for(int i=0;i<THREAD_COUNT;i++) {
					if(threads[i] == 0) {
						x = i; break;
					}
				}
			}
			threads[x] = -1;		// Occupied but not yet populated.
			
			// Create Thread
			args->fd = s;
			args->thread = x;
			int res = pthread_create(&thread, NULL, (void * (*)(void *))process_thread, args);
			if(res < 0) {
				fprintf(stderr, "Error creating thread: %s \n", strerror(errno));
				close(s);
				free(args);
				continue;
			} else {
				// Also set in the thread to ensure data consistency
				threads[x] = thread;
			}
		}
#else
		// Directly process request (single-thread)
		process(s);
		shutdown(s, SHUT_RDWR);
		close(s);
#endif
	  }
  }

  close(sock);
  printf("Bye\n");
  return 0;
}

