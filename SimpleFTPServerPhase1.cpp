#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fstream>
#include <sstream>

using namespace std;

#define BUFFER_SIZE 1024

void error(const char *msg, int exit_code) {
    cerr << msg << endl;
    exit(exit_code);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        error("Usage: portNum fileToTransfer", 1);
    }
    int portNum = atoi(argv[1]);

    // Open the file for reading
    ifstream file(argv[2], ios::binary);
    if (!file.is_open()) {
        error("Cannot open file", 3);
    }

    // Get the file size
    file.seekg(0, ios::end);
    int fileSize = file.tellg();
    file.seekg(0, ios::beg);

    // Create the socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket", 2);
    }

    // Bind the socket
    struct sockaddr_in serv_addr, cli_addr;
    // bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(portNum);
    memset(&(serv_addr.sin_zero),'\0',8);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("Error on binding", 2);
    }
    cout << "BindDone: " << portNum << endl;

    // Listen for incoming connections
    listen(sockfd, 1);
    cout << "ListenDone: " << portNum << endl;

    // Accept the incoming connection
    socklen_t clilen = sizeof(cli_addr);
    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
        error("Error on accept", 2);
    }
    cout << "Client: " << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << endl;


    // Transfer the file
    char buffer[BUFFER_SIZE];
    int totalBytesSent = 0;
    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
        int bytesSent = send(newsockfd, buffer, file.gcount(), 0);
        if (bytesSent < 0) {
            error("Error sending file", 2);
        }
        totalBytesSent += bytesSent;
    }
    file.close();
    close(newsockfd);
    close(sockfd);

    cout << "TransferDone: " << totalBytesSent << " bytes" << endl;

    return 0;
}
