
#ifndef REMOTEDEVICE_H
#define REMOTEDEVICE_H

#include "netcomm.h"
#include "freetmc.h"
#include "protocol.h"

#include <cstdlib>
#include <vector>
#include <string>
#include <exception>

#include <cstdarg>


class TMC_RemoteDevice: public TMC_Device {
    FramedConnection conn;
    uint8_t seqNum;
  public:
    TMC_RemoteDevice(uint16_t vendID, uint16_t prodID, const std::string & sernum, int sockfd):
        conn(sockfd),
        seqNum(0)
    {
        // conn.SetBlocking();
        std::vector<uint8_t> bfr(sernum.size() + PKT_HEADER_SIZE);
        std::copy(sernum.begin(), sernum.end(), bfr.begin() + PKT_HEADER_SIZE);
        bfr[0] = 1;
        bfr[1] = 1;
        bfr[2] = seqNum++;
        bfr[3] = ~bfr[2];
        *(uint32_t*)&(bfr)[4] = htonl(sernum.size());
        *(uint32_t*)&(bfr)[8] = htonl((uint32_t)vendID << 16 | prodID);
        // *(uint16_t*)&(bfr)[8] = htons(vendID);
        // *(uint16_t*)&(bfr)[10] = htons(prodID);
        conn.SendMessage(&bfr[0], bfr.size());
    }
    virtual ~TMC_RemoteDevice() {}
    
    virtual size_t Write(const uint8_t * msg, size_t len)
    {
        std::vector<uint8_t> bfr(len + PKT_HEADER_SIZE);
        std::copy(msg, msg + len, bfr.begin() + PKT_HEADER_SIZE);
        bfr[0] = 10;
        bfr[1] = 0;
        bfr[2] = seqNum++;
        bfr[3] = ~bfr[2];
        *(uint32_t*)&(bfr)[4] = htonl(len);
        *(uint32_t*)&(bfr)[8] = 0;
        conn.SendMessage(&bfr[0], bfr.size());
        return len;
    }
    
    virtual void StartRead(uint8_t * msg, size_t len)
    {
        uint8_t bfr[PKT_HEADER_SIZE];
        bfr[0] = 10;
        bfr[1] = 0;
        bfr[2] = seqNum++;
        bfr[3] = ~bfr[2];
        *(uint32_t*)&(bfr)[4] = 0;
        *(uint32_t*)&(bfr)[8] = htonl(len);
        conn.SendMessage(bfr, PKT_HEADER_SIZE);
    }
    
    virtual ssize_t FinishRead(uint8_t * msg, size_t len)
    {
        std::vector<uint8_t> * resp = conn.WaitPopMessage();
        size_t nbytes = std::min(len, resp->size() - PKT_HEADER_SIZE);
        std::copy(resp->begin() + PKT_HEADER_SIZE, resp->begin() + PKT_HEADER_SIZE + nbytes, msg);
        delete resp;
        return nbytes;
    }
};


#endif // REMOTEDEVICE_H


