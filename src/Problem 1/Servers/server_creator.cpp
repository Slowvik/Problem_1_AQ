#include "server.h"
#include <string>

int main(int argc, char* argv[])
{
    std::string filename = argv[1];
    int port_ID = atoi(argv[2]);

    server::init(filename, port_ID);
    
    server::start();
    bool status = server::acceptConnection();

    server::sendData();
    server::close();

    return 0;
}