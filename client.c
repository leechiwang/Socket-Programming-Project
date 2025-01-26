#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include <sys/wait.h> 

#define SERVER_PORT 45720

volatile int sockfd = -1; 


//Search from the internet about the signal handling
void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        if (sockfd != -1) {
            close(sockfd);
        }
        exit(0);
    }
}

//Follow the encryption rules from project description
void encrypt2(char *input) {
    while (*input) {
        if (*input >= 'a' && *input <= 'z') {
            *input = ((*input - 'a' + 3) % 26) + 'a';
        } else if (*input >= 'A' && *input <= 'Z') {
            *input = ((*input - 'A' + 3) % 26) + 'A';
        } else if (*input >= '0' && *input <= '9') {
            *input = ((*input - '0' + 3) % 10) + '0';
        }
        input++;
    }
}

//Send the request to main server
void send_request(int sockfd, const char *request, int port) {
    send(sockfd, request, strlen(request), 0);
    char buffer[1024];
    int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    printf("The client received the response from the main server using TCP over port %d.\n", port);
    if (n > 0) {
        buffer[n] = '\0';
        printf("%s\n", buffer);
    }
}

void identity_check(const char *username, const char *encusername, const char *encpassword, int is_guest, int sockfd, int port) {
    char send_buffer[1024];
    snprintf(send_buffer, sizeof(send_buffer), "%s %s %s", username, encusername, encpassword);

    send(sockfd, send_buffer, strlen(send_buffer), 0);
    if (is_guest) {
        printf("%s sent a guest request to the main server using TCP over port %d.\n", username, port);
    } else {
        printf("%s sent an authentication request to the main server.\n", username);
    }

    char buffer[1024];
    int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        buffer[n] = '\0'; 

        if (strcmp(buffer, "Welcome member") == 0) {
            printf("Welcome member %s!\n", username);
        } else if (strcmp(buffer, "Welcome guest") == 0) {
            printf("Welcome guest %s!\n", username);
        } else {
            printf("%s\n", buffer);  
            if (strstr(buffer, "Failed") != NULL) {
                close(sockfd); //Close the connection if login failed
                exit(1); 
            }
        }
    } else {
        if (n < 0) {
            perror("recv failed");
        }
        printf("No response from server or connection closed unexpectedly.\n");
        close(sockfd);
        exit(1);
    }
}

//Handle availability
void availability_request(int sockfd, const char *username, const char *room_code, int port) {
    printf("%s sent an availability request to the main server.\n", username);
    char send_buffer[1024];
    snprintf(send_buffer, sizeof(send_buffer), "Availability %s %s", username, room_code);
    send_request(sockfd, send_buffer, port);
}

//Handle reservation
void reservation_request(int sockfd, const char *username, const char *room_code, int is_guest, int port) {
    printf("%s sent an reservation request to the main server.\n", username);
    char send_buffer[1024];
    snprintf(send_buffer, sizeof(send_buffer), "Reservation %s %s", username, room_code);
    send_request(sockfd, send_buffer, port);
}

int main() {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGCHLD, signal_handler);
    printf("Client is up and running.\n");

    char username[51], password[51], enc_username[51], enc_password[51];
    printf("Please enter the username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    strcpy(enc_username, username); // Copy username to encrypted version

    printf("Please enter the password: ");
    fgets(password, sizeof(password), stdin);
    int is_guest = (strlen(password) == 1 && password[0] == '\n');

    if (is_guest) {  //Check if only enter was pressed
        strcpy(enc_password, "GUEST");  //Use a special flag or keep it empty as preferred
    } else {
        password[strcspn(password, "\n")] = 0;  
        strcpy(enc_password, password); //Copy password to encrypted version
        encrypt2(enc_password);
    }

    encrypt2(enc_username);

    //From beej
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    struct sockaddr_in my_addr;
    socklen_t addrlen = sizeof(my_addr);

    if (sockfd < 0) {
        perror("Cannot create socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(1);
    }

    if (getsockname(sockfd, (struct sockaddr *)&my_addr, &addrlen) == -1) {
        perror("getsockname");
        close(sockfd);
        exit(1);
    }

    int port = ntohs(my_addr.sin_port);
    identity_check(username, enc_username, enc_password, is_guest, sockfd, port);

    char room_code[51], action[51];
    signal(SIGINT, signal_handler);
    while (1) {
        printf("Please enter the room code: ");
        fgets(room_code, sizeof(room_code), stdin);
        room_code[strcspn(room_code, "\n")] = 0; 

        printf("Would you like to search for the availability or make a reservation? (Enter \"Availability\" to search for the availability or Enter \"Reservation\" to make a reservation ): ");
        fgets(action, sizeof(action), stdin);
        action[strcspn(action, "\n")] = 0; 
        if (strcmp(action, "Availability") == 0) {
            availability_request(sockfd, username, room_code, port);
        } else if (strcmp(action, "Reservation") == 0) {
            reservation_request(sockfd, username, room_code, is_guest, port);
        }  
        printf("\n");
        printf("-----Start a new request-----\n");
    }

    close(sockfd);
    return 0;
}