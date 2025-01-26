#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>  
#include <sys/wait.h> 

#define MAXBUFLEN 1024
#define UDP_PORT 44720 //Port for receiving data from backend servers
#define TCP_PORT 45720 //Port for client connections
#define MAX_USERS 100
#define MAX_ROOM_TYPES 100
#define RESPONSE_TIMEOUT 2


typedef struct {
    char server_id;     //Server identifier ('S', 'D', 'U')
    int is_received;    //Flag to check if the status has been received
    struct sockaddr_in addr;
} ServerInfo;

ServerInfo serverInfo[3];

typedef struct {
    char username[50];
    char password[50];
} user_credentials;

user_credentials users[MAX_USERS];
int num_users = 0;

void initialize_server_info() {
    // Server S
    serverInfo[0].server_id = 'S';
    serverInfo[0].addr.sin_family = AF_INET;
    serverInfo[0].addr.sin_port = htons(41720);
    serverInfo[0].addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Server D
    serverInfo[1].server_id = 'D';
    serverInfo[1].addr.sin_family = AF_INET;
    serverInfo[1].addr.sin_port = htons(42720);
    serverInfo[1].addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Server U
    serverInfo[2].server_id = 'U';
    serverInfo[2].addr.sin_family = AF_INET;
    serverInfo[2].addr.sin_port = htons(43720);
    serverInfo[2].addr.sin_addr.s_addr = inet_addr("127.0.0.1");
}

//search from the internet about the signal handling
void sigchld_handler(int s) {
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void handle_udp_message(const char *msg, int len) {
    if (len > 0) {
        char serverId = msg[0]; //Assume first character is the server ID

        for (int i = 0; i < 3; i++) {
            if (serverInfo[i].server_id == serverId && !serverInfo[i].is_received) {
                printf("The main server has received the room status from Server %c using UDP over port %d.\n", serverId, UDP_PORT);
                serverInfo[i].is_received = 1; 
            }
        }
    }
}

//load user credentials from member.txt
void load_user_credentials(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Cannot open file");
        exit(1);
    }
    char line[1024]; 

    while (fgets(line, sizeof(line), file) != NULL) {
        char username[50], password[50];
        //Use sscanf to parse the line, skipping any spaces after the comma
        if (sscanf(line, "%49[^,], %49s", username, password) == 2) {
            strncpy(users[num_users].username, username, sizeof(users[num_users].username) - 1);
            users[num_users].username[sizeof(users[num_users].username) - 1] = '\0'; 
            strncpy(users[num_users].password, password, sizeof(users[num_users].password) - 1);
            users[num_users].password[sizeof(users[num_users].password) - 1] = '\0'; 
            num_users++;
            if (num_users >= MAX_USERS) {
                break; 
            }
        }
    }

    fclose(file);
}

int authenticate_user(const char* username, const char* password) {
    if (strcmp(password, "GUEST") == 0) {  //Check for guest login
        return 2;  
    }
    for (int i = 0; i < num_users; i++) {
        if (strcmp(users[i].username, username) == 0) {
            if (strcmp(users[i].password, password) == 0) {
                return 1;  //Authentication successful
            }
            return -1;  //Password is incorrect
        }
    }
    return 0;  //Username does not exist
}

void send_to_backend_server(char serverId, const char *message, char *result) {
    int sockfd;
    struct sockaddr_in backend_addr;
    socklen_t addr_len;
    char buffer[MAXBUFLEN];
    fd_set read_fds;
    struct timeval tv;

    for (int i = 0; i < 3; i++) {
        if (serverInfo[i].server_id == serverId) {
            sockfd = socket(AF_INET, SOCK_DGRAM, 0);
            if (sockfd < 0) {
                perror("Cannot create socket");
                return;
            }

            //Send the message to the backend server
            if (sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&serverInfo[i].addr, sizeof(serverInfo[i].addr)) < 0) {
                perror("sendto failed");
                close(sockfd);
                return;
            }

            printf("The main server sent a request to Server %c.\n", serverId);

            //From beej
            FD_ZERO(&read_fds);
            FD_SET(sockfd, &read_fds);
            tv.tv_sec = RESPONSE_TIMEOUT;
            tv.tv_usec = 0;

            //From beej
            if (select(sockfd + 1, &read_fds, NULL, NULL, &tv) > 0) {
                addr_len = sizeof(backend_addr);
                int n = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0, (struct sockaddr *)&backend_addr, &addr_len);
                if (n < 0) {
                    perror("recvfrom failed");
                    strncpy(result, "Error receiving response", MAXBUFLEN);
                } else {
                    buffer[n] = '\0'; 
                    strncpy(result, buffer, MAXBUFLEN); //Copy the response to the result buffer
                }
            } else {
                strncpy(result, "No response received", MAXBUFLEN);
            }

            close(sockfd);
            break;
        }
    }
}


int main() {
    int udp_fd, tcp_fd, new_fd; 
    struct sockaddr_in udp_addr, tcp_addr, their_addr;
    socklen_t addr_len;
    char buf[MAXBUFLEN];
    fd_set master_set, read_fds;

    signal(SIGCHLD, sigchld_handler); //Clean up zombie processes
    initialize_server_info();
    load_user_credentials("member.txt");

    //From beej
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);
    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (udp_fd < 0 || tcp_fd < 0) {
        perror("Failed to create sockets");
        exit(1);
    }

    //From beej
    memset(&udp_addr, 0, sizeof(udp_addr));
    udp_addr.sin_family = AF_INET;
    udp_addr.sin_port = htons(UDP_PORT);
    udp_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(udp_fd, (struct sockaddr *)&udp_addr, sizeof(udp_addr)) < 0) {
        perror("UDP bind failed");
        exit(1);
    }

    //From beej
    memset(&tcp_addr, 0, sizeof(tcp_addr));
    tcp_addr.sin_family = AF_INET;
    tcp_addr.sin_port = htons(TCP_PORT);
    tcp_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(tcp_fd, (struct sockaddr *)&tcp_addr, sizeof(tcp_addr)) < 0) {
        perror("TCP bind failed");
        exit(1);
    }

    if (listen(tcp_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Server is up and running.\n");

    FD_ZERO(&master_set);
    FD_SET(udp_fd, &master_set);
    FD_SET(tcp_fd, &master_set);

    while (1) {
        read_fds = master_set;
        int retval = select(FD_SETSIZE, &read_fds, NULL, NULL, NULL);

        if (retval == -1) {
            if (errno == EINTR) {
                //Interrupted by a signal such as SIGCHLD.
                continue; 
            } else {
                perror("select");
                exit(4);
            }
        }
        
        //From beej
        if (FD_ISSET(udp_fd, &read_fds)) {
            addr_len = sizeof(their_addr);
            int numbytes = recvfrom(udp_fd, buf, MAXBUFLEN - 1, 0, (struct sockaddr *)&their_addr, &addr_len);
            if (numbytes > 0) {
                buf[numbytes] = '\0';
                handle_udp_message(buf, numbytes);
            }
        }
        //From beej
        if (FD_ISSET(tcp_fd, &read_fds)) {
            addr_len = sizeof(their_addr);
            new_fd = accept(tcp_fd, (struct sockaddr *)&their_addr, &addr_len);
            if (new_fd < 0) {
                if (errno != EINTR) {
                    perror("accept");
                }
                continue;
            }
            if (!fork()) {
                close(tcp_fd); //Close listening socket in the child process
                int numbytes = recv(new_fd, buf, MAXBUFLEN - 1, 0);
                int auth_result = -2;
                if (numbytes > 0) {
                    buf[numbytes] = '\0';
                    char username[50], encusername[50], encpassword[50];
                    sscanf(buf, "%s %s %s", username, encusername, encpassword);
                    auth_result = authenticate_user(encusername, encpassword);
                    if (auth_result == 1) {
                        printf("The main server received the authentication for %s using TCP over port 45720.\n", username);
                        send(new_fd, "Welcome member", 17, 0);
                        printf("The main server sent the authentication result to the client.\n");
                    } else if (auth_result == 2){
                        printf("The main server received the guest request for %s using TCP over port 45720.\n", username);
                        printf("The main accepts %s as a guest.\n", username);
                        send(new_fd, "Welcome guest", 17, 0);
                        printf("The main server sent the authentication result to the client.\n");
                    } else if (auth_result == -1) {
                        printf("The main server received the authentication for %s using TCP over port 45720.\n", username);
                        send(new_fd, "Failed login: Password does not match.", 38, 0);
                        printf("The main server sent the authentication result to the client.\n");
                    } else {
                        printf("The main server received the authentication for %s using TCP over port 45720.\n", username);
                        send(new_fd, "Failed login: Username does not exist.", 38, 0);
                        printf("The main server sent the authentication result to the client.\n");
                    }
                }
                while ((numbytes = recv(new_fd, buf, MAXBUFLEN - 1, 0)) > 0) {
                    buf[numbytes] = '\0';
                    char command[15], roomcode[10], username[20], result[MAXBUFLEN];
                    if (sscanf(buf, "%s %s %s", command, username, roomcode) == 3) {

                        if (strcmp(command, "Availability") == 0) {
                            printf("The main server has received the availability request on Room %s from %s using TCP over port 45720.\n", roomcode, username);
                            char serverAssigned = roomcode[0];
                            send_to_backend_server(serverAssigned, buf, result);
                        } else if (strcmp(command, "Reservation") == 0) {
                            printf("The main server has received the reservation request on Room %s from %s using TCP over port 45720.\n", roomcode, username);
                            if(auth_result==2){
                                strncpy(result, "Permission denied: Guest cannot make a reservation.\n", MAXBUFLEN);
                                char response[MAXBUFLEN];
                                snprintf(response, sizeof(response), "%s", result);
                                send(new_fd, response, strlen(response), 0);
                                continue;
                            }
     
                            char serverAssigned = roomcode[0];
                            send_to_backend_server(serverAssigned, buf, result);
                        }

                        char response[MAXBUFLEN];
                        snprintf(response, sizeof(response), "%s", result);
                        send(new_fd, response, strlen(response), 0);

                        if (strcmp(command, "Availability") == 0) {
                            printf("The main server received the response from Server %c using UDP over port 44720.\n", roomcode[0]);
                            fflush(stdout);  //Force the output to appear immediately
                            printf("The main server sent the availability information to the client.\n");
                            fflush(stdout);
                        } else if (strcmp(command, "Reservation") == 0) {
                            printf("The main server has received the reservation request on Room %s from %s using TCP over port 45720.\n", roomcode, username);
                            fflush(stdout); 
                            printf("The main server sent the reservation result to the client.\n");
                            fflush(stdout); 
                        }
                    }
                }

                if (numbytes == 0) {
                    //Connection closed by client
                    printf("Client closed the connection.\n");
                } else if (numbytes < 0) {
                    perror("recv");
                }
                close(new_fd);
                exit(0); //Child exits after handling the connection
            }
            close(new_fd); //Parent closes the child socket
        }
    }

    close(udp_fd);
    close(tcp_fd);
    return 0;
}
