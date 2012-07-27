// g++ netcomm_servtest.cpp -o netcomm_servtest


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
        Host host("9393");
        cout << "Server started" << endl;
        
        Socket * sock = NULL;
        while(!sock) {
            usleep(100000);
            sock = host.Accept();
        }
        cout << "Client connected: " << sock->other << endl;
        
        FramedConnection conn(sock->sockfd);
        while(1) {
            if(conn.Poll())
            {
                std::vector<uint8_t> * msg = conn.PopMessage();
                cout << "received: " << string(msg->begin(), msg->end()) << endl;
                delete msg;
            }
        }
    }
    catch(exception & err)
    {
        cout << "Caught exception: " << err.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}


