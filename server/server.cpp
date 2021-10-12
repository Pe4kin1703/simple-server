#include <iostream>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <chrono>
#include <ctime>
#include <map>
#include <functional>
#include <mutex>

const int tableSize = 3;
const int messageSize = 30;
std::vector<std::vector<std::string>> table(tableSize, std::vector<std::string>(tableSize));

std::mutex tableMutex;
std::mutex fileMutex;

int port = 1030;
int MAX_PORT = 10000;

int clientsCount = 0;


std::ofstream logFile;

std::map<std::string, std::function<bool(int x, int y, const std::string &text, const int clientServerSocFD,
                                         const int &clientID)>> commands;

bool addRequest(int x, int y, const std::string &text, const int clientServerSocFD, const int &clientID);

bool changeRequest(int x, int y, const std::string &text, const int clientServerSocFD, const int &clientID);

bool deleteRequest(int x, int y, const std::string &text, const int clientServerSocFD, const int &clientID);

bool who(int x, int y, const std::string &text, const int clientServerSocFD, const int &clientID);

bool end(int x, int y, const std::string &text, const int clientServerSocFD, const int &clientID);

void initCommandFunc() {
    commands["ADD"] = addRequest;
    commands["CHANGE"] = changeRequest;
    commands["DELETE"] = deleteRequest;
    commands["WHO"] = who;
    commands["END"] = end;

}

void serverConfiguration(int &sockfd, sockaddr_in &serv_addr, int &bindSocFD) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cout << "ERROR opening socket \n";
        throw (0);
    }
    while (port < MAX_PORT) {
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);
        serv_addr.sin_family = AF_INET;
        bindSocFD = bind(sockfd, (sockaddr *) &serv_addr, sizeof(serv_addr));
        if (bindSocFD < 0) {
            port++;
            // std::cout << "Port: " << port << " close\n";
            std::cout << bindSocFD << "\n";
        } else {
            std::cout << "Port: " << port << " bind\n";
            break;
        }
    }

    if (port > MAX_PORT) {
        throw (-1);
    }
}

bool who(int x, int y, const std::string &text, const int clientServerSocFD, const int &clientID) {
    std::lock_guard<std::mutex> fileLK(fileMutex);
    logFile << "#" << clientID << ":\nRequest: "<< text <<"\n" << std::flush;
    std::string answer = "Server was created by Danil Spitsyn from K-27\n\n";
    send(clientServerSocFD, answer.c_str(), answer.size(), 0);

    return 0;
}

bool addRequest(int x, int y, const std::string &text, const int clientServerSocFD, const int &clientID) {
    std::lock_guard<std::mutex> lock(tableMutex);
    logFile << "#" << clientID << ":\nRequest: ADD, " << x << ", " << y << ", " << text << "\n\n" << std::flush;
    try {
        if (x > 3 || x < 1 || y > 3 || y < 1) throw (-1);
        if ((table[x - 1][y - 1] + text).size() <= messageSize) {
            table[x - 1][y - 1] += text;
            std::string answer = "Text was completely add\n";
            send(clientServerSocFD, answer.c_str(), answer.size(), 0);
            return 1;
        }
    }
    catch(int){
        std::string answer = "ERROR: Invalid parametrs";
        send(clientServerSocFD, answer.c_str(), answer.size(), 0);
        logFile << "#" << clientID << ":\nInvalid parametrs\n\n" << std::flush;
        return 0;
    }
    std::string answer = "ERROR: Text will be too long";
    send(clientServerSocFD, answer.c_str(), answer.size(), 0);
    return 0;
}

bool changeRequest(int x, int y, const std::string &text, const int clientServerSocFD, const int &clientID) {
    std::lock_guard<std::mutex> lock(tableMutex);
    logFile << "#" << clientID << ":\nRequest: CHANGE, " << x << ", " << y << ", " << text << "\n\n" << std::flush;
    try {
        if (x > 3 || x < 1 || y > 3 || y < 1) throw (-1);
        if (text.size() <= messageSize) {
            //   table[x-1][y-1].clear();
            table[x - 1][y - 1] = text;
            std::string answer = "Text was completely change\n";
            send(clientServerSocFD, answer.c_str(), answer.size(), 0);
            return 1;
        }
    }
    catch(int){
            std::string answer = "ERROR: Invalid parametrs";
            send(clientServerSocFD, answer.c_str(), answer.size(), 0);
            logFile << "#" << clientID << ":\nInvalid parametrs\n\n" << std::flush;
            return 0;
    }
    std::string answer = "ERROR: Text is too long";
    send(clientServerSocFD, answer.c_str(), answer.size(), 0);

    return 0;
}

bool deleteRequest(int x, int y, const std::string &text, const int clientServerSocFD, const int &clientID) {
    std::lock_guard<std::mutex> lock(tableMutex);
    logFile << "#" << clientID << ":\nRequest: DELETE, " << x << ", " << y << "\n\n" << std::flush;
    table[x - 1][y - 1].clear();

    std::string answer = "Text was completely delete\n";
    send(clientServerSocFD, answer.c_str(), answer.size(), 0);
    return 1;
}

bool end(int x, int y, const std::string &text, const int clientServerSocFD, const int &clientID) {
    std::lock_guard<std::mutex> lock(tableMutex);
    logFile << "#" << clientID << ":\nRequest: END " << "\n\n" << std::flush;
    std::string answer = "Bye Bye\n";
    send(clientServerSocFD, answer.c_str(), answer.size(), 0);
    return 1;
}

std::vector<std::string> requestParsing(std::string request, int &_argc) {
    //request.erase(remove_if(request.begin(), request.end(), isspace), request.end());
    _argc = 0;
    std::vector<std::string> _argv;
    std::string subStr;
    for (int i = 0; i < request.size(); ++i) {
//        if (request == "END" || request == "WHO") {
//            _argv.push_back(request);
//            _argv.push_back("0");
//            _argv.push_back("0");
//            _argv.push_back(request);
//
//        }
        if (request[i] == ',') {
            _argv.push_back(subStr);
            _argv[_argc].erase(remove_if(_argv[_argc].begin(), _argv[_argc].end(), isspace), _argv[_argc].end());
            subStr.clear();
            _argc++;
        } else {
            subStr += request[i];
        }
    }
    _argv.push_back(subStr);
    subStr.clear();
    _argc++;
    if (_argv.size() == 1){
        _argv.push_back("0");
        _argv.push_back("0");
        _argv.push_back(request);
        _argc = 4;
    }
//    _argv.erase(_argv.begin()+_argc+1);
    // _argv.push_back(subStr);
    return _argv;
}

void _argvShow(std::vector<std::string> _argv) {
    for (int i = 0; i < _argv.size(); ++i) {
        std::cout << _argv[i] << " ";
    }
    std::cout << "\n";
}

void showTable() {
    std::lock_guard<std::mutex> lock(tableMutex);
    std::cout << "\n\n ###NEW TABLE\n";

    for (int i = 0; i < tableSize; ++i) {
        std::cout
                << "-------------------------------------------------------------------------------------------------\n";
        for (int j = 0; j < tableSize; ++j) {
            std::string text = table[i][j];
            std::cout << "|" << text;
            // std::cout<<"Text size: "<<text.size()<<"\n";
            for (int x = 0; x < 31 - text.size() - 1; ++x) {
                std::cout << ".";
            }
            //std::cout<<"|";
        }
        std::cout << "|";
        std::cout << std::endl;
    }
    std::cout << "-------------------------------------------------------------------------------------------------\n";
}

std::string getIP(sockaddr_in &client) {
    std::lock_guard<std::mutex> fileLK(tableMutex);
    uint32_t ip = client.sin_addr.s_addr;
    /*sprintf(buf, "%d.%d.%d.%d", reinterpret_cast<uint8_t*>(&ip)[0],
            reinterpret_cast<uint8_t*>(&ip)[1],
            reinterpret_cast<uint8_t*>(&ip)[2],
            reinterpret_cast<uint8_t*>(&ip)[3]
            );*/
    return std::to_string(reinterpret_cast<uint8_t *>(&ip)[0]) + "." +
           std::to_string(reinterpret_cast<uint8_t *>(&ip)[1]) + "." +
           std::to_string(reinterpret_cast<uint8_t *>(&ip)[2]) + "." +
           std::to_string(reinterpret_cast<uint8_t*>(&ip)[3]);
}

void conversationWithClientThread(const int clientServerSocFD, const int clientID) {
    int res;
    bool needStop = false;
    while (!needStop) {

        std::string requestString;
        int sz = 512;
        char buf[sz]{0};
        std::string s;
        res = recv(clientServerSocFD, buf, sz, 0);
        requestString += buf;
        requestString.erase(requestString.end() - 1);

        if (res == 0){
            fileMutex.lock();
            logFile << "#" << clientID << "\n" << "Client dropped connection\n\n" << std::flush;
            fileMutex.unlock();
            break;
        }

        int _argc;
        std::vector<std::string> _argv = requestParsing(requestString, _argc);
        std::cout << "Coordinates: \n";
        std::cout << _argv[1] << " " << _argv[2] << "\n";

        std::cout << "TEXT:\n";
        std::cout << _argv[3] << "\n";
        try {
            bool isComSuccess = commands[_argv[0]](std::stoi(_argv[1]), std::stoi(_argv[2]), _argv[3],
                                                   clientServerSocFD, clientID);
        } catch (std::exception) {
            tableMutex.lock();
            logFile << "#" << clientID << ":\nIncorrect request\n" << std::flush;
            tableMutex.unlock();
            std::string answer = "Incorrect request\n";
            send(clientServerSocFD, answer.c_str(), answer.size(), 0);
        }
        if (_argv[0] == "END") {
            needStop = true;
        }
        showTable();

        /*std::string answer;
        if (isComSuccess){
            answer = "1";
        }
        else*/
    }
    close(clientServerSocFD);

}

int acceptServerThread() {
    int sockfd;
    sockaddr_in serv_addr;
    sockaddr_in client_addres;

    int bindSocFD;

    try {
        serverConfiguration(sockfd, serv_addr, bindSocFD);
    }
    catch (int) {
        return 0;
    }

    if (listen(sockfd, 10) < 0) {
        logFile << "Error we cannot listen the port\n" << std::flush;
        return 0;
    }

    socklen_t clilen = sizeof(client_addres);

    int clientID;

    while (1) {
        int newSocketFD = accept(sockfd, (sockaddr *) &client_addres, &clilen);
        if (newSocketFD < 0) {
            logFile << "Accept error\n" << std::flush;
            // close(sockfd);
            return -2;
        }
        clientID = ++clientsCount;
        std::time_t time = std::time(0);
        std::tm *time_now = std::localtime(&time);
        std::mktime(time_now);

        logFile << std::asctime(time_now) << "Connection accept for client with IP " << getIP(client_addres) << " ID: #" << clientID << "\n\n"
                << std::flush;


        std::thread serverCommunication(conversationWithClientThread, newSocketFD, clientID);
        serverCommunication.detach();

        //close (newSocketFD);
    }

    // std::cout << serverAnswer << "\n";

    close(sockfd);

    return 0;
}

int main() {
    initCommandFunc();

    logFile.open("server-log.txt", std::ios::app);
    if (logFile.is_open()) {
        logFile << "Server started\n" << std::flush;
    } else {
        std::cout << "File is not open\n";
    }

    int err = acceptServerThread();

    logFile << "Server end\n" << std::flush;

    logFile.close();
    return 0;
}
