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
    if (argc != 5) {
        error("Usage: serverIPAddr:port operation fileToReceive receiveInterval", 1);
    }

    // Parse the server IP address and port number
    char *serverAddr = strtok(argv[1], ":");
    int portNum = atoi(strtok(NULL, ":"));
    int receive_interval = atoi(argv[4]);
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
    if(strcmp(argv[2], "get")==0){
        string filereq="get ";
        filereq+=argv[3];
        filereq+='\0';
        if (send(sockfd, filereq.c_str(), filereq.length(), 0) < 0) {
            error("Unable to send file request to server\n",2);
        }

        // Receive the file
        char buffer[BUFFER_SIZE];
        int totalBytesReceived = 0;
        ofstream file(argv[3], ios::binary);
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
            remove(argv[3]);
            error("Error receiving file", 3);
        }

        cout << "FileWritten: " << totalBytesReceived << " bytes" << endl;
    }
    else if(strcmp(argv[2],"put")==0){
        string filereq="put ";
        filereq+=argv[3];
        filereq+='\0';
        if (send(sockfd, filereq.c_str(), filereq.length(), 0) < 0) {
            error("Unable to send file request to server\n",2);
        }
        ifstream filesend(argv[3]);
        if (!filesend.is_open()) {
            cout << "FileTransferFail" << endl;
            close(sockfd);
            cerr<<"Cannot open file"<<endl;
        }
        else{
            char buffer2[BUFFER_SIZE];
            memset(&buffer2, 0, BUFFER_SIZE);
            filesend.seekg(0, ios::end);
            int filesend_size = filesend.tellg();
            filesend.seekg(0, ios::beg);
            int totalBytesSent=0;
            bool ibroke=false;
            while (!filesend.eof()) {
                filesend.read(buffer2, BUFFER_SIZE);
                int bytesSent = send(sockfd, buffer2, filesend.gcount(), 0);
                if (bytesSent < 0) {
                    cerr<<"Error sending file"<<endl;
                    close(sockfd);
                    ibroke=true;
                    break;
                }
                totalBytesSent += bytesSent;
            }
            if(!ibroke){
                filesend.close();
                cout << "TransferDone: " << totalBytesSent << " bytes" << endl;
                close(sockfd);
            }
            else {
                filesend.close();
                cout<<"FileTransferFail"<<endl;
                close(sockfd);
                cerr<<"Cannot send file"<<endl;
            }
        }

    }
    else{
        error("Error in command operation", 9);
    }
    
    return 0;
}