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

#define S_UDP_PORT "41720"
#define MAXBUFLEN 256
#define HASH_SIZE 100

typedef struct room {
    char room_number[10];
    int available_count;
    struct room *next;
} Room;

Room *head = NULL;

//Iterate the room linked list
Room* find_room(const char *room_number) {
    Room *current = head;
    while (current != NULL) {
        if (strcmp(current->room_number, room_number) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

//Insert the new room into the list
void insert_room(const char *room_number, int available_count) {
    Room *new_room = malloc(sizeof(Room));
    if (!new_room) {
        perror("Memory allocation failed");
        exit(1);
    }
    strcpy(new_room->room_number, room_number);
    new_room->available_count = available_count;
    new_room->next = head;
    head = new_room;
}

//Read room information from file
void read_room_data(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        exit(1);
    }

    char room_number[10];
    int available_count;
    while (fscanf(file, " %[^,], %d", room_number, &available_count) == 2) {
        insert_room(room_number, available_count);
    }
    fclose(file);
}

//Send the room data to main server, from beej
void send_room_statuses(const char *server_ip, int port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = inet_addr(server_ip);

    char buffer[MAXBUFLEN];
    Room *room = head;
    while (room != NULL) {
        sprintf(buffer, "%s %d", room->room_number, room->available_count);
        if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            perror("sendto failed");
            close(sockfd);
            exit(1);
        }
        room = room->next;
    }
    close(sockfd);
}

//Handle the request from main server
void handle_requests(int sockfd) {
    struct sockaddr_in cliaddr;
    socklen_t len = sizeof(cliaddr);
    char buffer[MAXBUFLEN];
    char response[MAXBUFLEN];

    while (1) {
        int n = recvfrom(sockfd, buffer, MAXBUFLEN - 1, 0, (struct sockaddr *)&cliaddr, &len);
        if (n == -1) {
            perror("recvfrom failed");
            continue;
        }
        buffer[n] = '\0'; 

        char command[20], room_number[10], username[20];
        if (sscanf(buffer, "%s %s %s", command, username, room_number) == 3) {
            Room *room = find_room(room_number);

            if (!room) {
                if (strcmp(command, "Availability") == 0){
                    printf("The Server D received an availability request from the main server.\n");
                    printf("Not able to find the room layout.\n");
                    sprintf(response, "Not able to find the room layout.\n");
                    printf("The Server D finished sending the response to the main server.\n");
                }else{
                    printf("The Server D received a reservation request from the main server.\n");
                    sprintf(response, "Oops! Not able to find the room.");
                    printf("Cannot make a reservation. Not able to find the room layout.\n");
                    printf("The Server D finished sending the response to the main server.\n");
                }
            } else if (strcmp(command, "Availability") == 0) {
                printf("The Server D received an availability request from the main server.\n");
                if(room->room_number>0){
                    printf("Room %s is available.\n", room_number);
                }else{
                    printf("Room %s is not available.\n", room_number);
                }
                sprintf(response, "The requested room is %s.\n", room->available_count > 0 ? "available" : "not available");
                printf("The Server D finished sending the response to the main server.\n");
            } else if (strcmp(command, "Reservation") == 0) {
                printf("The Server D received a reservation request from the main server.\n");
                if (room->available_count > 0) {
                    room->available_count--;
                    printf("Successful reservation. The count of room %s is now %d.\n", room_number, room->available_count);
                    sprintf(response, "Congratulation! The reservation for Room %s has been made.\n", room_number);
                    printf("The Server D finished sending the response and the updated room status to the main server.\n");
                } else {
                    printf("Cannot make a reservation. Room %s is not available.\n", room_number);
                    sprintf(response, "Sorry! The requested room is not available.\n");
                    printf("The Server D finished sending the response to the main server.\n");
                }
            } 

            if (sendto(sockfd, response, strlen(response), 0, (struct sockaddr *)&cliaddr, len) == -1) {
                perror("sendto failed");
            }

        } 
    }
}

int main() {
    printf("The Server S is up and running using UDP on port %d.\n", atoi(S_UDP_PORT));
    read_room_data("single.txt");
    send_room_statuses("127.0.0.1", 44720);
    printf("The Server S has sent the room status to the main server.\n");

    //Setup UDP server to listen for incoming requests, from beej
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(1);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(S_UDP_PORT));
    servaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind failed");
        close(sockfd);
        exit(1);
    }

    handle_requests(sockfd);
    return 0;
}
