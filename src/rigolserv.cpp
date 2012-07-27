
#include "freetmc_local.h"
#include "rigoltmc.h"
#include "netcomm.h"

#include <iostream>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>


using namespace std;

#define HEADER_SIZE  (12)

std::vector<DS1000E *> scopes;


void ConnectDevice(uint16_t vendID, uint16_t prodID, const std::string & sernum)
{
    
}

int main(int argc, const char * argv[])
{
    try {
        DS1000E scope(new TMC_LocalDevice(0x1AB1, 0x0588, "DS1EB134806939"));
        IDN_Response idn = scope.Identify();
        cout << "Manufacturer: " << idn.manufacturer << endl;
        cout << "Model: " << idn.model << endl;
        cout << "Serial: " << idn.serial << endl;
        cout << "Version: " << idn.version << endl;
        
        
        Host host("9393");
        cout << "Server started" << endl;
        Socket * sock = NULL;
        while(!sock) {
            usleep(100000);
            sock = host.Accept();
        }
        cout << "Client connected: " << sock->other << endl;
        
        FramedConnection conn(sock->sockfd);
        while(1)
        {
            if(conn.Poll())
            {
                std::vector<uint8_t> * msg = conn.PopMessage();
                
                // printf("received: \n");
                // for(ssize_t j = 0; j < msg->size(); ++j)
                //     printf(" %02X", (*msg)[j]);
                // printf("\n");
                
                if(msg->size() < HEADER_SIZE) {
                    cerr << "Dropped short message (msg size: " << msg->size() << ")" << endl;
                    delete msg;
                    continue;
                }
                if((*msg)[0] != 0x01) {
                    delete msg;
                    cerr << "Dropped message with incorrect command" << endl;
                    continue;
                }
                
                uint8_t cmd = (*msg)[0];
                uint8_t cmd2 = (*msg)[1];
                uint8_t seqnum = (*msg)[2];
                uint8_t seqnum2 = (*msg)[3];
                uint32_t writeSize = ntohl(*(uint32_t*)&(*msg)[4]);
                uint32_t readSize = ntohl(*(uint32_t*)&(*msg)[8]);
                
                if(seqnum != (0xFF & ~seqnum2)) {
                    delete msg;
                    cerr << "Dropped message with bad sequence number" << endl;
                    continue;
                }
                if(msg->size() != writeSize + HEADER_SIZE) {
                    delete msg;
                    cerr << "Dropped message with bad length" << endl;
                    continue;
                }
                
                if(writeSize) {
                    cout << "write: " << string(msg->begin() + HEADER_SIZE, msg->end()) << endl;
                    scope.Write(&(*msg)[HEADER_SIZE], msg->size() - HEADER_SIZE);
                }
                
                std::vector<uint8_t> resp(readSize + HEADER_SIZE);
                if(readSize)
                    readSize = scope.Read(&resp[HEADER_SIZE], readSize);
                
                resp[0] = 0x01;
                resp[1] = cmd2;
                resp[2] = seqnum;
                resp[3] = seqnum2;
                *(uint32_t*)&(resp)[4] = htonl(readSize);
                *(uint32_t*)&(resp)[8] = 0;
                
                // if(readSize > 128)
                //     printf("long response: %d B\n", (int)(readSize + HEADER_SIZE));
                // else
                // {
                //     printf("response: %d B\n", (int)(readSize + HEADER_SIZE));
                //     for(ssize_t j = 0; j < readSize + HEADER_SIZE; ++j)
                //         printf(" %02X", (*msg)[j]);
                //     printf("\n");
                // }
                
                if(readSize)
                conn.SendMessage((uint8_t*)&resp[0], readSize + HEADER_SIZE);
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


