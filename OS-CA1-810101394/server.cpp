#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <poll.h>
#include <unistd.h>
#include <map>
#include <algorithm>
#include <utility>
#include <memory>
#include <iostream>
#include <signal.h>
#include <sys/time.h>
#include <errno.h>
#include <csignal>
#include <array>
#include <sstream>

#define STDIN 0
#define STDOUT 1
#define BUFFER_SIZE 1024
#define BROADCAST_IP "127.255.255.255"
#define BACKSLASH_N '\n'

const char* UP_SERVER = "Server is up!\n";
const char* NEW_CONNECTION = "New Connection has found!\n";
const char* ENTER_NAME = "Please enter your name: ";
const char* CHOOSE_ROOM = "Choose a room number: ";
const char* ROOM_IS_FULL = "The selected room is full. Please choose another room.\n";
const char* WAIT_FOR_ROOM = "Waiting for another player to join...\n";
const char* GAME_STARTED_IN_A_ROOM = "Choose between these (paper , rock , scissors): ";
const char* GAME_WINNER = "\nGame finished! The winner is: \n";
const int TIME = 10;
const int ROOM_CAPACITY = 2;

using namespace std;

class Room;
int current_timer_id = 0;
vector<Room*> rooms;
int sock;
struct sockaddr_in bc_address;

class Client {
    private:
        int client_fd;
        string name;
        bool has_name;
        bool is_playing;
        int room_assigned;
        int room_port_assigned;
        int losses;
        int draws;
        int wins;
    public:
        Client(int _clinet_fd, int _room_port_assigned) {
            client_fd = _clinet_fd;
            room_port_assigned = _room_port_assigned;
            has_name = false;
            is_playing = false;
            losses = 0, wins = 0, draws = 0;
        }
        bool has_gotten_name() {
            return has_name;
        }
        bool is_client_playing() {
            return is_playing;
        }
        void set_name(string _name) {
            name = _name; 
            has_name = true;
        }
        void set_fd(int _clinet_fd) {
            client_fd = _clinet_fd;
        }
        void set_playing_mode(bool mode, int room_id) {
            is_playing = mode; 
            room_assigned = room_id;
        }
        int get_room() {
            return room_assigned;
        }
        int get_fd() {
            return client_fd;
        }
        int get_assigned_room_port() {
            return room_port_assigned;
        }
        string get_name() {
            return name;
        }
        int get_wins() {
            return wins;
        }
        int get_losses() {
            return losses;
        }
        int get_draws() {
            return draws;
        }
        void add_wins() {
            wins += 1;
        }
        void add_losses() {
            losses += 1;
        }
        void add_draws() {
            draws += 1;
        }
};

vector<Client*> clients;

class Room {
private:
    int room_fd;
    int room_port;
    vector<Client*> clients;
    vector<struct pollfd> pfds;
    pair<string, string> answers;
public:
    Room(int _room_fd, int _room_port) {
        room_fd = _room_fd;
        room_port = _room_port;
        answers = {"",""};
        pfds.push_back(pollfd{room_fd, POLLIN, 0});
    }
    int get_fd() {
        return room_fd;
    }
    int get_port() {
        return room_port;
    }
    int get_number_of_clients() {
        return clients.size();
    }
    void add_client(Client* client) {
        clients.push_back(client);
    }
    void send_message_to(const char* message, int index) {
        write(clients[index]->get_fd(), message, strlen(message));
    }
    void set_answer(int fd, string ans);
    bool is_game_end() {
        return (answers.first != "" && answers.second != "");
    }
    void clear_rooms();
    int get_client_index_start(int idx) {
        return clients[idx]->get_assigned_room_port();
    };
    vector<Client*> get_clients() {
        return clients;
    }
    int get_last_client_index_start();
    int guess_the_winner();
    string print_available_rooms(const vector<Room*> &all_rooms, int self_id);
    void game_control(int self_id);
    void send_all_clients_in_room(const char* message);
};

map<int, timer_t> active_timers;
map<int, int> map_timer;

void print_error_message(const char* msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int acceptClient(int server_fd) {
    int new_fd;
    struct sockaddr_in client_address;
    int client_address_len = sizeof(client_address);
    new_fd = accept(server_fd, (struct sockaddr *)&client_address, (socklen_t*) &client_address_len);
    return new_fd;
}

int setup_server(char* ipaddr, int port) {
    struct sockaddr_in server_addr;
    int server_fd, opt = 1;

    server_addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, ipaddr, &(server_addr.sin_addr)) < 0)
        print_error_message("FAILED: Input ipv4 address invalid");

    if((server_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        print_error_message("FAILED: Socket was not created");

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
        print_error_message("FAILED: Making socket reusable failed");

    if(setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) < 0)
        print_error_message("FAILED: Making socket reusable port failed");

    memset(server_addr.sin_zero, 0, sizeof(server_addr.sin_zero));
    server_addr.sin_port = htons(port);

    if(bind(server_fd, (const struct sockaddr*)(&server_addr), sizeof(server_addr)) < 0)
        print_error_message("FAILED: Bind unsuccessfull");

    if(listen(server_fd, 10) == -1)
        print_error_message("FAILED: Listen unsuccessfull");

    return server_fd;
}

void Room::set_answer(int fd, string ans) {
    if (fd == clients[0]->get_fd() && answers.first == "") answers.first = ans;
    if (fd == clients[1]->get_fd() && answers.second == "") answers.second = ans;
}

void Room::clear_rooms() {
    clients.clear();
    for (int i = 0; i < clients.size(); i++) {
        clients[i]->set_playing_mode(false, -1);
        answers = {"", ""};
    }
}

int Room::get_last_client_index_start() {
    if (clients.size() == 1) return clients[0]->get_assigned_room_port();
    if (clients.size() == 2) return clients[1]->get_assigned_room_port();
    return -1;
}

void Room::send_all_clients_in_room(const char* message) {
    for (int i = 0; i < clients.size(); i++)
        send_message_to(message, i);
}

int Room::guess_the_winner() {
    if (answers.first == "" && answers.second == "")
        return -1;
    else if (answers.first != "" && answers.second == "")
        return 0;
    else if (answers.first == "" && answers.second != "")
        return 1;
    
    if (answers.first == answers.second)
        return -2;
    else if (answers.first == "paper") {
        if (answers.second == "rock") {
            return 0;
        }
        else if (answers.second == "scissors") {
            return 1;
        }
    }
    else if (answers.first == "rock")
        if (answers.second == "paper")
            return 1;
        else if (answers.second == "scissors")
            return 0;
    else if (answers.first == "scissors") {
        if (answers.second == "paper")
            return 0;
        else if (answers.second == "rock")
            return 1;
    }

    if (answers.first == answers.second)
        return -2;
    else if (answers.second == "paper") {
        if (answers.first == "rock") {
            return 1;
        }
        else if (answers.first == "scissors") {
            return 0;
        }
    }
    else if (answers.second == "rock")
        if (answers.first == "paper")
            return 0;
        else if (answers.first == "scissors")
            return 1;
    else if (answers.second == "scissors") {
        if (answers.first == "paper")
            return 1;
        else if (answers.first == "rock")
            return 0;
    }

    return -2;    
}

string Room::print_available_rooms(const vector<Room*> &all_rooms, int self_id) {
    string room_list = "List of Rooms:\n";
    for (int i = 0; i < all_rooms.size(); i++) {
        if (i == self_id) {
            room_list += "Room " + to_string(i + 1) + " (capacity is 2)\n";
            continue;
        }
        int room_size = all_rooms[i]->get_number_of_clients();
        string available_msg = " (capacity is " + to_string(2 - room_size) + ")\n";
        string full_msg = " (full)\n";
        room_list += "Room " + to_string(i + 1) + (room_size < 2 ? available_msg : full_msg);
    }
    return room_list;
}

void Room::game_control(int self_id) {
    int winner_result = guess_the_winner();
    if (winner_result == -1) {
        string msg = "The game is over due to time pass.\n";
        send_all_clients_in_room(msg.c_str());
        clients[0]->add_draws();
        clients[1]->add_draws();
    }
    else if (winner_result == 0) {
        string win_res = GAME_WINNER + clients[0]->get_name();
        send_all_clients_in_room(win_res.c_str());
        clients[0]->add_wins();
        clients[1]->add_losses();
    }
    else if (winner_result == 1) {
        string win_res = GAME_WINNER + clients[1]->get_name();
        send_all_clients_in_room(win_res.c_str());
        clients[0]->add_losses();
        clients[1]->add_wins();
    }
    else if (winner_result == -2) {
        string msg = "The game was ended in a draw.\n";
        send_all_clients_in_room(msg.c_str());
        clients[0]->add_draws();
        clients[1]->add_draws();
    }
    string room_list = print_available_rooms(rooms, self_id);
    send_all_clients_in_room(room_list.c_str());
    send_all_clients_in_room(CHOOSE_ROOM);
    clear_rooms();
}

string print_available_rooms(vector<Room*> &all_rooms) {
    string room_list = "Rooms:\n";
    for (int i = 0; i < all_rooms.size(); i++) {
        int room_size = all_rooms[i]->get_number_of_clients();
        string available_msg = " (" + to_string(2 - room_size) + " Available)\n";
        string full_msg = " (Full)\n";
        string temp;
        if (room_size < 2)
            temp = available_msg;
        else
            temp = full_msg;
        room_list += "Room " + to_string(i + 1) + temp;
    }
    return room_list;
}

void handle_room_timer(int signum, siginfo_t *si, void *uc) {
    int timer_id = si->si_value.sival_int;  
    int room_id = map_timer[timer_id];
    string message = "Timer expired for Room: " + to_string(room_id) + "\n";
    write(STDOUT, message.c_str(), strlen(message.c_str()));
    rooms[room_id]->game_control(room_id);
    map_timer.erase(timer_id);      
    active_timers.erase(timer_id);      
}

void set_room_timer(int room_id, int seconds) {
    struct sigevent sev;
    struct itimerspec its;
    timer_t timer_id;
    current_timer_id++;
    map_timer[current_timer_id] = room_id;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGRTMIN;  
    sev.sigev_value.sival_int = current_timer_id;  
    timer_create(CLOCK_REALTIME, &sev, &timer_id);
    its.it_value.tv_sec = seconds;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;  
    its.it_interval.tv_nsec = 0;
    timer_settime(timer_id, 0, &its, NULL);
    active_timers[current_timer_id] = timer_id;
}

void remove_room_timer(int room_id) {
    for (const auto& entry : map_timer) {
        if (entry.second == room_id) {
            int timer_id = entry.first;  
            timer_t timer_to_delete = active_timers[timer_id];
            if (timer_delete(timer_to_delete) == -1) {
                print_error_message("delete timer");
            } else {
                string message = "Timer canceled for Room: " + to_string(room_id) + "\n";
                write(STDOUT, message.c_str(), strlen(message.c_str()));
            }
            map_timer.erase(timer_id);
            active_timers.erase(timer_id);
            return;
        }
    }
}

void register_signal_handler() {
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_room_timer;
    sigemptyset(&sa.sa_mask);
   
    if (sigaction(SIGRTMIN, &sa, NULL) == -1)
        print_error_message("sigaction");
}

int calculate_client_ind(int i, int client_index_start) {
    return i - client_index_start;
}

void end_game(vector<Client*>& clients, int sock, struct sockaddr_in bc_address) {
    string game_summary = "\nGame Summary:\n";
    for (const auto& client : clients) {
        game_summary += "Player: " + client->get_name() +
                        "\nWins: " + to_string(client->get_wins()) +
                        "\nLosses: " + to_string(client->get_losses()) +
                        "\nDraws: " + to_string(client->get_draws()) + "\n\n";
    }
    write(STDOUT, game_summary.c_str(), strlen(game_summary.c_str()));
    for (const auto& client : clients)
        sendto(sock, game_summary.c_str(), strlen(game_summary.c_str()), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
    for (auto& client : clients)
        close(client->get_fd());
    exit(EXIT_SUCCESS);
}

void check_room(vector<Client*> clients, int client_ind, vector<struct pollfd> pfds, int i, int room_choice) {
    if (rooms[room_choice]->get_number_of_clients() >= ROOM_CAPACITY) {
        write(pfds[i].fd, ROOM_IS_FULL, strlen(ROOM_IS_FULL));
        string room_list = print_available_rooms(rooms);
        write(pfds[i].fd, room_list.c_str(), strlen(room_list.c_str()));
        write(pfds[i].fd, ROOM_IS_FULL, strlen(ROOM_IS_FULL));
    }
    else {
        clients[client_ind]->set_playing_mode(true, room_choice);
        rooms[room_choice]->add_client(clients[client_ind]);
        string room_port = "Room: " + to_string(rooms[room_choice]->get_port());
        write(pfds[i].fd, room_port.c_str(), strlen(room_port.c_str()));
    }
}

void check_answer(string ans, int room_number, const vector <Room*>&rooms, vector<struct pollfd>pfds, int i, vector<Client*>& clients, int sock, struct sockaddr_in bc_address) {
    rooms[room_number]->set_answer(pfds[i].fd, ans);
    if (rooms[room_number]->is_game_end()) {
        remove_room_timer(room_number);
        rooms[room_number]->game_control(room_number);
    }
}

int main(int argc, char* argv[])
{
    register_signal_handler();
    if(argc != 4)
        print_error_message("Invalid Arguments");

    char* ipaddr = argv[1];
    int port = stoi(argv[2]);
    int number_of_rooms = atoi(argv[3]);

    int server_fd, room_server_fd;
    int client_index_start = number_of_rooms + 2;
    
    vector<struct pollfd> pfds;

    server_fd = setup_server(ipaddr, port);
    write(1, UP_SERVER, strlen(UP_SERVER));
    pfds.push_back(pollfd{server_fd, POLLIN, 0});
    pfds.push_back(pollfd{STDIN, POLLIN, 0});

    for (int i = 0; i < number_of_rooms; i++) {
        room_server_fd = setup_server(ipaddr, port + i + 1);
        pfds.push_back(pollfd{room_server_fd, POLLIN, 0});
        rooms.push_back(new Room(room_server_fd, port + i + 1));
        // printf("%d\n", room_server_fd);
    }

    int broadcast = 1, opt = 1;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
    setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    bc_address.sin_family = AF_INET; 
    bc_address.sin_port = htons(port); 
    bc_address.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    bind(sock, (struct sockaddr *)&bc_address, sizeof(bc_address));

    map <string, int> res;
    res["paper"] = 1;
    res["rock"] = 2;
    res["scissors"] = 3;

    while(1) {
        int poll_result = poll(pfds.data(), (nfds_t)(pfds.size()), -1);
        for(size_t i = 0; i < pfds.size(); i++) {
            if(pfds[i].revents & POLLIN) {
                if (pfds[i].fd == STDIN) {
                    char buffer[BUFFER_SIZE];
                    fgets(buffer, BUFFER_SIZE, stdin);
                    string command(buffer);
                    command.erase(remove(command.begin(), command.end(), BACKSLASH_N), command.end());
                    if (command == "end_game") {
                        end_game(clients, sock, bc_address);
                    }
                }
                if(pfds[i].fd == server_fd) {
                    int new_client_fd = acceptClient(server_fd);
                    // printf("new client %d\n", new_client_fd);
                    write(STDOUT, NEW_CONNECTION, strlen(NEW_CONNECTION));
                    pfds.push_back(pollfd{new_client_fd, POLLIN, 0});
                    clients.push_back(new Client(new_client_fd, pfds.size() - 1));
                    write(new_client_fd, ENTER_NAME, strlen(ENTER_NAME));
                    continue;
                }
                for(size_t j = 0; j < rooms.size(); j++) {
                    if(pfds[i].fd == rooms[j]->get_fd()) {
                        int new_fd = acceptClient(rooms[j]->get_fd());
                        int last_client_index = rooms[j]->get_last_client_index_start();
                        pfds[last_client_index] = pollfd{new_fd, POLLIN, 0};
                        clients[last_client_index - client_index_start]->set_fd(new_fd);
                        if (rooms[j]->get_number_of_clients() == 1)
                            rooms[j]->send_message_to(WAIT_FOR_ROOM, 0);
                        else if (rooms[j]->get_number_of_clients() == 2) {
                            sendto(sock, GAME_STARTED_IN_A_ROOM, strlen(GAME_STARTED_IN_A_ROOM), 0,(struct sockaddr *)&bc_address, sizeof(bc_address));
                            set_room_timer(j, TIME);
                        }
                        continue;
                    }
                }
                int client_ind = calculate_client_ind(i, client_index_start);
                if (client_ind >= 0) {
                    if (clients[client_ind]->is_client_playing() == true) {
                        int room_number = clients[client_ind]->get_room();
                        char buffer[BUFFER_SIZE];
                        memset(buffer, 0, BUFFER_SIZE);
                        recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                        string ans_str = buffer;
                        ans_str.erase(remove(ans_str.begin(), ans_str.end(), BACKSLASH_N), ans_str.end());
                        check_answer(ans_str, room_number, rooms, pfds, i, clients, sock, bc_address);
                        continue;
                    }
                    else {
                        char buffer[BUFFER_SIZE];
                        memset(buffer, 0, BUFFER_SIZE);
                        recv(pfds[i].fd, buffer, BUFFER_SIZE, 0);
                        if (clients[client_ind]->has_gotten_name() == false) {
                            clients[client_ind]->set_name(string(buffer)); 
                            string room_list = print_available_rooms(rooms);
                            write(pfds[i].fd, room_list.c_str(), strlen(room_list.c_str()));
                            write(pfds[i].fd, CHOOSE_ROOM, strlen(CHOOSE_ROOM));
                        }
                        else {
                            int room_choice = atoi(buffer) - 1;
                            check_room(clients, client_ind, pfds, i, room_choice);
                        }
                    }
                }
            }
        }
    }
}
