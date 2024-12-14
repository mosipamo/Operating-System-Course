#include <bits/stdc++.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <cctype>
#include "logger.hpp"

using namespace std;

struct Item {
    float quantity;
    float unitPrice;
    string type;
};

int num_available = 0;
double price_available = 0;

Logger logger("item");

vector<string> create_filepath_items(string& buffer) {
    vector<string> result;
    stringstream ss(buffer);
    string token;

    while (getline(ss , token , '$'))
        result.push_back(token);
    return result;
}

string get_info(string FIFO_name){
    string result;
    int fd = open(FIFO_name.c_str(), O_RDONLY);
    if(fd < 0)
        logger.error("file descriptor wasn't acquired!");
    else
        logger.info(FIFO_name + " is opened successfully");
    char buffer[256];
    ssize_t bytes_red = read(fd, buffer, sizeof(buffer) - 1);

    if (bytes_red > 0) {
        buffer[bytes_red] = '\0';
        logger.info("Message is recieved from " + FIFO_name);
    }
    else
        logger.error("Read failed");
    close(fd);
    result = buffer;
    return result;
}

void convert_message_to_items(vector<Item*>& items, string& mes) {
    vector<string> result;
    stringstream ss(mes);
    string token;

    while (getline(ss , token , '$'))
    {
        result.push_back(token);
    }

    for (int i = 0; i < int(result.size()); i++)
    {
        vector<string> item;
        stringstream ss2(result[i]);
        string token2;

        while (getline(ss2 , token2 , '%'))
        {
            item.push_back(token2);
        }
        Item* temp = new Item({stof(item[0]), stof(item[1]), item[2]});
        items.push_back(temp);
    }
}

void calculate_availability(vector<Item*> items) {
    for (int i = 0; i < int(items.size()); i++) {
        if (items[i]->type == "output") {
            for (int j = 0; j < i; j++) {
                if ((items[j]->type == "input")) {
                    if (items[j]->quantity >= items[i]->quantity) {
                        items[j]->quantity = items[j]->quantity - items[i]->quantity;
                        items[i]->quantity = 0;
                        break;
                    }
                    else {
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




    for (int i = 0; i < int(items.size()); i++)
    {
        if (items[i]->type == "input")
        {
            num_available += items[i]->quantity;
            price_available += items[i]->quantity * items[i]->unitPrice;
        } else {
            // cout << "test " << items[i]->quantity;
            num_available -= items[i]->quantity;
            price_available -= items[i]->quantity * items[i]->unitPrice;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        logger.error("Usage: ./child1 <pipe_fd>");
        return 1;
    }

    vector<Item*> items;

    int pipe_fd_parent_to_child = atoi(argv[1]);
    int pipe_fd_child_to_parent = atoi(argv[2]);
    char buffer[10563];

    ssize_t bytesRead = read(pipe_fd_parent_to_child, buffer, sizeof(buffer) - 1);
    vector<string> new_buffer;

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        string message(buffer);
        // cout << "item: " << message << endl;

        new_buffer = create_filepath_items(message);
        string item = new_buffer[0];
        logger.info("Recieved message from parent. This is " + item);

        // cout << "new_buffer:\n";
        // for (auto i: new_buffer)
        //     cout << i << endl;


        for (int i = 1; i < int(new_buffer.size()); i++)
        {
            string fifo_name = new_buffer[i] + "_" + item;
            string result = get_info(fifo_name);
            // cout << "res" << result << " this is " << item << " from " << fifo_name << endl;
            convert_message_to_items(items, result);
            logger.info("Messages are ready for " + item);
        }
        // cout << item << endl;
        calculate_availability(items);
        
        string send_to_parent = "Item: " + item + " available num = " + to_string(num_available) + " available price = " + to_string(price_available);
        if (write(pipe_fd_child_to_parent, send_to_parent.c_str(), int(send_to_parent.size())) == -1) {
            logger.error("write unsuccessfull");
            return EXIT_FAILURE;
        } 
        else 
            logger.info("Messege succesfully sent to parent from " + item);

    } 
    else
        logger.error("Read failed");

    close(pipe_fd_parent_to_child);
    close(pipe_fd_child_to_parent);
    return 0;
}