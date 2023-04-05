#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define BUFFER_SIZE 1000
#define NUMCLI 10

using namespace std;


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

    fd_set main_fds, read_fds;
    FD_ZERO(&main_fds); 
    FD_ZERO(&read_fds);
    FD_SET(sockfd,&main_fds);
    int max_fd=sockfd;

    int closeconn;
    char buffer1[BUFFER_SIZE];
    // Enter loop to wait for incoming connections or data from clients
    while(true){
        // Call select system call to wait for activity on sockets
        read_fds = main_fds;
        int num_ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if(num_ready<0){
            error("select error",9);
        }
        // Check for incoming connection requests
        for(int i=0; i<=max_fd; i++){
            memset(&buffer1, 0, BUFFER_SIZE);
            if(FD_ISSET(i, &read_fds)){
                if(sockfd==i){
                    struct sockaddr_in cli_addr;
                    socklen_t clilen = sizeof(cli_addr);
                    int newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
                    if (newsockfd < 0) {
                        cerr<<"ERROR on accept"<<endl;
                        continue;
                    }
                    cout << "Client: " << inet_ntoa(cli_addr.sin_addr) << ":" << ntohs(cli_addr.sin_port) << endl;
                    if(newsockfd>max_fd) max_fd=newsockfd;
                    FD_SET(newsockfd, &main_fds);
                }
                else if((closeconn=recv(i,buffer1,BUFFER_SIZE,0))<=0){
                    close(i); //Close connection
                    FD_CLR(i, &main_fds); //Remove from main file ds
                    continue;
                }
                else{
                    if(strncmp(buffer1, "get ", 4) != 0 && strncmp(buffer1, "put ", 4) != 0){
                        cout << "UnknownCmd" << endl;
                        close(i);
                        cerr<<"ERROR incorrect command"<<endl;
                        FD_CLR(i, &main_fds); //Remove from main file ds
                        continue;
                    }
                    if(strncmp(buffer1,"get ",4)==0){
                        string filename = string(buffer1 + 4, closeconn - 5);
                        cout << "FileRequested: " << filename << endl;
                        ifstream filesend(filename);
                        if (!filesend.is_open()) {
                            cout << "FileTransferFail" << endl;
                            close(i);
                            cerr<<"Cannot open file"<<endl;
                            FD_CLR(i, &main_fds);
                            continue;
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
                                int bytesSent = send(i, buffer2, filesend.gcount(), 0);
                                if (bytesSent < 0) {
                                    cerr<<"Error sending file"<<endl;
                                    close(i);
                                    FD_CLR(i, &main_fds);
                                    ibroke=true;
                                    break;
                                }
                                totalBytesSent += bytesSent;
                            }
                            if(!ibroke){
                                filesend.close();
                                cout << "TransferDone: " << totalBytesSent << " bytes" << endl;
                                close(i);
                                FD_CLR(i, &main_fds);
                            }
                            else {
                                filesend.close();
                                cout<<"FileTransferFail"<<endl;
                                close(i);
                                cerr<<"Cannot open file"<<endl;
                                FD_CLR(i, &main_fds);
                            }
                        }
                    }
                    else if(strncmp(buffer1,"put ",4)==0){
                        string file1="";
                        int k=4;
                        while (k<BUFFER_SIZE && buffer1[k]!='\0'){
                            file1+=buffer1[k];
                            k++;
                        }
                        // cout<<"file1"<<file1<<endl;
                        // string filename = string(buffer1 + 4, closeconn - 5);
                        cout << "FileGetting: " << file1 << endl;
                        ofstream filerecv(file1, ios::binary);
                        if (!filerecv.is_open()) {
                            cout << "FileTransferFail" << endl;
                            close(i);
                            cerr<<"Cannot open file"<<endl;
                            FD_CLR(i, &main_fds);
                            continue;
                        }
                        int stlen=file1.length();
                        int totalBytesRecv=0;
                        if((stlen+5)<closeconn){
                            filerecv.write(buffer1+5+stlen,closeconn-5-stlen);
                            totalBytesRecv+=(closeconn-5-stlen);
                        }
                        char buffer3[BUFFER_SIZE];
                        memset(&buffer3, 0, BUFFER_SIZE);
                        
                        bool ibroke=false;
                        while (true) {
                            int bytesReceived = recv(i, buffer3, BUFFER_SIZE, 0);
                            if (bytesReceived <= 0) {
                                break;
                            }
                            filerecv.write(buffer3, bytesReceived);
                            totalBytesRecv += bytesReceived;
                        }
                        filerecv.close();
                        cout << "ReceiveDone: " << totalBytesRecv << " bytes" << endl;
                        close(i);
                        FD_CLR(i, &main_fds);
                    }    
                }
            }   
        }
    }
}