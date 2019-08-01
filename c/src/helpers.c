#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>

#include "helpers.h"


int startcmp(char* str, const char* cmp) {
	unsigned int places = strlen(cmp);

	for (int i = 0; i < places; i++) {
		if (str[i] != cmp[i]) {
			return 0;
		}
	}
	return 1;
}
const char* replace(const char* search, const char* replace, const char* subject) {
	char* p = NULL;
	char* old = NULL;
	const char* new_subject = NULL;
    int c = 0 , search_size;
     
    search_size = strlen(search);
     
    for (p = strstr(subject, search); p != NULL; p = strstr(p + search_size, search)) {
        c++;
    }
     
    c = (strlen(replace) - search_size) * c + strlen(subject);
     
    new_subject = (char*) malloc(sizeof(char) * c);
     
    strncpy((char*) new_subject, "\0", c);
     
    old = (char*) subject;
     
    for (p = strstr(subject, search) ;p != NULL ;p = strstr(p + search_size, search)) {
        strncpy((char*) new_subject + strlen(new_subject), old, p - old);
        strcpy((char*) new_subject + strlen(new_subject), replace);
         
        old = p + search_size;
    }
     
    strcpy((char*) new_subject + strlen(new_subject), old);

    return new_subject;
}


int split(const char* str, const char* delim, char*** r, bool command) {

    char **res = (char**) malloc(sizeof(char*) * strlen(str));
    char *p;
    int i = 0;
    while ((p = strtok((char*) str, delim))) {
        res[i] = (char*) malloc(strlen(p) + 1);
        strcpy(res[i], p);
        ++i;
        str = NULL;
    }
    res = (char**) realloc(res, sizeof(char*) * i);
    *r = res;
    return i;
}

