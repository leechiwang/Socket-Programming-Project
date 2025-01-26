(a) Full Name: Lee-Chi Wang
(b) Student ID: 6503825720
(c) I didn't complete the extra credit part. 
    What I have done:
    Phase 1: I use linked list in backend server to store the room information, as the number of room types can easily be added, and send it to main server.
    Phase 2: Store and encrypt the user input of name and password, send it to main server and check the identity.
    Phase 3: Handle the request including reservation and availability, send the request to specific backend server via main server.
    Phase 4: Main server send the reply from backend server to the client.
(d) client.c: Deal with user input, encrypt the username, password, and send all(including request) to the main server.
    serverM.c: First receive the room data from three backend server and then use the input client transfered to authenticate and process the request,
               transfer the request to thee according back end server and send back the result get from backend server to the client. 
    serverS.c: Store the room information of single room, and then handle the request from main server.
    serverD.c: Store the room information of single room, and then handle the request from main server.
    serverU.v: Store the room information of single room, and then handle the request from main server.
(e) use following example to demonstrate the output:
    James, SODids392, S233, Reservation

    1. Client console output:
    Client is up and running.
    Please enter the username: James
    Please enter the password: SODids392
    James sent an authentication request to the main server.
    Welcome member James!
    Please enter the room code: S233
    Would you like to search for the availability or make a reservation? (Enter "Availability" to search for the availability or Enter "Reservation" to make a reservation ): Reservation
    James sent an reservation request to the main server.
    The client received the response from the main server using TCP over port 62261.
    Congratulation! The reservation for Room S233 has been made.

    -----Start a new request-----
    Please enter the room code:

    2. ServerM output:
    Server is up and running.
    The main server has received the room status from Server S using UDP over port 44720.
    The main server has received the room status from Server D using UDP over port 44720.
    The main server has received the room status from Server U using UDP over port 44720.
    The main server received the authentication for James using TCP over port 45720.
    The main server sent the authentication result to the client.
    The main server has received the reservation request on Room S233 from James using TCP over port 45720.
    The main server sent a request to Server S.
    The main server has received the reservation request on Room S233 from James using TCP over port 45720.
    The main server sent the reservation result to the client.
    
    3.ServerS output:
    The Server S is up and running using UDP on port 41720.
    The Server S has sent the room status to the main server.
    The Server S received a reservation request from the main server.
    Successful reservation. The count of room S233 is now 5.
    The Server S finished sending the response and the updated room status to the main server.

(f) I think and hope there's no failure...
(g) 
    1. Refer following to beej, but not exactly the same, and all are write in comment part:
        ip4addr.sin_family = AF_INET;
        ip4addr.sin_port = htons(3490);
        inet_pton(AF_INET, "10.0.0.1", &ip4addr.sin_addr);
        uint32_t htonl(uint32_t hostlong); 
        uint16_t htons(uint16_t hostshort); 
        uint32_t ntohl(uint32_t netlong); 
        uint16_t ntohs(uint16_t netshort);
        FD_ZERO(&readfds);
        FD_SET(s1, &readfds); FD_SET(s2, &readfds);
        tv.tv_sec = 10;
        tv.tv_usec = 500000;    
    2. ask gpt about the signal handling code like SIGINT SIGTERM, and also get the function getsockname(), 
       fflush() (search when i can't see the output on the console).