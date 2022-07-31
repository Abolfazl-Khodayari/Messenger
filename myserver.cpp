#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <threads.h>
#include <thread>
#include <unistd.h>
#include <mutex>
#include <map>

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
    Server(int _port){
        id_numbers = 0;
        max_length = 150;
        port = _port;
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
            perror("we fucked up: socket");
            exit(-1);
	    }
        if ((bind(server_socket, (struct sockaddr *)&server, sizeof(struct sockaddr_in))) == -1){
            perror("we fucked up: bind");
            exit(-1);
	    }
        if ((listen(server_socket, 5)) == -1){
            perror("we fucked up: listen");
            exit(-1);
        }
        multi_print("--Server start listening--");
    }
    void start_accepting(){
        struct sockaddr_in client;
        int client_socket;
        unsigned int len = sizeof(sockaddr_in);
        while (true){
            if ((client_socket = accept(server_socket, (struct sockaddr *)&client, &len)) == -1){
                perror("we fucked up: accept");
                exit(-1);
            }
            id_numbers++;
            User_server* user_server = new User_server(id_numbers, client_socket);
            thread* client_thread = new thread( handle_client ,this, user_server);
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
            if(find_or_creat_user(the_name, the_password, user_server)){
                send_message(user_server->client_socket, "Wrong UserName or Password " + string(the_name));
                multi_print("Wrong UserName or Password " + string(the_name));
                continue;
            }
            user_server->name = the_name;
            send_message(user_server->client_socket, "Chatroom -> Wellcome " + string(the_name));
            multi_print("Chatroom -> Wellcome " + string(the_name));
            return true;
        }   

    }
    void check_user_status(string name, bool online_status){
        if (users_list.find(name) == users_list.end()){
            throw "User not found";
        }
        if(online_status && !users_list[name]->user_server){
            throw "User is offline";
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
            if (splited_message->size() == 0 || (splited_message->size() != -1)){
                throw "The message format is incorrect";
            }
            if (splited_message->at(0) == "pv"){
                if (splited_message->size() == 0){
                    throw "The message format is incorrect";
                } 
                check_user_status(splited_message->at(1), true);
                send_pv(user->user_server, users_list[splited_message->at(1)]->user_server, splited_message->at(2)); 
            }
            else{
                throw "The command is not executable";
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
};









int main(){
    cout << "---Server-starting---\n";
    Server myserver(10001);
    myserver.start_listening();
    myserver.start_accepting();


    
    return 0;
}