#include "server2.h"
#include <string>

int main(int argc, char* argv[])
{
    std::string filename = argv[1];
    int port_num = atoi(argv[2]);

    broker2::server s(filename, port_num);
    
    s.start();
    //s.accept_connection();
    //s.send_data();
    s.close();

    return 0;
}