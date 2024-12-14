#include <bits/stdc++.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <filesystem>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include "const.hpp"
#include "logger.hpp"

using namespace std;
using namespace std::filesystem;

Logger logger("main");

vector <string> tokenize(string inp_str, char delimiter)
{
    stringstream ss(inp_str); 
    string s; 
    vector <string> str;
    while (getline(ss, s, delimiter)) {    
        str.push_back(s);
    }
    return str; 
}

vector<string> read_warehouses(string directorypath) {
    vector< string> warehouses, temp;
    if (exists(directorypath) && is_directory(directorypath)) { 
        for (const auto& entry: directory_iterator(directorypath)) { 
            temp = tokenize(entry.path().string(), '/');
            int ind = temp[1].find('.');
            if (temp[1].substr(0, ind) != "Parts")
                warehouses.push_back(temp[1].substr(0, ind));
        }
    }
    else
        logger.error("Directory not found.");
    return warehouses;
}

vector<string> read_items(const string &filePath) {
    vector<string> items_in_parts_csv;
    ifstream file(filePath);
    if (file.is_open()) {
        string line;
        getline(file, line);
        stringstream ss(line);
        string item;
        while (getline(ss, item, COMMA))
            items_in_parts_csv.push_back(item);
        file.close();
    } else
        logger.error("Error: Unable to open file " + filePath);

    return items_in_parts_csv;
}

void show_vectors(const vector <string> &vectors, string val) {
    SetColor(34);
    cout << "[INFO-main]: ";
    ResetColor();
    SetColor(32);
    cout << int(vectors.size()) << " " << val << " found: " << endl << TAB << LONGER_SPACE;
    for (auto& v: vectors)
        cout << v << SPACE;
    cout << endl;
    for (int i = 0; i < 75; i++)
        cout << '-';
    cout << endl;
    ResetColor();
}

vector<string> make_csv_addresses(string path, const vector<string> &warehouses) {
    vector<string> addresses;
    for (auto warehouse: warehouses)
        addresses.push_back("./" + path + "/" + warehouse + ".csv");
    return addresses;
}

void show_addresses(vector<string> &addresses) {
    SetColor(34);
    cout << "[INFO-main]: ";
    ResetColor();
    SetColor(32);
    cout << "CSV addresses: " << endl;
    for (auto i: addresses)
        cout << TAB << LONGER_SPACE << i << endl;
    ResetColor();
}

vector<string> get_wanted_items(vector<string>& item) {
    logger.info("These are the Items.\n What do you want to show it's datails?\n");
    for (int i = 0; i < int(item.size()); i++)
        cout << i + 1 << " " << item[i] << endl;
    string input;
    vector <int> input_numbers;
    vector <string> temp;
    getline(cin, input);
    stringstream ss(input);
    int number;
    while (ss >> number)
        input_numbers.push_back(number);

    for (int i = 0; i < int(input_numbers.size()); i++)
        temp.push_back(item[input_numbers[i] - 1]);

    return temp;
}

void create_named_pipe(vector<string> &item , vector<string> &warehouses) {
    for (int i = 0; i < int(warehouses.size()); i++)
    {
        for (int j = 0; j < int(item.size()); j++)
        {
            string fifo_name = warehouses[i] + "_" + item[j];
            if (exists(fifo_name)) {
                logger.warning(fifo_name + "EXISTS\n");
                unlink(fifo_name.c_str());
            }
            if(mkfifo(fifo_name.c_str(), 0666) == -1){
                logger.error("Making named pipe failed!");
            }
            else
                logger.info("This fifo is created: " + fifo_name);
        }
    }
}

void remove_named_pipe(vector<string> &item, vector<string> &warehouses) {
    for (int i = 0; i < int(warehouses.size()); i++)
    {
        for (int j = 0; j < int(item.size()); j++)
        {
            string fifo_name = warehouses[i] + "_" + item[j];
            if (exists(fifo_name)) {
                logger.info("This " + fifo_name + " fifo is removed");
                unlink(fifo_name.c_str());
            }
        }
    }
}

vector<string> create_warehouses_messege(vector<string> old_mes, vector<string> item) {
    string add_mes = "$";
    for (int i = 0; i < int(item.size()); i++)
        add_mes += item[i] + "$";
    for (int i = 0; i < int(old_mes.size()); i++)
        old_mes[i] = old_mes[i] + add_mes;
    return old_mes;
}

vector<string> create_item_messege(vector<string> item , vector<string> warehouse) {
    vector<string> res;
    for (int i = 0; i < int(item.size()); i++)
    {
        string mes = item[i] + "$";
        for (int j = 0; j < int(warehouse.size()); j++)
        {
            mes = mes + warehouse[j] + "$";
        }
        res.push_back(mes);
    }
    return res;
}

void test(vector<string> &csv_addresses_and_items_together, vector<string> &item_messages) {
    for (auto i: csv_addresses_and_items_together) {
        cout << i << endl;
    }
    for (auto i: item_messages) {
        cout << i << endl;
    }
}

void show_buffer_of_warehouses(vector<string> &buffer_for_warehouses) {
    // SetColor(32);
    for (int i = 0; i < int(buffer_for_warehouses.size()); i++)
        logger.info(buffer_for_warehouses[i]);
    // ResetColor();
}

void show_buffer_of_items(vector<string> &buffer_for_items) {
    // SetColor(32);
    for (int i = 0; i < int(buffer_for_items.size()); i++)
        logger.info(buffer_for_items[i]);
    // ResetColor();
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <path_to_stores>" << endl;
        return 1;
    }
    string stores_path = argv[1];
    string parts_file_path = stores_path + PARTS_PATH;

    vector<string> items = read_items(parts_file_path);
    if (items.empty()) {
        logger.error("Error: No items found in " + parts_file_path + '\n');
        return 1;
    }
    show_vectors(items, "items");
    vector<string> warehouses = read_warehouses(stores_path);
    if (warehouses.empty()) {
        logger.error("Error: No warehouses found in " + stores_path + '\n');
        return 1;
    }
    show_vectors(warehouses, "warehouses");

    vector<string> csv_addresses = make_csv_addresses(stores_path, warehouses);
    show_addresses(csv_addresses);

    items = get_wanted_items(items);
    create_named_pipe(items, warehouses);

    const int NUM_WAREHOUSES = int(warehouses.size());
    vector<int[2]> parent_to_child_pipes_warehouses(NUM_WAREHOUSES);
    vector<int[2]> child_to_parent_pipes_warehouses(NUM_WAREHOUSES);

    const int NUM_ITEMS = int(items.size());
    vector<int[2]> parent_to_child_pipes_items(NUM_ITEMS);
    vector<int[2]> child_to_parents_pipes_items(NUM_ITEMS);

    for (int i = 0; i < NUM_WAREHOUSES; i++) {
        if (pipe(parent_to_child_pipes_warehouses[i]) == -1 || pipe(child_to_parent_pipes_warehouses[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }
    logger.info("Ordinary pipes are created to make connection between main and warehouses!");

    for (int i = 0; i < NUM_ITEMS; i++) {
        if (pipe(parent_to_child_pipes_items[i]) == -1 || pipe(child_to_parents_pipes_items[i]) == -1) {
            perror("pipe");
            return 1;
        }
    }
    logger.info("Ordinary pipes are created to make connection between main and items!");

    for (int i = 0; i < int(warehouses.size()); i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            logger.info("Fork for warehouse: " + warehouses[i]);
            for (int j = 0; j < NUM_WAREHOUSES; ++j) {
                if (j != i) {
                    close(parent_to_child_pipes_warehouses[j][READ]);
                    close(parent_to_child_pipes_warehouses[j][WRITE]);
                    close(child_to_parent_pipes_warehouses[j][READ]);
                    close(child_to_parent_pipes_warehouses[j][WRITE]);
                }
            }

            close(parent_to_child_pipes_warehouses[i][WRITE]);
            close(child_to_parent_pipes_warehouses[i][READ]);

            char pipeFD_parent_to_child_warehouses[10];
            char pipeFD_child_to_parent_warehouses[10];
            snprintf(pipeFD_parent_to_child_warehouses, sizeof(pipeFD_parent_to_child_warehouses), "%d", parent_to_child_pipes_warehouses[i][READ]);
            snprintf(pipeFD_child_to_parent_warehouses, sizeof(pipeFD_child_to_parent_warehouses), "%d", child_to_parent_pipes_warehouses[i][WRITE]);
            
            execl(WAREHOUSE_OBJ, WAREHOUSE_OBJ, pipeFD_parent_to_child_warehouses, pipeFD_child_to_parent_warehouses, nullptr);
            perror("execl");
            return 1;
        }
    }

    for (int i = 0; i < int(items.size()); i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return 1;
        }

        if (pid == 0) {
            logger.info("Fork for item: " + items[i]);
            for (int j = 0; j < NUM_ITEMS; ++j) {
                if (j != i) {
                    close(parent_to_child_pipes_items[j][READ]);
                    close(parent_to_child_pipes_items[j][WRITE]);
                    close(child_to_parents_pipes_items[j][READ]);
                    close(child_to_parents_pipes_items[j][WRITE]);
                }
            }

            close(parent_to_child_pipes_items[i][WRITE]);
            close(child_to_parents_pipes_items[i][READ]);

            char pipeFD_parent_to_child_items[10];
            char pipeFD_child_to_parent_items[10];
            snprintf(pipeFD_parent_to_child_items, sizeof(pipeFD_parent_to_child_items), "%d", parent_to_child_pipes_items[i][READ]);
            snprintf(pipeFD_child_to_parent_items, sizeof(pipeFD_child_to_parent_items), "%d", child_to_parents_pipes_items[i][WRITE]);
            
            execl(ITEM_OBJ, ITEM_OBJ, pipeFD_parent_to_child_items, pipeFD_child_to_parent_items, nullptr);
            perror("execl");
            return 1;
        }
    }

    for (int i = 0; i < NUM_WAREHOUSES; i++) {
        close(parent_to_child_pipes_warehouses[i][READ]);
        close(child_to_parent_pipes_warehouses[i][WRITE]);
    }

    for (int i = 0; i < NUM_ITEMS; i++) {
        close(parent_to_child_pipes_items[i][READ]);
        close(child_to_parents_pipes_items[i][WRITE]);
    }

    vector<string> csv_addresses_and_items_together = create_warehouses_messege(csv_addresses, items);
    logger.info("Messages are created for warehouses");
    vector<string> item_messages = create_item_messege(items, warehouses);
    logger.info("Messages are created for items");
    
    // test(csv_addresses_and_items_together, item_messages);

    for (int i = 0; i < NUM_WAREHOUSES; i++) {
        write(parent_to_child_pipes_warehouses[i][WRITE], csv_addresses_and_items_together[i].c_str(), int(csv_addresses_and_items_together[i].size()));
        close(parent_to_child_pipes_warehouses[i][WRITE]);
    }

    for (int i = 0; i < NUM_ITEMS; i++) {
        write(parent_to_child_pipes_items[i][WRITE], item_messages[i].c_str(), int(item_messages[i].size()));
        close(parent_to_child_pipes_items[i][WRITE]);
    }

    vector<string> buffer_for_warehouses;
    for (int i = 0; i < NUM_WAREHOUSES; i++) {
        char buffer[256];
        ssize_t bytesRead = read(child_to_parent_pipes_warehouses[i][READ], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            buffer_for_warehouses.push_back(buffer);
        } 
        else
            logger.error("Can not read");
        close(child_to_parent_pipes_warehouses[i][READ]);
    }

    vector<string> buffer_for_items;
    for (int i = 0; i < NUM_ITEMS; i++) {
        char buffer[256];
        ssize_t bytesRead = read(child_to_parents_pipes_items[i][READ], buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            buffer_for_items.push_back(buffer);
        } 
        else
            logger.error("Can not read");
        close(child_to_parents_pipes_items[i][READ]);
    }

    for (int i = 0; i < NUM_WAREHOUSES + NUM_ITEMS; i++)
        wait(nullptr);

    show_buffer_of_warehouses(buffer_for_warehouses);
    show_buffer_of_items(buffer_for_items);
    
    remove_named_pipe(items , warehouses);
    return 0;
}