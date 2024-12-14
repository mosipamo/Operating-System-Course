#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <vector>
#include <iostream>
#include <string>
#include <sstream>

#define STDIN 0
#define STDOUT 1
#define BUFFER_SIZE 1024
const char* MAIN_SERVER = "Connected to the main server!";

using namespace std;

void print_error_message(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int connectServer(int port, const char* ip, int opt) {
    int server_fd;
    struct sockaddr_in server_address;

    server_address.sin_family = AF_INET; 
    if(inet_pton(AF_INET, ip, &(server_address.sin_addr)) == -1)
        print_error_message("FAILED: Input ipv4 address invalid");
    
    if((server_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
        print_error_message("FAILED: Socket was not created");
    
    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        print_error_message("FAILED: Making socket reusable failed");

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1)
        print_error_message("FAILED: Making socket reusable port failed");

    memset(server_address.sin_zero, 0, sizeof(server_address.sin_zero));
    server_address.sin_port = htons(port);
    
    server_address.sin_addr.s_addr = inet_addr(ip);

    if (connect(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
        print_error_message("Error in connecting to server");

    return server_fd;
}

vector<string> tokenizer(string line) {
    vector <string> tokens;

    stringstream check1(line);
    
    string intermediate;
    
    while(getline(check1, intermediate, ' '))
    {
        tokens.push_back(intermediate);
    }
    return tokens;
}

int main(int argc, char* argv[])
{
    if(argc != 3)
        print_error_message("Invalid Arguments");

    char buffer[BUFFER_SIZE];
    char* ipaddr = argv[1];
    int port = stoi(argv[2]);

    struct sockaddr_in server_addr;
    int server_fd, main_sevrer, opt = 1;
    vector<pollfd> pfds;

    server_fd = connectServer(port, ipaddr, opt);

    main_sevrer = server_fd;

    pfds.push_back(pollfd{server_fd, POLLIN, 0});
    pfds.push_back(pollfd{STDIN, POLLIN, 0});

    int sock, broadcast = 1;
    char buffer_b[1024] = {0};
    struct sockaddr_in bc_address;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET; 
    bc_address.sin_port = htons(port); 
    bc_address.sin_addr.s_addr = inet_addr("127.255.255.255");

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));


    pfds.push_back(pollfd{sock, POLLIN, 0});


    while(true)
    {
        if(poll(pfds.data(), (nfds_t)(pfds.size()), -1) == -1)
            print_error_message("FAILED: Poll");

        for(size_t i = 0; i < pfds.size(); i++)
        {
            if(pfds[i].revents & POLLIN)
            {
                memset(buffer, 0, BUFFER_SIZE);
                if (i == 0) {
                    recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                    string buffer_str = buffer;
                    vector<string> tokenized_words = tokenizer(buffer_str);
                    if (tokenized_words[0] == "Room:") {
                        string port_str = buffer_str.substr(5);
                        int room_port = stoi(port_str);
                        int room_fd = connectServer(room_port, ipaddr, opt);
                        pfds[0] = pollfd{room_fd, POLLIN, 0};
                        continue;
                    }
                    else if (tokenized_words[0] == "Connect") {
                        pfds[0] = pollfd{server_fd, POLLIN, 0};
                    }
                    else {
                        if (buffer_str == "end_game")
                            close(pfds[i].fd);
                        write(STDOUT, buffer, strlen(buffer));
                    }
                }
                else if (i == 1) {
                    read(STDIN, buffer, BUFFER_SIZE);
                    send(pfds[0].fd, buffer, strlen(buffer), 0);
                }
                else if(i == 2){
                    memset(buffer_b, 0, BUFFER_SIZE);
                    recv(sock, buffer_b, BUFFER_SIZE, 0);
                    write(STDOUT, buffer_b, strlen(buffer_b));
                }
            }
        }        
    }
}