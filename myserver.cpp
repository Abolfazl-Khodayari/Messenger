#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <threads.h>
#include <thread>
#include <unistd.h>
#include <mutex>
#include <map>
#include <fstream>
#include <signal.h>

using namespace std;

vector<string>* split_message(string the_message, string spliter, int start_position){
    vector<string>* message = new vector<string>;
    int end_position;
    while((end_position = the_message.find(spliter, start_position)) != string::npos){ // idn string::npos
        message->push_back(the_message.substr(start_position, end_position-start_position));
        start_position = end_position + spliter.length();
    }
    message->push_back(the_message.substr(start_position));
    return message;
}

class User_server{
public:
    int id;
    int client_socket;
    string name;
    string username;
    thread* client_thread;
    User_server(int _id, int _client_socket):
    id(_id), client_socket(_client_socket), name("Anonymous")
    {}
    ~User_server(){
        if (client_thread){
            if (client_thread->joinable()){
                client_thread->detach();
                delete client_thread;
            }
            client_thread = 0;
        }
        if (client_socket){
            close(client_socket);
        }
    }
};
class User{
public:
    string name;
    string password;
    User_server* user_server;
    User(string the_name, string the_password, User_server* the_user_server=0){
        name = the_name;
        password = the_password;
        user_server = the_user_server;
    }   
};


class Users_list_file{
public:
    fstream file;
    string file_address;
    string spliter;
    mutex write_mtx;
    bool first = true;
    Users_list_file(string address, string split){
        spliter = split;
        file_address = address;
    }
    void add_user(User* user){
        lock_guard<mutex> guard(write_mtx);
        if(!file){
            cout << "file not opened\n";
        }
        if(first){
            file.open(file_address, ios::out);
            file.close();
            first = false;
        }
        file.open(file_address, ios::app);
        if(!file){
            cout << "file not opened\n";
        }
        string newline = user->name + spliter + user->password + "\n";
        file << newline;
        file.close();
    }
    vector<User*>* get_users(){
        vector<User*>* users_list = new vector<User*>;
        lock_guard<mutex> guard(write_mtx);
        file.open(file_address, ios::in);
        string line;
        vector<string>* messages;
        while(!file.eof()){
            getline(file, line);
            if(line.length() <= 0){
                continue;
            }
            messages = split_message(line, spliter, 0);
            users_list->push_back(new User(messages->at(0), messages->at(1)));
        }
        file.close();
        return users_list;
    }

};

class Groups_list_file{
public:
    fstream file;
    string file_address;
    string spliter;
    mutex write_mtx;
    Groups_list_file(string address, string split){
        spliter = split;
        file_address = address;
    }

};

class Pv_messages_file{
public:
    fstream file;
    string file_address;
    string spliter;
    mutex write_mtx;
    Pv_messages_file(string address, string split){
        spliter = split;
        file_address = address;
    }

};

class Group_messages_file{
public:
    fstream file;
    string file_address;
    string spliter;
    mutex write_mtx;
    Group_messages_file(string address, string split){
        spliter = split;
        file_address = address;
    }

};

class Buffer_messages_file{
public:
    fstream file;
    string file_address;
    string spliter;
    mutex write_mtx;
    Buffer_messages_file(string address, string split){
        spliter = split;
        file_address = address;
    }

};

class Block_list_file{
public:
    fstream file;
    string file_address;
    string spliter;
    mutex write_mtx;
    Block_list_file(string address, string split){
        spliter = split;
        file_address = address;
    }

};

class Server{
public:
    int id_numbers;
    int max_length;
    int server_socket;
    int port;
    map<string, User*> users_list;
    map<int, User_server*> clients_list;
    mutex server_mutex;
    mutex client_mutex;
    mutex print_mutex;
    Users_list_file* users_list_file;
    Groups_list_file* groups_list_file;
    Pv_messages_file* pv_messages_file;
    Group_messages_file* groups_messages_file;
    Buffer_messages_file* buffer_messages_file;
    Block_list_file* block_list_file;

    Server(int _port){
        id_numbers = 0;
        max_length = 150;
        port = _port;
        users_list_file = new Users_list_file("database/users_list_file.txt", "#!#");
        // groups_list_file = new Groups_list_file;
        // pv_messages_file = new Pv_messages_file;
        // groups_messages_file = new Groups_messages_file;
        // buffer_messages_file = new buffer_messages_file;
        
    }   
    //void start_listening();
    //void start_accepting();
    //void multi_print(string);
    // void handle_client(Server*, User_server*);
    //bool login_client(User_server*);
    //bool find_or_creat_user(User_server*, string, string);
    //void send_message(int, string);
    //void process_message(User*, string);
    //void check_user_status(string, bool);
    //void send_pv(User_server*, User_server*, string);
    //void add_client(User_server*);
    //void terminate_connection(int)

    void start_listening(){
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        server.sin_addr.s_addr = INADDR_ANY;
        if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
            perror("Server | we fucked up: socket");
            exit(-1);
	    }
        if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1){
            perror("Server | we fucked up: bind");
            exit(-1);
	    }
        if ((listen(server_socket, 5)) == -1){
            perror("Server | we fucked up: listen");
            exit(-1);
        }
        multi_print("--Server | Server start listening--");
    }
    void start_accepting(){
        struct sockaddr_in client;
        int client_socket;
        unsigned int len = sizeof(sockaddr_in);
        while (true){
            if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1){
                perror("Server | we fucked up: accept");
                exit(-1);
            }
            id_numbers++;
            User_server* user_server = new User_server(id_numbers, client_socket);
            thread* client_thread = new thread(handle_client ,this, user_server);
            user_server->client_thread = client_thread;
            add_client(user_server);
        }
    }
    void multi_print(string str){
        lock_guard<mutex> guard(print_mutex);
        cout<<str<<"\n";
    }
    bool find_or_creat_user(string name, string password, User_server* user_server){
        if(users_list.find(name) != users_list.end()){
            if(users_list[name]->password == password){
                users_list[name]->user_server == user_server;
            }
            else{
                return false;
            }
        }
        else{
            users_list[name] = new User(name, password, user_server);
            users_list_file->add_user(users_list[name]);
        }
        return true;
    }
    bool login_client(User_server* user_server){
        while (1){
            char the_name[max_length];
            //char new_username[max_length];
            char the_password[max_length];
            int byte_received = recv(user_server->client_socket, the_name, sizeof(the_name), 0);
            if(byte_received <= 0){
                return false;
            }
            byte_received = recv(user_server->client_socket, the_password, sizeof(the_password), 0);
            if(byte_received <= 0){
                return false;
            }
            if(!find_or_creat_user(the_name, the_password, user_server)){
                send_message(user_server->client_socket, "Wrong UserName or Password " + string(the_name));
                multi_print("Server | Wrong UserName or Password " + string(the_name));
                continue;
            }
            user_server->name = the_name;
            send_message(user_server->client_socket, "Server | Wellcome " + string(the_name));
            multi_print("Server | Wellcome " + string(the_name));
            return true;
        }   

    }
    void check_user_status(string name, bool online_status){
        if (users_list.find(name) == users_list.end()){
            throw "Server | User not found";
        }
        if(online_status && !users_list[name]->user_server){
            throw "Server | User is offline";
        }
    }
    void send_pv(User_server* sender, User_server* receiver, string message){
        send_message(receiver->client_socket, "PV | " + sender->name + " : " + message);
        send_message(sender->client_socket, "Server | From you to " + receiver->name + " : " + message);
    }
    void process_message(User* user, string message){
        vector<string>* splited_message = 0;
        try{
            splited_message = split_message(message, " #", 1);
            check_message_size(splited_message);
            if (splited_message->at(0) == "pv"){
                check_message_size(splited_message, 3);
                check_user_status(splited_message->at(1), true);
                send_pv(user->user_server, users_list[splited_message->at(1)]->user_server, splited_message->at(2)); 
            }
            else{
                throw "the command is not executable";
            }
        }
        catch(const char* error_message){
            send_message(user->user_server->client_socket, "Server | we fucked up: " + string(error_message));
        }
        if (splited_message){
            splited_message->clear();
            delete splited_message;
        }
    }
    void terminate_connection(int id){
        lock_guard<mutex> guard(client_mutex);
        if (clients_list[id]){
            delete clients_list[id];
        }
    }
    static void handle_client(Server* the_server, User_server* the_user_server){
        if(the_server->login_client(the_user_server)){
            char message1[the_server->max_length];
            int bytes_received;
            while (1){
                bytes_received = recv(the_user_server->client_socket, message1, sizeof(message1), 0);
                if (bytes_received <= 0){
                    break;
                }
                the_server->multi_print(the_user_server->name + " : " + message1);
                the_server->process_message(the_server->users_list[the_user_server->name], message1);
            }
            the_server->users_list[the_user_server->name]->user_server = 0;
        }
        the_server->terminate_connection(the_user_server->id);
    }
    void send_message(int client_socket, string message){
        send(client_socket, &message[0], max_length, 0);
    }
    void add_client(User_server* user_server){
        lock_guard<mutex> guard(client_mutex);
	    clients_list[user_server->id] = user_server;
    }
    void check_message_size(vector<string>* message_splitted, int i = -1){
        if (message_splitted->size() == 0 || (i != -1 && message_splitted->size() != i)){
            throw "The message format is incorrect";
        }
    }
    void delete_users(){
        for (auto & u : clients_list){
            delete u.second;
        }
        clients_list.clear();
        for (auto & u : users_list){
            delete u.second;
        }
        users_list.clear();
    }
    ~Server(){
        delete users_list_file;
        delete_users();
        close(server_socket);
    }
};






Server* myserver = 0;
void exit_app(int sig_num){
    if (myserver)
        delete myserver;
    cout<<"\n---Server|bye---"<<endl;
    exit(sig_num);
}


int main(){
    cout << "---Server-starting---\n";
    myserver = new Server(10002);
    signal(SIGINT, exit_app);
    myserver->start_listening();
    myserver->start_accepting();
    exit_app(0);
    return 0;
}

// g++ 