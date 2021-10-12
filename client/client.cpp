#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

bool needStop = false;

void programInfo() {
    std::cout
            << "Hi user!\nServer supports 5 commands:\n\t"
               "1. 'WHO'\n\t2. 'ADD, <x coordinate>, <y coordinate>, <text>'\n\t"
               "3. 'CHANGE, <x coordinate>, <y coordinate>, <text>'\n\t"
               "4. 'DELETE, <x coordinate>, <y coordinate>'\n\t"
               "5. 'END\n";
}

void connectionConfiguration(sockaddr_in &infoAboutServer) {
    infoAboutServer.sin_family = AF_INET;
    infoAboutServer.sin_port = htons(1030);
    infoAboutServer.sin_addr.s_addr = inet_addr("127.0.0.1");
}

bool connection(int &clientSockFD, const sockaddr_in &infoAboutServer) {
    clientSockFD = socket(AF_INET, SOCK_STREAM, 0);
    int Connect = connect(clientSockFD, (sockaddr *) &infoAboutServer, sizeof(infoAboutServer));
    if (Connect < 0) {
        std::cout << "Failed connection, return value: " << Connect << ", errno: " << strerror(errno) << "\n";
        return 0;
    } else {
        std::cout << "Client connected\n";
    }
    return 1;
}

int clientPart() {
    int clientSockFD;
    sockaddr_in infoAboutServer;

    connectionConfiguration(infoAboutServer);

    if (!connection(clientSockFD, infoAboutServer)) {
        return -1;
    }

    while (!needStop) {
        std::cout << "-> ";
        std::string request;
        getline (std::cin, request);
        request += "\n";
        send(clientSockFD, request.c_str(), request.size(), 0);

        if (request == "END"){
            needStop = true;
        }

        char buf[512]{0};
        std::string answer;
        recv(clientSockFD, buf, 512,0);
        answer+=buf;
        std::cout<<answer;

    }


    return 0;
}


int main() {
    std::cout << "Client started\n";

    programInfo();

    int err = clientPart();

    std::cout << "Client end\n";

    return 0;
}
