#include "server.h"
#include <string>

int main(int argc, char* argv[])
{
    int server_ID = atoi(argv[1]);
    int port_ID = atoi(argv[2]);

    server::init(server_ID, port_ID);
    
    server::start();
    bool status = server::acceptConnection();

    server::sendData();
    server::close();

    return 0;
}