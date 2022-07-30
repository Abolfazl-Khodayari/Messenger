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
    mutex server_mutex;
    mutex client_mutex;
    mutex print_mutex;
    Server(int theport){
        id_numbers = 0;
        max_length = 150;
        port = theport;
    }   
    void start_listening();
    void start_accepting();
    void multi_print(string);
    void handle_client(Server*, User_server*);
    bool login_client(User_server*);
    bool find_or_creat_user(User_server*, string, string);
    void send_message(int, string);

    void start_listening(){
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(9911);
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
            thread* client_thread = new thread(handle_client, this, user_server);
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
            send_message(user_server->client_socket, "Wellcome " + string(the_name));
            multi_print("Wellcome " + string(the_name));
            return true;
        }   

    }
    void handle_client(Server* the_server, User_server* the_user_server){
        if(login_client(the_user_server)){
            char message1[the_server->max_length];
            int bytes_received;
            while (1){
                bytes_received = recv(the_user_server->client_socket, message1, sizeof(message1), 0);
                if (bytes_received <= 0){
                    break;
                }
                the_server->multi_print(the_user_server->name + " : " + message1);
                
            }
        }
    }
    void send_message(int client_socket, string message){
        send(client_socket, &message[0], max_length, 0);
    }
    
};









int main(){
    cout << "---starting---\n";
    Server myserver(10001);
    myserver.start_listening();


    
    return 0;
}