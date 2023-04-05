#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fstream>

using namespace std;

#define BUFFER_SIZE 1000

void error(const char *msg, int exit_code) {
    cerr << msg << endl;
    exit(exit_code);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        error("Usage: serverIPAddr:port fileToReceive receiveInterval", 1);
    }

    // Parse the server IP address and port number
    char *serverAddr = strtok(argv[1], ":");
    int portNum = atoi(strtok(NULL, ":"));
    int receive_interval = atoi(argv[3]);
    receive_interval*=1000;

    // Create the socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket", 2);
    }

    // Connect to the server
    struct sockaddr_in serv_addr;
    //bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(serverAddr);
    serv_addr.sin_port = htons(portNum);
    memset(&(serv_addr.sin_zero),'\0',8);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("Error connecting to server", 2);
    }
    cout << "ConnectDone: " << serverAddr << ":" << portNum << endl;
    string filereq="get ";
    filereq+=argv[2];
    filereq+='\0';
    if (send(sockfd, filereq.c_str(), filereq.length(), 0) < 0) {
        error("Unable to send file request to server\n",2);
    }

    // Receive the file
    char buffer[BUFFER_SIZE];
    int totalBytesReceived = 0;
    ofstream file(argv[2], ios::binary);
    if (!file.is_open()) {
        error("Cannot create file", 3);
    }
    while (true) {
        int bytesReceived = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) {
            break;
        }
        file.write(buffer, bytesReceived);
        totalBytesReceived += bytesReceived;
        usleep(receive_interval);
    }
    file.close();
    close(sockfd);

    if (totalBytesReceived == 0) {
        remove(argv[2]);
        error("Error receiving file", 3);
    }

    cout << "FileWritten: " << totalBytesReceived << " bytes" << endl;

    return 0;
}