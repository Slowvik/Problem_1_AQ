#include "client.h"
#include <iostream>
#include <thread>

int main(int argc, char* argv[])
{
    int brokers_num = atoi(argv[1]);
    int ports_num[100];

    for(int i = 0; i<brokers_num; i++)
    {
        ports_num[i] = atoi(argv[i+2]);
    }    

    client::init(ports_num, brokers_num);

    std::thread thread_array[100];
    for(int i = 0; i<brokers_num; i++)
    {
        std::thread th(client::threadRunner, i);
        thread_array[i] = std::move(th);
    }
    
    for(int i = 0; i<brokers_num; i++)
    {
        thread_array[i].join();
    }

    client::printListOfInstruments();

    system("PAUSE");
    client::close();

    return 0;
}