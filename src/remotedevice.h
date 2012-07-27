
#ifndef REMOTEDEVICE_H
#define REMOTEDEVICE_H

#include "netcomm.h"
#include "freetmc.h"

#include <cstdlib>
#include <vector>
#include <string>
#include <exception>

#include <cstdarg>

#define HEADER_SIZE  (12)

class TMC_RemoteDevice: public TMC_Device {
    FramedConnection conn;
  public:
    TMC_RemoteDevice(uint16_t vendID, uint16_t prodID, const std::string & sernum, int sockfd): conn(sockfd) {
        // conn.SetBlocking();
    }
    virtual ~TMC_RemoteDevice() {}
    
    virtual size_t Write(const uint8_t * msg, size_t len)
    {
        std::vector<uint8_t> bfr(len + HEADER_SIZE);
        std::copy(msg, msg + len, bfr.begin() + HEADER_SIZE);
        bfr[0] = 0x01;
        bfr[1] = 0x00;
        bfr[2] = 0x01;
        bfr[3] = ~bfr[2];
        *(uint32_t*)&(bfr)[4] = htonl(len);
        *(uint32_t*)&(bfr)[8] = 0;
        conn.SendMessage(&bfr[0], bfr.size());
        return len;
    }
    
    virtual size_t Read(uint8_t * msg, size_t len)
    {
        uint8_t bfr[HEADER_SIZE];
        bfr[0] = 0x01;
        bfr[1] = 0x00;
        bfr[2] = 0x01;
        bfr[3] = ~bfr[2];
        *(uint32_t*)&(bfr)[4] = 0;
        *(uint32_t*)&(bfr)[8] = htonl(len);
        conn.SendMessage(bfr, HEADER_SIZE);
        
        std::vector<uint8_t> * resp = conn.WaitPopMessage();
        size_t nbytes = std::min(len, resp->size() - HEADER_SIZE);
        std::copy(resp->begin() + HEADER_SIZE, resp->begin() + HEADER_SIZE + nbytes, msg);
        delete resp;
        return nbytes;
    }
};


#endif // REMOTEDEVICE_H


