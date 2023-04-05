#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;
#define BUFFER_SIZE 1024
#define NUMCLI 5

void error(const char *msg, int exit_code) {
    cerr << msg << endl;
    exit(exit_code);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        error("Usage: portNum", 1);
    }

    int portNum = stoi(argv[1]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("ERROR opening socket", 2);
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(portNum);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding", 2);
    }

    cout << "BindDone: " << portNum << endl;

    listen(sockfd, NUMCLI);

    cout << "ListenDone: " << portNum << endl;

    while (true) {
        struct sockaddr_in cli_addr;
        socklen_t clilen = sizeof(cli_addr);
        int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) {
            // error("ERROR on accept", 2);
            cerr<<"ERROR on accept"<<endl;
            continue;
        }

        cout << "Client: " << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << endl;
        char buffer1[256];
        memset(buffer1, 0, sizeof(buffer1));

        int n = read(newsockfd, buffer1, 255);
        if (n < 0) {
            // error("ERROR reading from socket",2);
            cerr<<"ERROR reading from socket"<<endl;
            close(newsockfd);
            continue;
        }

        if (strncmp(buffer1, "get ", 4) != 0) {
            cout << "UnknownCmd" << endl;
            close(newsockfd);
            cerr<<"ERROR incorrect command"<<endl;
            continue;
        }

        string filename = string(buffer1 + 4, n - 5);
        cout << "FileRequested: " << filename << endl;

        ifstream file(filename);
        if (!file.is_open()) {
            cout << "FileTransferFail" << endl;
            close(newsockfd);
            cerr<<"Cannot open file"<<endl;
            continue;
        }

        file.seekg(0, ios::end);
        int filesize = file.tellg();
        file.seekg(0, ios::beg);

        char buffer[BUFFER_SIZE];
        int totalBytesSent = 0;
        bool ibroke=false;
        while (!file.eof()) {
            file.read(buffer, BUFFER_SIZE);
            int bytesSent = send(newsockfd, buffer, file.gcount(), 0);
            if (bytesSent < 0) {
                cerr<<"Error sending file"<<endl;
                close(newsockfd);
                ibroke=true;
                break;
            }
            totalBytesSent += bytesSent;
        }
        if(!ibroke){
            file.close();
            cout << "TransferDone: " << totalBytesSent << " bytes" << endl;
            close(newsockfd);
        }
        else{
            file.close();
        }
    }
    close(sockfd);
    return 0;
}