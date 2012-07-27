// g++ netcomm_clienttest.cpp -o netcomm_clienttest


#include <iostream>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "netcomm.h"


using namespace std;

int main(int argc, const char * argv[])
{
    try {
        Client client;
        cout << "Attempting to connect" << endl;
        
        Socket * sock = NULL;
        while(!sock) {
            usleep(10000);
            // conn = client.Connect("127.0.0.1", "9393");
            sock = client.Connect("localhost", "9393");
        }
        cout << "Connected to server: " << sock->other << endl;
        FramedConnection conn(sock->sockfd);
        string msg;
        msg = "Hello World!\n";
        conn.SendMessage((uint8_t*)&msg[0], msg.size());
        msg = "Testing...\n";
        conn.SendMessage((uint8_t*)&msg[0], msg.size());
    }
    catch(exception & err)
    {
        cout << "Caught exception: " << err.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}


