#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <threads.h>
#include <thread>
#include <unistd.h>
#include <mutex>
#include <map>
#include <signal.h>

using namespace std;

class Client{
public:
    thread* send_thread;
    thread* receive_thread;
    int max_length;
    bool logged_in;
    bool exited;
    char* name;
    char* password;
    char* username;
    int port;
    mutex print_mutex;
    int client_socket;

    //client();
    //void start_connection();
    //void login();
    //void multi_print(string, bool);
    //void start_chatting();
    //static void send_handler (Client*);
    //static void receive_handler (Client*);
    //void terminate_connection();

    Client(int _port){
        max_length = 150;
        logged_in = false;
        exited = false;
        name = new char[max_length];
        password = new char[max_length];
        port = _port;
    }
    void start_connection(){
        if((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
            perror("client | we fucked up: client socket; ");
            exit(1);
        }
        struct sockaddr_in client;
        client.sin_family=AF_INET;
	    client.sin_port=htons(port);
	    client.sin_addr.s_addr=INADDR_ANY;
        if((connect(client_socket, (struct sockaddr *)&client, sizeof(struct sockaddr_in))) == -1){ // idk
            perror("client | we fucked up: client connect; ");
            exit(1);
        }
    }
    void login(){
        while(1){
            cout << "client | Welcome to SINA chatroom" << endl;
            cout << "client | Enter your name:\n";
            cin.getline(name, max_length);
            cout << "Enter your password:\n";
            cin.getline(password, max_length);
            send(client_socket, name, sizeof(name), 0);
            send(client_socket, password, sizeof(password), 0);
            multi_print("client | Fill captcha :)" , false);
            char server_response[max_length];
            int bytes_received = recv(client_socket, server_response, sizeof(server_response), 0);
            if (bytes_received <= 0){
                continue;
            }
            multi_print(server_response, false);
            if (string(server_response) == "Server |  Wellcome " + string(name)){
                break;
            }
        }
    }
    void multi_print(string message, bool you = true){ //idk
        lock_guard<mutex> guard(print_mutex);
        if (message.length()){
            cout<<"\33[2K \r"<<message<<endl;
        }
        if (you){
            cout<<"\33[2K \r"<<"Type a message: ";
        }
    }
    static void send_handler (Client* myclient){
        while(1){
            myclient->multi_print("");
            char string1[myclient->max_length];
            cin.getline(string1, myclient->max_length);
            if(string(string1) == "#exit"){
                myclient->exited = true;
                myclient->receive_thread->detach();
                close(myclient->client_socket);
                return;
            }
            send(myclient->client_socket, string1, sizeof(string1), 0);
        }
    }
    static void receive_handler (Client* myclient){
        while(!myclient->exited){
            char message[myclient->max_length];
            int bytes_received = recv(myclient->client_socket, message, sizeof(message), 0);
            if (bytes_received <= 0){
                continue;
            }
            myclient->multi_print(message);
            fflush(stdout);
        }   
    }
    void start_chatting(){
        send_thread = new thread(send_handler, this);
        receive_thread = new thread(receive_handler, this);
        if(send_thread->joinable()){
            send_thread->join();
        }
        if (receive_thread->joinable()){
            receive_thread->join();
        }
    }
    void terminate_connection(){
        if (send_thread){
            if (send_thread->joinable()){
                send_thread->detach();
                delete send_thread;
            }
            send_thread = 0;
        }
        if (receive_thread){
            if (receive_thread->joinable()){
                receive_thread->detach();
                delete receive_thread;
            }
            receive_thread = 0;
        }
        close(client_socket);
        multi_print("--socket turned off--", false);
    }
    ~Client(){
        terminate_connection();
        delete [] password;
        delete [] name;
    }
};

Client* myclient = 0;
void exit_app(int sig_num){
    if (myclient)
        delete myclient;
    cout<<"\n---bye---"<<endl;
    exit(sig_num);
}


int main(){
    cout << "---Server-starting---\n";
    myclient = new Client(10002);
    signal(SIGINT, exit_app); // ink
    myclient->start_connection();
    myclient->login();
    myclient->start_chatting();
    return 0;
}