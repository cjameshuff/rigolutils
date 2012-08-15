
#include "freetmc_local.h"
#include "rigoltmc.h"
#include "netcomm.h"
#include "protocol.h"

#include <iostream>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <arpa/inet.h>
#include <signal.h>


using namespace std;

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
    uint32_t payloadSize = PKT_PAYLOAD_SIZE(&msg[0]);
    const uint8_t * cmdPayload = PKT_PAYLOAD(&msg[0]);
    uint32_t readSize = ntohl(*(uint32_t*)&cmdPayload[0]);
    uint32_t bytesRead = 0;
    
    if(!scope) {
        cerr << "No scope connected!" << endl;
    }
    
    if(scope && payloadSize > 4) {
        cout << "write: " << string(cmdPayload + 4, cmdPayload + payloadSize) << endl;
        scope->Write(cmdPayload + 4, payloadSize - 4);
    }
    
    std::vector<uint8_t> resp(readSize + PKT_HEADER_SIZE);
    PKT_SetCMD(resp, CMD_DEVICE_COMM);
    PKT_SetSeq(resp, msg[2]);
    uint8_t * respPayload = PKT_PAYLOAD(resp);
    if(scope && readSize)
        bytesRead = scope->Read(respPayload, readSize);
    PKT_SetPayloadSize(resp, bytesRead);
    
    if(bytesRead > 128)
        printf("long response: %d B\n", (int)(PKT_SIZE(resp)));
    else
    {
        printf("response: %d B\n", (int)(PKT_SIZE(resp)));
        for(ssize_t j = 0; j < PKT_SIZE(resp); ++j)
            printf(" %02X", msg[j]);
        printf("\n");
    }
    
    if(readSize)
        conn.SendMessage((uint8_t*)&resp[0], PKT_SIZE(resp));
}

//******************************************************************************

class ScopeDiscoverableServer: public DiscoverableServer {
  public:
    ScopeDiscoverableServer(const std::string & a, uint16_t qp, uint16_t rp): DiscoverableServer(a, qp, rp)
    {
    }
    
    virtual bool HandleRequest(uint8_t * buffer, size_t msgLen, sockaddr_in & srcAddr, socklen_t srcAddrLen)
    {
        if(msgLen < PKT_HEADER_SIZE) {
            cerr << "Dropped short discovery request (msg size: " << msgLen << ")" << endl;
            return false;
        }
        uint16_t cmd = PKT_CMD(buffer);
        uint8_t seqnum = PKT_SEQ(buffer);
        uint8_t seqnum2 = PKT_SEQ2(buffer);
        uint32_t payloadSize = PKT_PAYLOAD_SIZE(buffer);
        
        if(!PKT_SEQ_GOOD(buffer)) {
            cerr << "Dropped discovery request with bad sequence number" << endl;
            return false;
        }
        
        if(payloadSize != 0) {
            cerr << "Dropped discovery request with bad length" << endl;
            return false;
        }
        
        char s[INET6_ADDRSTRLEN];
        struct sockaddr * sa = (struct sockaddr *)&srcAddr;
        if(sa->sa_family == AF_INET)
            inet_ntop(sa->sa_family, &(((struct sockaddr_in *)sa)->sin_addr), s, INET6_ADDRSTRLEN);
        else
            inet_ntop(sa->sa_family, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, INET6_ADDRSTRLEN);
        std::cerr << "Received discovery query from: " << s << std::endl;
        // cerr << "Sent response" << endl;
        
        string servName = "Test Server";
        std::vector<uint8_t> resp(servName.size() + PKT_HEADER_SIZE);
        PKT_SetCMD(resp, CMD_PING);
        PKT_SetSeq(resp, 0);
        PKT_SetPayloadSize(resp, servName.size());
        uint8_t * payload = PKT_PAYLOAD(resp);
        
        std::copy(servName.begin(), servName.end(), payload);
        
        SendResponse(&resp[0], resp.size(), srcAddr, srcAddrLen);
        return true;
    }
};

//******************************************************************************

int main(int argc, const char * argv[])
{
    try {
        struct sigaction sa;
        sa.sa_handler = sigchld_handler;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if(sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }
        
        Host host("9393");
        cout << "Server started" << endl;
        
        ScopeDiscoverableServer dserv(DISC_BCAST_ADDR, DISC_QUERY_PORT, DISC_RESP_PORT);
        
        Socket * sock = NULL;
        while(!sock)
        {
            dserv.Poll();
            // cout << "."; cout.flush();
            
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
        
        // uint32_t timeout = 0;
        FramedConnection conn(sock->sockfd);
        while(1)
        {
            // if(timeout++ > 10000) {
            //     cerr << "connection timed out" << endl;
            //     break;
            // }
        
            usleep(1000);
            if(conn.Poll())
            {
                std::vector<uint8_t> * msg = conn.PopMessage();
                
                if(msg->size() < PKT_HEADER_SIZE) {
                    cerr << "Dropped short message (msg size: " << msg->size() << ")" << endl;
                    delete msg;
                    continue;
                }
                uint16_t cmd = PKT_CMD(*msg);
                uint8_t seqnum = PKT_SEQ(*msg);
                uint8_t seqnum2 = PKT_SEQ2(*msg);
                uint32_t payloadSize = PKT_PAYLOAD_SIZE(*msg);
                uint8_t * payload = PKT_PAYLOAD(*msg);
                
                if(!PKT_SEQ_GOOD(*msg)) {
                    cerr << "Dropped message with bad sequence number" << endl;
                    delete msg;
                    continue;
                }
                
                if((payloadSize + PKT_HEADER_SIZE) != msg->size()) {
                    cerr << "Dropped message with bad length (payload size: " << (msg->size() - PKT_HEADER_SIZE)
                        << " B, header claims " << payloadSize << " B)" << endl;
                    delete msg;
                    continue;
                }
                
                // timeout = 0;
                if(cmd == CMD_PING)
                {
                    cerr << "CMD_PING not implemented yet" << endl;
                }
                else if(cmd == CMD_DISCONNECT)
                {
                    cerr << "CMD_DISCONNECT not implemented yet" << endl;
                }
                else if(cmd == CMD_CONNECT_DEVICE)
                {
                    if(payloadSize < 4) {
                        cerr << "Bad CMD_CONNECT_DEVICE request: short payload" << endl;
                        delete msg;
                        continue;
                    }
                    uint32_t sernumSize = payloadSize - 4;
                    uint32_t vidpid = ntohl(*(uint32_t*)&payload[0]);
                    uint16_t vendorID = (vidpid >> 16) & 0xFFFF;
                    uint16_t productID = vidpid & 0xFFFF;
                    // uint16_t vendorID = ntohs(*(uint16_t*)&msg[8]);
                    // uint16_t productID = ntohs(*(uint16_t*)&msg[10]);
                    // printf("received: \n");
                    // for(ssize_t j = 0; j < msg->size(); ++j)
                    //     printf(" %02X", (*msg)[j]);
                    // printf("\n");
                    string sernum(payload + 4, payload + 4 + sernumSize);
                    printf("vid: %04X pid: %04X, %s\n", vendorID, productID, sernum.c_str());
                    if(scope)
                        delete scope;
                    scope = new DS1000E(new TMC_LocalDevice(vendorID, productID, sernum));
                }
                else if(cmd == CMD_DEVICE_COMM)
                {
                    // printf("devicecomm msg\n");
                    DeviceComm(conn, *msg);
                }
                else {
                    cerr << "Received message with unsupported command" << endl;
                }
                
                delete msg;
            } // if(Poll())
        } // while(1)
    }
    catch(exception & err)
    {
        cout << "Caught exception: " << err.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}


