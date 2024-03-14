#ifndef SERVER_H_
#define SERVER_H_

#include <winsock2.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <string>
#include <chrono>
#include <thread>

#define SERVER_NAME_BASE "server"
#define BUFFER_SIZE_DEFAULT 20

namespace server
{
    //File reading stuff:
    std::ifstream file_input;
    std::string filename;
    uint32_t broker_ID;
    uint64_t instrument_ID;

    WSADATA win_sock_data;
    int WSA_startup_return;
    int WSA_cleanup_return;

    int port_number;
    SOCKET TCP_server_socket;
    int close_socket_return;
    struct sockaddr_in TCP_server;

    SOCKET TCP_accepted_socket;

    int bind_return;
    int listen_return;

    unsigned char sender_buffer[BUFFER_SIZE_DEFAULT];//Intentionally a little larger.
    int sender_buffer_size;
    int send_return;

    bool readData()
    {
        if(!file_input.eof())
        {
            file_input>>broker_ID>>instrument_ID;
            return true;
        }
        return false;
    }

    void pushInt4ToByteStream()//converts and places it in sender buffer
    {
        sender_buffer[0] = (broker_ID >> 24) & 0xFF;
        sender_buffer[1] = (broker_ID >> 16) & 0xFF;
        sender_buffer[2] = (broker_ID >> 8) & 0xFF;
        sender_buffer[3] = broker_ID & 0xFF;
    }

    void appendInt8ToByteStream()//converts and places it in sender buffer
    {
        sender_buffer[4] = (instrument_ID >> 56) & 0xFF;
        sender_buffer[5] = (instrument_ID >> 48) & 0xFF;
        sender_buffer[6] = (instrument_ID >> 40) & 0xFF;
        sender_buffer[7] = (instrument_ID >> 32) & 0xFF;
        sender_buffer[8] = (instrument_ID >> 24) & 0xFF;
        sender_buffer[9] = (instrument_ID >> 16) & 0xFF;
        sender_buffer[10] = (instrument_ID >> 8) & 0xFF;
        sender_buffer[11] = instrument_ID & 0xFF;
    }

    void init(std::string file_name, int port_num)
    {
        sender_buffer_size = BUFFER_SIZE_DEFAULT;
        port_number = port_num;
        WSA_startup_return = WSAStartup(MAKEWORD(2,2), &win_sock_data); //WSA Version 2.2
        if(WSA_startup_return!=0)
        {
            std::cout<<"WSAStartup failed"<<std::endl;
            return;
        }
        std::cout<<"WSAStartup successful"<<std::endl;

        file_input.open(file_name.c_str(),std::ifstream::in);
        sender_buffer_size = 20;
    }

    void start()
    {
        TCP_server.sin_family = AF_INET;
        TCP_server.sin_addr.s_addr = inet_addr("127.0.0.1");
        TCP_server.sin_port = htons(port_number);

        TCP_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if(TCP_server_socket == INVALID_SOCKET)
        {
            std::cout<<"Server socket creation failed"<<std::endl;
            return;
        }
        std::cout<<"Server socket creation successful"<<std::endl;

        bind_return = bind (TCP_server_socket, (SOCKADDR*)&TCP_server, sizeof(TCP_server));
        if(bind_return == SOCKET_ERROR)
        {
            std::cout<<"Bind failed"<<std::endl;
            return;
        }
        std::cout<<"Binding successful"<<std::endl;

        listen_return = listen(TCP_server_socket, 2);
        if(listen_return==SOCKET_ERROR)
        {
            std::cout<<"Listen failed"<<std::endl;
            return;
        }
        std::cout<<"Listen Successful"<<std::endl;               
    }


    bool acceptConnection()
    {
        std::cout<<"Waiting for a connection request..."<<std::endl;

        struct sockaddr_in TCP_client;
        int TCP_client_size = sizeof(TCP_client);
        SOCKET new_socket = accept(TCP_server_socket, (SOCKADDR*)&TCP_client, &TCP_client_size);

        TCP_accepted_socket = new_socket;
        if(TCP_accepted_socket == INVALID_SOCKET)
        {
            std::cout<<"Accept error"<<std::endl;
            return false;
        }
        std::cout<<"Accept successful"<<std::endl;
        return true;
    }

    void sendData()
    {
        while(readData())
        {
            pushInt4ToByteStream();
            appendInt8ToByteStream();
           
            send_return = send(TCP_accepted_socket, (char*)sender_buffer, BUFFER_SIZE_DEFAULT, 0);
            if(send_return == SOCKET_ERROR)
            {
                std::cout<<"Send failed"<<std::endl;
                return;
            }
            std::cout<<"sending data successful"<<std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));//To simulate real-world servers
        }
    }

    void close()
    {
        system("PAUSE");//Abruptly closing the socket might lead to loss of data
        close_socket_return = closesocket(TCP_server_socket);
        if(close_socket_return == SOCKET_ERROR)
        {
            std::cout<<"Socket error in close"<<std::endl;
            return;
        }
        std::cout<<"Closed successfully"<<std::endl;

        WSA_cleanup_return = WSACleanup();
        if(WSA_cleanup_return == SOCKET_ERROR)
        {
            std::cout<<"WSACleaup error"<<std::endl;
            return;
        }
        std::cout<<"WSACleaup successful"<<std::endl;
    }
}

#endif