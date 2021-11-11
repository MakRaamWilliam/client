#include <iostream>
#include <stdio.h>
//#include <winsock2.h>
#include "Ws2tcpip.h"
#include <ws2tcpip.h>
#include <windows.h>
//#include <arpa/inet.h>

#include <stdlib.h>
#include <unistd.h>
#include <bits/stdc++.h>

#pragma comment(lib, "Ws_.lib")

#define  MAXDATABUF 500000

using namespace std;

void send_post(int sock, string command);
void send_get(int sock, string command);
void parse_requests(int sock);

// from https://stackoverflow.com/questions/15660203/inet-pton-identifier-not-found/15660299
int inet_pton(int af, const char *src, void *dst)
{
    struct sockaddr_storage ss;
    int size = sizeof(ss);
    char src_copy[INET6_ADDRSTRLEN+1];

    ZeroMemory(&ss, sizeof(ss));
    /* stupid non-const API */
    strncpy (src_copy, src, INET6_ADDRSTRLEN+1);
    src_copy[INET6_ADDRSTRLEN] = 0;

    if (WSAStringToAddress(src_copy, af, NULL, (struct sockaddr *)&ss, &size) == 0) {
        switch(af) {
            case AF_INET:
                *(struct in_addr *)dst = ((struct sockaddr_in *)&ss)->sin_addr;
                return 1;
            case AF_INET6:
                *(struct in6_addr *)dst = ((struct sockaddr_in6 *)&ss)->sin6_addr;
                return 1;
        }
    }
    return 0;
}

vector<string> splitwithdel(string command, char del){

    stringstream commandstr(command);
    string s;
    vector<string> lines;
    while(getline(commandstr, s, del))
        lines.push_back(s);

    return lines;
}

string getdata(string command, string del){
    size_t pos = 0;
    while ((pos = command.find(del)) != std::string::npos) {
        command.erase(0, pos + del.length());
    }
    return command ;
}


int main(int argc, char const *argv[]){

    WSAData data;
    WORD ver = MAKEWORD(2, 2);
    int wsResult = WSAStartup(ver, &data);
    if (wsResult != 0)
    {
        cerr << "Can't start Winsock, Err #" << wsResult << endl;
        return 0;
    }

    char *servIP = "127.0.0.1";
    int servPort = 5000;
    if (argc > 2){
        strcpy(servIP, argv[1]);
        servPort = atoi(argv[2]);
    }
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        cout<<"Error in Socket creation\n";
        return -1;
    }

    struct sockaddr_in servAddr;
    memset(&servAddr, '0', sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(servPort);
    int rtnVal = inet_pton(AF_INET, servIP, &servAddr.sin_addr.s_addr);
    if(rtnVal <= 0 )
    {
        cout <<"Invalid address/ Address not supported \n";
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
    {
        cout <<"Connection Failed \n";
        return -1;
    }

    ifstream commandsfile("C:\\Users\\makrm\\CLionProjects\\client\\commands\\commands.txt");
    string command = "";
    while (getline (commandsfile, command)) {
        if(splitwithdel(command, ' ')[0] == "client-get"){
            send_get(sock, command);
        } else if(splitwithdel(command, ' ')[0] == "client-post") {
            send_post(sock, command);
        }else{
            cout<<"invalid command\n";
            break;
        }
    }
    commandsfile.close();
    close(sock);

//    string userInput;
//    do{
//        cout<<">";
//        char buf[4096];
//        getline(cin, userInput);
//        // Send the text
//        int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
//        if (sendResult != SOCKET_ERROR)
//        {
//            // Wait for response
//            ZeroMemory(buf, 4096);
//            int bytesReceived = recv(sock, buf, 4096, 0);
//            if (bytesReceived > 0)
//            {
//                // Echo response to console
//                cout << "SERVER> " << string(buf, 0, bytesReceived) << endl;
//            }
//        }
//    } while(userInput.size() > 0);		// Make sure the user has typed in something
//    closesocket(sock);


    return 0;
}




void send_get(int sock, string command){
    string request="";
    string mypath = "C:\\Users\\makrm\\CLionProjects\\client\\gets\\";
    vector<string> tokens = splitwithdel(command, ' ');
    if(tokens.size() < 2){
        cout<<"invalid command\n";
        return;
    }
    string file_path = tokens[1];
    request = "GET "+ file_path + " HTTP/1.1\r\n\r\n";
    int sendBytes = send(sock ,&request[0] ,request.size() ,0 );
    if(sendBytes== 0){
        cout<<"failed in send\n";
        return;
    }
    cout << ">>>>>>>>>>>>> Request Sent "<<sendBytes<<" Bytes \n" << request << "\n";
    char buff[MAXDATABUF] = {0};
    int recBytes = recv(sock, buff, MAXDATABUF,0);
    if(recBytes == 0){
        cout<<"failed in rec\n";
        return;
    }
    cout << ">>>>>>>>>>>>> Response Received " <<recBytes<<" Bytes\n"<< string(buff,recBytes) << "\n";
    string  buffstr = string(buff,recBytes);
    if(buffstr.substr(9, 3) == "200"){
        file_path = mypath + splitwithdel(file_path, '/').back();
        ofstream file(file_path,std::ios::binary); //stdop
        string data =  getdata(buffstr, "\r\n\r\n");
        file << data.substr(0,data.size()-1);
        file.close();
    }
}

void send_post(int sock, string command){
    string mypath = "C:\\Users\\makrm\\CLionProjects\\client\\posts";
    vector<string> tokens = splitwithdel(command, ' ');
    if(tokens.size() < 2){
        cout<<"Invalid command\n";
        return;
    }
    string file_path = tokens[1];
    string request = "POST "+ file_path + " HTTP/1.1\n";
    file_path = mypath + file_path;
    if(splitwithdel(file_path, '.').size() <2){
        cout<<"Invalid command\n";
        return;
    }
    string type = splitwithdel(file_path, '.')[1];
    if (type == "txt"){
        type = "Content-Type: text/plain\n";
    } else if(type == "html"){
        type = "Content-Type: text/html\n";
    } else {
        if(type == "jpg") type = "jpeg";
        type = "Content-Type: image/" + type + "\n";
    }
    ifstream file(file_path,std::ios::binary);//in bin
    if(file.good()) {
        string datafile;
        string temp = "";
        while (getline(file, temp)) {
            datafile += temp + "\n";
        }
        request += type;
        request += "Content-Length: ";
        request += to_string(datafile.size());
        request += "\r\n\r\n" + datafile;
        file.close();
        int sendBytes = send(sock, &request[0], request.size(), 0);
        cout << ">>>>>>>>>>>>> Request Sent "<<sendBytes<<" Bytes\n" << request << "\n";
        char buffer[MAXDATABUF] = {0};
        int recBytes = recv(sock, buffer, MAXDATABUF, 0);
        if(recBytes > 0 )
            cout << "=========== Response Received "<<recBytes<<" Bytes\n"<< string(buffer,recBytes) << "\n";
    }else{
        cout<<"Invalid post\n";
    }
}


