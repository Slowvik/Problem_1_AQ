#include <iostream>
#include <fstream>
#include <string>

#define MAX_INSTRUMENTS 100
#define INSTRUMENT_ID_BASE 100
#define BROKER_ID_BASE 10

int main(int argc, char* argv[])
{
    int server_number = atoi(argv[1]);
    int broker_ID = server_number * BROKER_ID_BASE;
    int instrument_ID = INSTRUMENT_ID_BASE + server_number;

    std::string filename = "server"+std::to_string(server_number)+".txt";

    std::ofstream outfile(filename.c_str(), std::ofstream::out);

    for(int i=1; i<=MAX_INSTRUMENTS; i++)
    {
        outfile<<broker_ID<<" "<<instrument_ID*i<<"\n";
    }

    outfile<<broker_ID<<" "<<0;

    outfile.close();
    return 0;
}