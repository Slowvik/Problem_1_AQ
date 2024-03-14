#include "server1.h"
#include <string>

int main(int argc, char* argv[])
{
    std::string filename = argv[1];
    int port = atoi(argv[2]);

    broker1::server s(filename, port);
    
    s.start();
    //s.accept_connection();
    //s.send_data();
    s.close();

    return 0;
}