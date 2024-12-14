#include <bits/stdc++.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <cctype>
#include "logger.hpp"
#include "const.hpp"

using namespace std;

struct Item {
    string name;
    double unit_price;
    double quantity;
    string type;
};

Logger logger("warehouse");
vector<Item*> items;
char buffer[1024];
int pipe_fd_parent_to_child;
int pipe_fd_child_to_parent;

void read_csv_and_make_items(const string& filePath, vector<Item*> &items) {
    ifstream file(filePath);
    if (file.is_open()) {
        string line;
        while (getline(file, line)) {
            stringstream ss(line);
            string name, price, quantity, type;
            getline(ss, name, ',');
            getline(ss, price, ',');
            getline(ss, quantity, ',');
            getline(ss, type);
            type.erase(remove(type.begin(), type.end(), '\r'), type.end());

            double unit_price = stod(price);
            double qty = stod(quantity);
            Item* temp = new Item({name, unit_price, qty, type});
            items.push_back(temp);
        }
        file.close();
    } 
    else
        logger.error("Error: Unable to open file " + filePath);
}

double calculate_profit(vector<Item*>& items, string name_warehouse) {
    double profit = 0;

    for (int i = 0; i < int(items.size()); i++) {
        if (items[i]->type == "output") {
            for (int j = 0; j < i; j++) {
                if ((items[j]->name == items[i]->name) && (items[j]->type == "input")) {
                    if (items[j]->quantity >= items[i]->quantity) {
                        double diff = items[i]->unit_price - items[j]->unit_price;
                        profit += diff * items[i]->quantity;
                        items[j]->quantity = items[j]->quantity - items[i]->quantity;
                        break;
                    }
                    else {
                        double diff = items[i]->unit_price - items[j]->unit_price;
                        profit += diff * items[j]->quantity;
                        items[i]->quantity -= items[j]->quantity; 
                        items[j]->quantity = 0;
                    }
                }
                else {
                    continue;
                }
            }
        }
    }
    logger.info("Profit is calculated in " + name_warehouse);
    return profit;
}

vector<string> create_filepath_items(string& buffer) {
    vector<string> result;
    stringstream ss(buffer);
    string token;

    while (getline(ss, token, DOLLAR_SIGN))
        result.push_back(token);
    return result;
}

string create_message_to_send_to_item(vector<Item*> items , string name_item) {
    string res = "";
    for (int i = 0; i < int(items.size()); i++)
        if (items[i]->name == name_item)
            res = res + to_string(items[i]->quantity) + PERCENT_SIGN + to_string(items[i]->unit_price) + PERCENT_SIGN + items[i]->type + DOLLAR_SIGN;
    return res;
}

void send_data_to_ietm(string fifo_name , string msg){
    int fd = open(fifo_name.c_str(), O_WRONLY);
    logger.info(fifo_name + " is opened successfully");
    write(fd, msg.c_str(), int(msg.size())+1);
    logger.info("message is sent to " + fifo_name + " successfully");
    close(fd);
    return;
}

string get_file_name(const string& filePath) {
    size_t last_slash = filePath.find_last_of('/');
    size_t last_dot = filePath.find_last_of('.');
   
    if (last_slash != std::string::npos && last_dot != std::string::npos && last_dot > last_slash) {
        return filePath.substr(last_slash + 1, last_dot - last_slash - 1);
    }
    return "";
}

void handle_item_in_warehouse(vector<string> new_buffer, string name_warehouse) {
    for (int i = 1; i < int(new_buffer.size()); i++)
    {
        string name_fifo = name_warehouse + "_" + new_buffer[i];
        string mes = create_message_to_send_to_item(items , new_buffer[i]);
        logger.info("Creating message for " + new_buffer[i] + " is successfull");
        send_data_to_ietm(name_fifo, mes);
    }
}

void write_message_to_main(string csv_file_warehouse, int pipe_fd_child_to_parent, double profit, string name_warehouse) {
    string message = "Profit for " + csv_file_warehouse + ": " + to_string(profit);
    if (write(pipe_fd_child_to_parent, message.c_str(), int(message.size())) == -1) {
        logger.error("Write failed");
    }
    else {
        logger.info("Message sent to main successfully from" + name_warehouse);
    }
}

void get_args(char *argv[]) {
    pipe_fd_parent_to_child = atoi(argv[1]);
    pipe_fd_child_to_parent = atoi(argv[2]);
}

void close_fds() {
    close(pipe_fd_parent_to_child);
    close(pipe_fd_child_to_parent);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        logger.error("Usage: ./child1 <pipe_fd_parent_to_child> <pipe_fd_child_to_parent>");
        return 1;
    }

    get_args(argv);
    ssize_t bytesRead = read(pipe_fd_parent_to_child, buffer, sizeof(buffer) - 1);

    if (bytesRead < 0)
        logger.error("Read failed");
    
    buffer[bytesRead] = '\0';
    string filePath(buffer);
    vector<string> new_buffer;
    new_buffer = create_filepath_items(filePath);
    string csv_file_warehouse = new_buffer[0];
    logger.info("Recieving message from main was successfull. This is " + csv_file_warehouse);
    string name_warehouse = get_file_name(csv_file_warehouse);
    read_csv_and_make_items(csv_file_warehouse, items);

    handle_item_in_warehouse(new_buffer, name_warehouse);
    
    double profit = calculate_profit(items, name_warehouse);

    write_message_to_main(csv_file_warehouse, pipe_fd_child_to_parent, profit, name_warehouse);
    
    close_fds();
    return 0;
}