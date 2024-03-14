/*
Server side notes:
    > Use proper methods to convert data to network byte stream.

Key considerations:
    > How to format and send the data correctly.

In case someone is looking for modern C++20 solution (with <bit> in standard library).

Here are two nice and templated functions to convert fundamental types (like int, short, etc.) to bytes with conversion to necessary endianness:

template<typename T, std::endian to_endianness = std::endian::big>
std::vector<uint8_t> toBytes(T value)
{
    std::vector<uint8_t> buffer(sizeof(T));

    std::copy_n(reinterpret_cast<uint8_t*>(&value), sizeof(T), buffer.begin());

    if constexpr (std::endian::native != to_endianness)
        std::reverse(buffer.begin(), buffer.end());

    return buffer;
}
...and to convert from bytes you can use:

template<typename T, std::endian from_endianness = std::endian::big>
T fromBytes(std::vector<uint8_t> bytes)
{
    if constexpr (std::endian::native != from_endianness)
        std::reverse(bytes.begin(), bytes.end());

    T* buffer = reinterpret_cast<T*>(bytes.data());

    return *buffer;
}
To keep code in this answer simple I have intentionally removed checks for mixed endianness and std::is_fundamental<T>, you can see code with these checks on my GitHub gist (Unlicense license) if you need to.
*/
#ifndef SERVER1_H_
#define SERVER1_H_

#include <winsock2.h>
#include <iostream>
#include <cstring>
#include <assert.h>
#include <fstream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

namespace broker1
{
    class server
    {
        private:

            //File reading stuff:
            std::ifstream file_input;
            std::string filename;
            uint32_t broker_ID;
            uint64_t instrument_ID;

            WSADATA win_sock_data;
            int WSA_startup_return;
            int WSA_cleanup_return;

            SOCKET TCP_server_socket;
            int close_socket_return;
            struct sockaddr_in TCP_server;

            SOCKET TCP_accepted_socket;

            int bind_return;
            int listen_return;

            unsigned char sender_buffer[20];
            int sender_buffer_size;
            int send_return;



            bool read_data()
            {
                if(!file_input.eof())
                {
                    file_input>>broker_ID>>instrument_ID;
                    return true;
                }
                return false;
            }

            bool push_int4_to_byte_stream()//converts and places it in sender buffer
            {
                sender_buffer[0] = (broker_ID >> 24) & 0xFF;
                sender_buffer[1] = (broker_ID >> 16) & 0xFF;
                sender_buffer[2] = (broker_ID >> 8) & 0xFF;
                sender_buffer[3] = broker_ID & 0xFF;
                return true;
            }

            bool append_int8_to_byte_stream()//converts and places it in sender buffer
            {
                sender_buffer[4] = (instrument_ID >> 56) & 0xFF;
                sender_buffer[5] = (instrument_ID >> 48) & 0xFF;
                sender_buffer[6] = (instrument_ID >> 40) & 0xFF;
                sender_buffer[7] = (instrument_ID >> 32) & 0xFF;
                sender_buffer[8] = (instrument_ID >> 24) & 0xFF;
                sender_buffer[9] = (instrument_ID >> 16) & 0xFF;
                sender_buffer[10] = (instrument_ID >> 8) & 0xFF;
                sender_buffer[11] = instrument_ID & 0xFF;
                return true;
            }

        public:

            explicit server(std::string f_name)
            {
                WSA_startup_return = WSAStartup(MAKEWORD(2,2), &win_sock_data); //WSA Version 2.2
                if(WSA_startup_return!=0)
                {
                    std::cout<<"WSAStartup failed"<<std::endl;
                    return;
                }
                std::cout<<"WSAStartup successful"<<std::endl;

                filename = f_name;
                file_input.open(filename.c_str(),std::ifstream::in);
                sender_buffer_size = 20;
            }

            void start()
            {
                TCP_server.sin_family = AF_INET;
                TCP_server.sin_addr.s_addr = inet_addr("127.0.0.1");
                TCP_server.sin_port = htons(8000);

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

                if(accept_connection())
                {
                    send_data();
                }                
            }


            bool accept_connection()
            {
                //assert(listen_return!=SOCKET_ERROR);
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

            void send_data()
            {
                while(read_data())
                {
                    /*
                    CONVERTING DATA TO TCP BYTE STREAM GOES HERE
                    */
                    if(push_int4_to_byte_stream() && append_int8_to_byte_stream())
                    {
                        std::cout<<"Unconverted ints: "<<broker_ID<<":"<<instrument_ID<<std::endl;
                        uint32_t reconverted_broker_ID = int(
                                    (unsigned char)(sender_buffer[0]) << 24 |
                                    (unsigned char)(sender_buffer[1]) << 16 |
                                    (unsigned char)(sender_buffer[2]) << 8 |
                                    (unsigned char)(sender_buffer[3]));

                        uint64_t reconverted_instrument_ID = int(
                                    (uint64_t)(sender_buffer[4]) << 56 |
                                    (uint64_t)(sender_buffer[5]) << 48 |
                                    (uint64_t)(sender_buffer[6]) << 40 |
                                    (uint64_t)(sender_buffer[7]) << 32 |
                                    (uint64_t)(sender_buffer[8]) << 24 |
                                    (uint64_t)(sender_buffer[9]) << 16 |
                                    (uint64_t)(sender_buffer[10]) << 8 |
                                    (uint64_t)(sender_buffer[11]));

                        std::cout<<"Reconverted: "<<reconverted_broker_ID<<":"<<reconverted_instrument_ID<<std::endl;

                        send_return = send(TCP_accepted_socket, (char*)sender_buffer, sender_buffer_size, 0);
                        if(send_return == SOCKET_ERROR)
                        {
                            std::cout<<"Send failed"<<std::endl;
                            return;
                        }
                        std::cout<<"sending data successful"<<std::endl;
                    }
                }
            }

            void close()
            {
                system("PAUSE");
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
                //system("PAUSE");
            }
    };
}

#endif