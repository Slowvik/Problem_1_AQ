#ifndef CLIENT_H_
#define CLIENT_H_

/*

> Receiver side notes:
    > Critical sections are:
        > Adding integers to the sorted list
    > Connecting to servers and reading can be async
    > servers contain sorted list.
    > integer 0 denotes end of stream. wait for all ends of streams and finally print the list from the main thread.

> Key considerations: 
    > Reading the exact 4 byte ID and 8 byte integer from the receive buffer
    > making sure no ints get dropped
    > minimising starvation
    > reading the final end of stream signal correctly 
*/

#define RECV_BUFFER_SIZE_DEFAULT 512
#define PACKET_SIZE_DEFAULT 20
#define NUMBER_OF_BROKERS_DEFAULT 100
#define TOTAL_INSTRUMENTS_DEFAULT 10000
#define INSTRUMENTS_PER_BROKER 1000

#include <winsock2.h>
#include <iostream>
#include <thread>
#include <semaphore>

namespace client
{
    
    int num_brokers;
    WSADATA client_data;

    int port_number_vec[NUMBER_OF_BROKERS_DEFAULT];
    SOCKET client_socket_vec[NUMBER_OF_BROKERS_DEFAULT];
    struct sockaddr_in server_details_vec[NUMBER_OF_BROKERS_DEFAULT];
    int connection_status_vec[NUMBER_OF_BROKERS_DEFAULT];
    int received_buffer_size_vec[NUMBER_OF_BROKERS_DEFAULT];

    int list_of_all_instruments[TOTAL_INSTRUMENTS_DEFAULT];
    int actual_number_of_instruments;
    std::binary_semaphore list_update_sema(1);

    void init(int ports[NUMBER_OF_BROKERS_DEFAULT], int n)
    {
        std::cout<<"Initialising the client"<<std::endl;

        actual_number_of_instruments = 0;
        
        int WSA_return = WSAStartup(MAKEWORD(2,2), &client_data);
        if(WSA_return!=0)
        {
            std::cout<<"WSAStartup failed"<<std::endl;
            return;
        }
        std::cout<<"WSAStartup successful"<<std::endl;

        num_brokers = n;
        for(int i = 0; i<n; i++) //Set up n client sockets and connect to all of them. This part could be divided into two parts: setting up the sockets, and connecting to them, with individual connections made asynchronous. 
        {
            client_socket_vec[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            port_number_vec[i] = ports[i];

            server_details_vec[i].sin_family = AF_INET;
            server_details_vec[i].sin_addr.s_addr = inet_addr("127.0.0.1");
            server_details_vec[i].sin_port = htons(port_number_vec[i]);

            connection_status_vec[i] = connect(client_socket_vec[i], (SOCKADDR*)&server_details_vec[i], sizeof(server_details_vec[i]));

            if(connection_status_vec[i] == SOCKET_ERROR)
            {
                std::cout<<"Error connecting to server with port "<<port_number_vec[i]<<std::endl;
                return;
            }
            std::cout<<"Successfully connected to server with port "<<port_number_vec[i]<<std::endl;

            received_buffer_size_vec[i] = 0;
        }
    }

    uint32_t byteToUint32_t(unsigned char* buffer, int start_pos)
    {
        uint32_t reconverted_broker_ID = int(
                            (uint32_t)(buffer[start_pos]) << 24 |
                            (uint32_t)(buffer[start_pos+1]) << 16 |
                            (uint32_t)(buffer[start_pos+2]) << 8 |
                            (uint32_t)(buffer[start_pos+3]));

        return reconverted_broker_ID;
    }

    uint64_t byteToUint64_t(unsigned char* buffer, int start_pos)
    {
        uint64_t reconverted_instrument_ID = int(
                            (uint64_t)(buffer[start_pos+4]) << 56 |
                            (uint64_t)(buffer[start_pos+5]) << 48 |
                            (uint64_t)(buffer[start_pos+6]) << 40 |
                            (uint64_t)(buffer[start_pos+7]) << 32 |
                            (uint64_t)(buffer[start_pos+8]) << 24 |
                            (uint64_t)(buffer[start_pos+9]) << 16 |
                            (uint64_t)(buffer[start_pos+10]) << 8 |
                            (uint64_t)(buffer[start_pos+11]));
        
        return reconverted_instrument_ID;
    }

    void printListOfInstruments()
    {
        if(actual_number_of_instruments!=0)
        {
            std::cout<<"List of all instruments in sorted order: "<<std::endl;
            for(int i = 0; i<actual_number_of_instruments; i++)
            {
                std::cout<<list_of_all_instruments[i]<<std::endl;
            }
            std::cout<<std::endl;
        }
        else
        {
            std::cout<<"No instruments in list..."<<std::endl;
        }            
    } 

    void listPush(int new_item)
    {
        if(actual_number_of_instruments == 0)
        {
            list_of_all_instruments[0] = new_item;
            actual_number_of_instruments++;
            return;
        }

        int insertion_pos = actual_number_of_instruments - 1;

        while(list_of_all_instruments[insertion_pos]>new_item)
        {
            //Copy:
            list_of_all_instruments[insertion_pos+1] = list_of_all_instruments[insertion_pos];
            insertion_pos--;
        } 

        if(list_of_all_instruments[insertion_pos] == new_item)
        {
            insertion_pos++;
            while(insertion_pos<actual_number_of_instruments)
            {
                //Copy back:
                list_of_all_instruments[insertion_pos] = list_of_all_instruments[insertion_pos+1];
                insertion_pos++;
            }
            return;
        } 

        insertion_pos++;
        list_of_all_instruments[insertion_pos] = new_item;       
        
        actual_number_of_instruments++;
    }

    void threadRunner(int thread_ID)
    {
        bool end_of_stream = false;
        bool queue_empty = false;

        unsigned char receiver_buffer_temp[RECV_BUFFER_SIZE_DEFAULT];  
        int queue_of_instruments[INSTRUMENTS_PER_BROKER];
        int number_of_instruments_in_queue = 0;
        int back = 0;  
        int front = 0;    

        while(1)
        {
            received_buffer_size_vec[thread_ID] = recv(client_socket_vec[thread_ID], (char*)receiver_buffer_temp, RECV_BUFFER_SIZE_DEFAULT, 0);

            if(received_buffer_size_vec[thread_ID]==SOCKET_ERROR)
            {
                std::cout<<"Socket error while receiving from "<<port_number_vec[thread_ID]<<std::endl;
                break;
            }

            int start = 0; 

            while(received_buffer_size_vec[thread_ID]>=PACKET_SIZE_DEFAULT)
            {
                int broker_ID = byteToUint32_t(receiver_buffer_temp, start);
                int instrument_ID = byteToUint64_t(receiver_buffer_temp, start);
                
                if(instrument_ID == 0)
                {
                    end_of_stream = true;
                    received_buffer_size_vec[thread_ID] = -1;
                    break;
                }

                queue_of_instruments[back++] = instrument_ID;
                number_of_instruments_in_queue++;
                received_buffer_size_vec[thread_ID]-=PACKET_SIZE_DEFAULT;
                start += PACKET_SIZE_DEFAULT;
            }

            if(number_of_instruments_in_queue != 0)
            {
                if(list_update_sema.try_acquire())
                {
                    listPush(queue_of_instruments[front++]);
                    number_of_instruments_in_queue--;
                    list_update_sema.release();                       
                }
            }
            

            if(end_of_stream && number_of_instruments_in_queue == 0)
            {
                break;
            }
        }
    }

    void close()
    {
        for(auto i = 0; i< num_brokers; i++)
        {
            int close_return = closesocket(client_socket_vec[i]);
            if(close_return == SOCKET_ERROR)
            {
                std::cout<<"Error closing socket with port number "<<port_number_vec[i]<<std::endl;
                break;
            }
            std::cout<<"closed socket with port number "<<port_number_vec[i]<<std::endl;
        }

        int WSA_cleanup_return = WSACleanup();
        if(WSA_cleanup_return == SOCKET_ERROR)
        {
            std::cout<<"WSACleaup error"<<std::endl;
            return;
        }
        std::cout<<"WSACleaup successful"<<std::endl;
    }        

}

#endif