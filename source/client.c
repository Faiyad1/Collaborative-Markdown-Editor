#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char* username;
int start = 1;
char role;

void handler(int sig, siginfo_t *info, void *ucontext){
    int cpid = getpid();

    // Open FIFOs
    int dlen = snprintf(NULL, 0, "FIFO_C2S_%d", cpid) + 1;

    char* C2S = malloc(dlen);
    char* S2C = malloc(dlen);
    snprintf(C2S, dlen, "FIFO_C2S_%d", cpid);
    snprintf(S2C, dlen, "FIFO_S2C_%d", cpid);

    int fdw = open(C2S, O_WRONLY);
    int fdr = open(S2C, O_RDONLY | O_NONBLOCK);
 

    // Write the username to the fifo
    write(fdw, username, strlen(username) + 1);


    //Epoll
    int efd = epoll_create1(0);

    struct epoll_event ev = {
        .events = EPOLLIN | EPOLLHUP,
        .data.fd = fdr
    };
    epoll_ctl(efd, EPOLL_CTL_ADD, fdr, &ev);

    struct epoll_event events[1];


    while (1){
        // Read data and display in terminal
        uint64_t len = 1;
        char* server_data = malloc(sizeof(char) * len);


        epoll_wait(efd, events, 1, -1);  

        while (read(fdr, &server_data[len - 1], 1) != 0){
            if (server_data[len - 1] == '\0'){
                break;
            }
            len++;
            server_data = realloc(server_data, sizeof(char) * len);
        }
        server_data[len - 1] = '\0';


        if (start){
            role = server_data[0];
            start = 0;
        }

        if (len > 2){
            printf("%s\n", server_data);
        } else if (len == 2){
            if (server_data[0] == '0'){
                printf("SUCCESS\n");
            } else if (server_data[0] == '1'){
                printf("Rejected INVALID_CURSOR_POS\n");
            } else if (server_data[0] == '2'){
                printf("Rejected DELETED_POSITION\n");
            } else if (server_data[0] == '4'){
                printf("Rejected INVALID OR LOWERCASE COMMAND\n");
            }
        }
        free(server_data);


        // Read from terminal and pass the command to the server
        char* client_data = malloc(sizeof(char) * 256 + 1);
        fgets(client_data, 256 + 1, stdin);
        if (role == 'w'){
            write(fdw, client_data, strlen(client_data) + 1);
        }
        if (strcmp(client_data, "DISCONNECT\n") == 0){
            break;
        }
        free(client_data);
    }

    close(fdw);
    close(fdr);
    free(C2S);
    free(S2C);


    (void)sig; (void)info; (void)ucontext;
    return;
}


int main(int argc, char *argv[]) {
    if (argc == 3){

        char* end_ptr;
        unsigned long long val = strtoull(argv[1], &end_ptr, 10);
        if(*end_ptr == '\0' && val > 0){

            // Send signal
            kill(val, SIGRTMIN);

            username = argv[2];

            //Receive sigrtmin + 1
            struct sigaction sa;
            memset(&sa, 0, sizeof(sa));
            sa.sa_sigaction = handler;
            sa.sa_flags = SA_SIGINFO;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGRTMIN + 1, &sa, NULL);

            pause();
        }
    }
    usleep(1200);
    return 0;
}