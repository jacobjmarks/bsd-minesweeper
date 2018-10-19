/*******************************************************************************
 * auth.c
 * 
 * Includes methods to handle the login requests of new clients.
 * 
 * Author: Jacob Marks n9188100
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <unistd.h>

#include "../common.h"
#include "auth.h"

/* -------------------------- FORWARD DECLARATIONS -------------------------- */

bool authenticate(char*, char*);

/* -------------------------------- PUBLIC ---------------------------------- */

/**
 * Attempts to authenticate an incoming client. If successful, the logged in
 * user is stored in the provided pointer.
 * 
 * Returns 1 if there was an issue completing the process, 0 otherwise.
 */
int client_login(int fd, char* user) {
    bool authenticated = false;

    while (!authenticated) {
        char* request;
        if (recv_string(fd, &request) <= 0) {
            printf("Closing connection: Error connecting to client.\n");
            return 1;
        }
        int protocol = ctoi(request[0]);

        if (protocol != LOGIN) continue;

        printf("Serving {\n");
        printf("    Protocol: %d\n", protocol);
        printf("    Message:  %s\n", request + 1);
        printf("}\n");

        char credentials[strlen(request)];
        strncpy(credentials, request + 1, strlen(request));

        char* input_user = strtok(credentials, ":");
        char* input_pass = strtok(NULL, "\n");

        if (input_user != NULL && input_pass != NULL) {
            printf("Authenticating %s:%s ...\n", input_user, input_pass);
            if ((authenticated = authenticate(input_user, input_pass))) {
                printf("  Granted\n");
                strcpy(user, input_user);
            } else {
                printf("  Denied\n");
            }
        } else {
            printf("Error parsing credentials.\n");
        }
        
        printf("Responding: %d\n", authenticated);
        send_int(fd, authenticated);
    }

    return strlen(user) ? 0 : 1;
}

/* -------------------------------- PRIVATE --------------------------------- */

/**
 * Authenticates a given user and password against the defined credentials
 * within the external authentication tsv file.
 * 
 * Returns true if the user and password is successfully found.
 */
bool authenticate(char* user, char* pass) {
    bool authenticated = false;

    FILE* auth_file = fopen("server/authentication.tsv", "r");

    if (!auth_file) {
        fprintf(stderr, "ERROR READING AUTH FILE\n");
        return false;
    }

    char line[255];
    fgets(line, sizeof(line), auth_file); // Skip first line
    while (fgets(line, sizeof(line), auth_file) != NULL) {
        bool user_match = strcmp(user, strtok(line, "\t")) == 0;
        bool pass_match = strcmp(pass, strtok(NULL, "\n")) == 0;
        if (user_match && pass_match) {
            authenticated = true;
            break;
        }
    }
    fclose(auth_file);

    return authenticated;
}
