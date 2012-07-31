
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
#include <signal.h>


using namespace std;

#define HEADER_SIZE  (12)

DS1000E * scope = NULL;


void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void ConnectDevice(uint16_t vendID, uint16_t prodID, const std::string & sernum)
{
    
}

void DeviceComm(FramedConnection & conn, const std::vector<uint8_t> & msg)
{
    uint32_t writeSize = ntohl(*(uint32_t*)&msg[4]);
    uint32_t readSize = ntohl(*(uint32_t*)&msg[8]);
    
    if(msg.size() != writeSize + HEADER_SIZE) {
        cerr << "Dropped message with bad length" << endl;
        return;
    }
    if(!scope) {
        cerr << "No scope connected!" << endl;
        return;
    }
    
    if(writeSize) {
        // cout << "write: " << string(msg.begin() + HEADER_SIZE, msg.end()) << endl;
        scope->Write(&msg[HEADER_SIZE], msg.size() - HEADER_SIZE);
    }
    
    std::vector<uint8_t> resp(readSize + HEADER_SIZE);
    if(readSize)
        readSize = scope->Read(&resp[HEADER_SIZE], readSize);
    
    resp[0] = 0x01;
    resp[1] = 0x00;
    resp[2] = msg[2];
    resp[3] = msg[2];
    *(uint32_t*)&(resp)[4] = htonl(readSize);
    *(uint32_t*)&(resp)[8] = 0;
    
    // if(readSize > 128)
    //     printf("long response: %d B\n", (int)(readSize + HEADER_SIZE));
    // else
    // {
    //     printf("response: %d B\n", (int)(readSize + HEADER_SIZE));
    //     for(ssize_t j = 0; j < readSize + HEADER_SIZE; ++j)
    //         printf(" %02X", msg[j]);
    //     printf("\n");
    // }
    
    if(readSize)
        conn.SendMessage((uint8_t*)&resp[0], readSize + HEADER_SIZE);
}

int main(int argc, const char * argv[])
{
    try {
        Host host("9393");
        cout << "Server started" << endl;
        
        struct sigaction sa;
        sa.sa_handler = sigchld_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if(sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }
        
        Socket * sock = NULL;
        while(!sock)
        {
            usleep(100000);
            sock = host.Accept();
            
            if(sock)
            {
                if(fork())
                {
                    // If the main process, delete sock and go back to waiting for connections
                    // Else loop breaks
                    delete sock;
                    sock = NULL;
                }
            }
        }
        cout << "Client connected: " << sock->other << endl;
        host.Close();
        
        // TODO: postpone fork until after device connect stage?
        // Smarter handling of attempts to access a given device, potential support for both
        // multi-access and exclusive access...
        
        uint32_t timeout = 0;
        FramedConnection conn(sock->sockfd);
        while(1)
        {
            if(timeout++ > 10000) {
                cerr << "connection timed out" << endl;
                break;
            }
            
            usleep(1000);
            if(conn.Poll())
            {
                std::vector<uint8_t> * msg = conn.PopMessage();
                
                if(msg->size() < HEADER_SIZE) {
                    cerr << "Dropped short message (msg size: " << msg->size() << ")" << endl;
                    delete msg;
                    continue;
                }
                uint8_t cmd = (*msg)[0];
                uint8_t cmd2 = (*msg)[1];
                uint8_t seqnum = (*msg)[2];
                uint8_t seqnum2 = (*msg)[3];
                
                if(seqnum != (0xFF & ~seqnum2)) {
                    delete msg;
                    cerr << "Dropped message with bad sequence number" << endl;
                    continue;
                }
                
                timeout = 0;
                
                if(cmd == 1 && cmd2 == 1)
                {
                    uint32_t sernumSize = ntohl(*(uint32_t*)&(*msg)[4]);
                    uint32_t vidpid = ntohl(*(uint32_t*)&(*msg)[8]);
                    uint16_t vendorID = (vidpid >> 16) & 0xFFFF;
                    uint16_t productID = vidpid & 0xFFFF;
                    // uint16_t vendorID = ntohs(*(uint16_t*)&msg[8]);
                    // uint16_t productID = ntohs(*(uint16_t*)&msg[10]);
                    // printf("received: \n");
                    // for(ssize_t j = 0; j < msg->size(); ++j)
                    //     printf(" %02X", (*msg)[j]);
                    // printf("\n");
                    string sernum(msg->begin() + HEADER_SIZE, msg->begin() + HEADER_SIZE + sernumSize);
                    // printf("vid: %04X pid: %04X, %s\n", vendorID, productID, sernum.c_str());
                    // scope = new DS1000E(new TMC_LocalDevice(0x1AB1, 0x0588, sernum));
                    if(scope)
                        delete scope;
                    scope = new DS1000E(new TMC_LocalDevice(vendorID, productID, sernum));
                }
                else if(cmd == 10 && cmd2 == 0)
                {
                    DeviceComm(conn, *msg);
                }
                else {
                    cerr << "Received message with unsupported command" << endl;
                }
                
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


