#include "../libs/markdown.h"

#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdlib.h>

//For FIFO
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include <inttypes.h>
#include <string.h>
#include <stdbool.h>


document* doc;
unsigned long long interval;
int quit = 0;

uint64_t clients_connected = 0;
uint64_t clients_disconnected = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;


int execute_cmd(char* cmd, int is_increment){
    pthread_mutex_lock(&mutex2);
    int return_value = 4;

    if (is_increment){
        markdown_increment_version(doc);
        printf("version increment %ld\n", doc->version);

    } else{
        cmd[strcspn(cmd, "\r\n")] = '\0';

        char* second_part_ptr;
        char* value_ptr = strtok_r(cmd, " ", &second_part_ptr);
        (void)value_ptr;
        if (*second_part_ptr != '\0'){ // has 2 parameters

            char* value_ptr2 = strtok_r(NULL, " ", &second_part_ptr);

            // Get number
            char* end_ptr;
            unsigned long long val = strtoull(value_ptr2, &end_ptr, 10);
            if (*end_ptr == '\0' && end_ptr != value_ptr2){

                // Commands with 2 parameters
                if (strcmp(cmd, "NEWLINE") == 0){
                    return_value = markdown_newline(doc, doc->version, val);

                } else if(strcmp(cmd, "BLOCKQUOTE") == 0){
                    return_value = markdown_blockquote(doc, doc->version, val);

                } else if(strcmp(cmd, "ORDERED_LIST") == 0){
                    return_value = markdown_ordered_list(doc, doc->version, val);

                } else if(strcmp(cmd, "UNORDERED_LIST") == 0){
                    return_value = markdown_unordered_list(doc, doc->version, val);

                }  else if(strcmp(cmd, "HORIZONTAL_RULE") == 0){
                    return_value = markdown_horizontal_rule(doc, doc->version, val);


                // Commands with 3 parameters
                } else if (*second_part_ptr != '\0'){// has 3 parameters


                    if (strcmp(cmd, "INSERT") == 0){
                        return_value = markdown_insert(doc, doc->version, val, second_part_ptr);


                    } else if(strcmp(cmd, "DEL") == 0){
                        // Get 2nd number
                        char* end_ptr2;
                        unsigned long long val2 = strtoull(second_part_ptr, &end_ptr2, 10);
                        if (*end_ptr2 == '\0' && end_ptr2 != second_part_ptr){
                            return_value = markdown_delete(doc, doc->version, val, val2);
                        }
                        

                    } else if(strcmp(cmd, "HEADING") == 0){
                        // Get 2nd number
                        char* end_ptr2;
                        unsigned long long val2 = strtoull(second_part_ptr, &end_ptr2, 10);
                        if (*end_ptr2 == '\0' && end_ptr2 != second_part_ptr){
                            markdown_heading(doc, doc->version, val, val2);
                        }


                    } else if(strcmp(cmd, "BOLD") == 0){
                        // Get 2nd number
                        char* end_ptr2;
                        unsigned long long val2 = strtoull(second_part_ptr, &end_ptr2, 10);
                        if (*end_ptr2 == '\0' && end_ptr2 != second_part_ptr){
                            return_value = markdown_bold(doc, doc->version, val, val2);
                        }


                    } else if(strcmp(cmd, "ITALIC") == 0){
                        // Get 2nd number
                        char* end_ptr2;
                        unsigned long long val2 = strtoull(second_part_ptr, &end_ptr2, 10);
                        if (*end_ptr2 == '\0' && end_ptr2 != second_part_ptr){
                            markdown_italic(doc, doc->version, val, val2);
                        }


                    } else if(strcmp(cmd, "CODE") == 0){
                        // Get 2nd number
                        char* end_ptr2;
                        unsigned long long val2 = strtoull(second_part_ptr, &end_ptr2, 10);
                        if (*end_ptr2 == '\0' && end_ptr2 != second_part_ptr){
                            return_value = markdown_code(doc, doc->version, val, val2);
                        }


                    } else if(strcmp(cmd, "LINK") == 0){
                        char* value_ptr3 = strtok_r(NULL, " ", &second_part_ptr);
                        if (*second_part_ptr != '\0'){

                            char* end_ptr2;
                            unsigned long long val2 = strtoull(value_ptr3, &end_ptr2, 10);
                            if (*end_ptr2 == '\0' && end_ptr2 != value_ptr3){
                                return_value = markdown_link(doc, doc->version, val, val2, second_part_ptr);
                            }
                        }
                    }
                }
            }
        }
    }

    //markdown_increment_version(doc);
    pthread_mutex_unlock(&mutex2);
    return return_value;
}


void *thread_func(void *arg){

    pid_t pcpid = *(pid_t *)arg;
    int cpid = (int)pcpid;
    free(arg);
    
    // Open FIFOs
    int dlen = snprintf(NULL, 0, "FIFO_C2S_%d", cpid) + 1;

    char* C2S = malloc(dlen);
    char* S2C = malloc(dlen);
    snprintf(C2S, dlen, "FIFO_C2S_%d", cpid);
    snprintf(S2C, dlen, "FIFO_S2C_%d", cpid);

    mkfifo(C2S, 0666);
    mkfifo(S2C, 0666);


    kill(pcpid, SIGRTMIN + 1); 


    int fdr = open(C2S, O_RDONLY | O_NONBLOCK);
    int fdw = open(S2C, O_WRONLY);


    // Open and determining file size
    FILE *file = fopen("roles.txt", "r");

    fseek(file, 0, SEEK_END);
    uint64_t size = ftell(file);
    fseek(file, 0, SEEK_SET);


    // Use epoll
    int efd = epoll_create1(0);

    struct epoll_event ev = {
        .events = EPOLLIN | EPOLLHUP,
        .data.fd = fdr
    };
    epoll_ctl(efd, EPOLL_CTL_ADD, fdr, &ev);

    struct epoll_event events[1];


    epoll_wait(efd, events, 1, -1);          

    // Read username data sent by client
    uint64_t len = 1;
    char* client_data = malloc(sizeof(char) * len);

    while (read(fdr, &client_data[len - 1], 1) != 0){ 
        if (client_data[len - 1] == '\0'){
            break;
        }
        len++;
        client_data = realloc(client_data, sizeof(char) * len);
    }
    len--;
    client_data = realloc(client_data, sizeof(char) * len);
    client_data[strcspn(client_data, "\r\n")] = '\0';

    
    // Check if username is specified and get its role
    uint64_t pos = 0;
    uint64_t cl_pos = 0;
    int is_name = 1;
    int found = 0;
    char chr;

    // Divide each line into 2 parts by ' ' and '\n', match username
    // and if exists get the first character of the user's role
    while (pos < size) {
        chr = fgetc(file);

        if (chr == ' '){
            if ((cl_pos + 1) == len && is_name){
                found = 1;
            } else{
                is_name = 0;
            }

        } else if (is_name && (cl_pos + 1) < len){
            if(chr == client_data[cl_pos]){
                cl_pos++;
            } else{
                is_name = 0;
            }
        }


        if (found && chr != ' '){ // Stores the role in chr
            break;
        }

        if (chr == '\n'){  // Start parsing again
            cl_pos = 0;
            is_name = 1;
        }

        pos++;
    }


    if (found){
        // First Document Transmission Protocol
        char* role;
        if (chr == 'r'){
            role = "read";
        } else{
            role = "write";
        }

        char* content;
        content = markdown_flatten(doc);

        int transmit_size = snprintf(
                                NULL, 0, "%s role\nversion: %"PRIu64"\nlength of document: %"PRIu64"\ndocument:", 
                                role, doc->version, doc->no_of_char) + 1;
                                
        char* transmit = malloc(sizeof(char) * (transmit_size + doc->no_of_char));

        snprintf(
            transmit, transmit_size, "%s role\nversion: %"PRIu64"\nlength of document: %"PRIu64"\ndocument:\n%s\n", 
            role, doc->version, doc->no_of_char, content);

        write(fdw, transmit, transmit_size);

        //print in server terminal
        printf("Client %s registered.\n", client_data);


        free(content);  
        free(transmit);



 
        // Process commands sent by the client
        while (1){ 
            char* client_data2 = malloc(257);


            int n = epoll_wait(efd, events, 1, -1);
            if (n < 0) {
                free(client_data2);
                break;
            }

            if (events[0].events & EPOLLIN) {               
                int r = read(fdr, client_data2, 257);
//printf("%s\n",client_data2);
                if (r > 0) {
                    if (strcmp(client_data2, "DISCONNECT\n") == 0){
                        free(client_data2);
                        break;
            
                    } else if (strcmp(client_data2, "DOC?\n") == 0){
                        char* doc_data = malloc(doc->no_of_char + 1);
                        doc_data = markdown_flatten(doc);
                        write(fdw, doc_data, doc->no_of_char);
                        free(doc_data);

                    } else{
                        int v = execute_cmd(client_data2, 0);
                        v = abs(v);

                        char* c = malloc(2);
                        c[0] = v + '0';
                        c[1] = '\0';
                        write(fdw, c, 2);
                        free(c);
                    }
                } else{
                    free(client_data2);
                    break;
                }
            }
            if (events[0].events & EPOLLHUP) {
                free(client_data2);
                break;
            }

            free(client_data2);
        }


    } else{ // If user's name is not found
        write(fdw, "Connection rejected by server: Reject UNAUTHORISED", 
            strlen("Connection rejected by server: Reject UNAUTHORISED") + 1);
    }

    
    printf("Client %s disconnected.\n", client_data);
    free(client_data);

    close(fdr);
    close(fdw);
    unlink(C2S);
    unlink(S2C);
    free(C2S);
    free(S2C);

    sleep(1);
    kill(cpid, SIGTERM);

    pthread_mutex_lock(&mutex);
    clients_disconnected++;
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *thread_inc_func(void *arg){
    char cmd[1];
    cmd[0] = '\0';
    while (!quit){
        usleep(interval * 1000);
        if(quit){
            return NULL;
        }
        execute_cmd(cmd, 1);
    }

    (void)arg; (void)cmd;
    return NULL;
}


void handler(int sig, siginfo_t *info, void *ucontext){
    clients_connected++;

    pthread_t thread;
    pid_t* current_cpid = malloc(sizeof(pid_t));
    *current_cpid = info->si_pid;
    pthread_create(&thread, NULL, thread_func, current_cpid);
    pthread_detach(thread);

    (void)sig; (void)ucontext;
    return;
}


int main(int argc, char *argv[]) {
    if (argc == 2){

        char* end_ptr;
        interval = strtoull(argv[1], &end_ptr, 10);
        if (*end_ptr == '\0' && interval > 0){
            printf("Server PID: %d\n", getpid());

            doc = markdown_init();

            // Start increment
            pthread_t thread_increment;
            pthread_create(&thread_increment, NULL, thread_inc_func, NULL);
            pthread_detach(thread_increment);


            // Receive signal sigrtmin
            struct sigaction sa;
            memset(&sa, 0, sizeof(sa));
            sa.sa_sigaction = handler;
            sa.sa_flags = SA_SIGINFO;
            sigemptyset(&sa.sa_mask);
            sigaction(SIGRTMIN, &sa, NULL);

            while (1){ // Check if QUIT cmd is sent
                char* server_data = malloc(sizeof(char) * 256 + 1);
                fgets(server_data, 256 + 1, stdin);

                if (strcmp(server_data, "QUIT\n") == 0){
                    int clients = clients_connected - clients_disconnected;
                    if (clients == 0){
                        printf("Stdin listener initiating shutdown\n"); // due to EOF/error with no clients.
                        free(server_data);
                        quit = 1;
                        usleep(interval * 1000);
                        break;
                    } else{
                        printf("QUIT rejected, %d clients still connected.\n", clients);
                    }
                }
                free(server_data);
            }

            FILE *file = fopen("doc.md", "w");
            markdown_print(doc, file);

            markdown_free(doc);
        }
    }
    printf("Shutting down server\n");
    return 0;
}