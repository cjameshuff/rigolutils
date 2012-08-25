
// A model consisting of a set of waveforms and instruments, managing
// including captures, periodic updates, generation of products from input
// waveforms, etc.
// 
// A device manages a local or remote connection to an instrument.
// A channel represents captured data, with sample rate and time reference.
// If channel device is NULL, 



#ifndef SCOPEV_MODEL_H
#define SCOPEV_MODEL_H
//******************************************************************************

#include "freetmc_local.h"
#include "freetmc_remote.h"
#include "plotting.h"
#include "rigoltmc.h"

#include "rigol_ds1k.h"
#include "cfgmap.h"
#include "selector.h"

#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct Device;

enum ChannelSource {
    ChSrc_Device,
    ChSrc_File,
    ChSrc_Math
};

struct Channel {
    bool enabled;
    bool updated;
    ChannelSource source;
    Device * device;
    sel_t name;
    Waveform waveform;
};

struct Device {
    std::string location;
    uint16_t vid;
    uint16_t pid;
    std::string sernum;
    TMC_Device * tmcDevice;
    std::vector<Channel> channels;
    bool connected;
    
    Device(const std::string & l, uint16_t vid, uint16_t pid, const std::string & s);
    ~Device();
};


class ScopeModel {
  protected:
    std::map<std::string, Device *> devices;
    
  public:
    ScopeModel();
    ~ScopeModel();
    
    void Update();
    
    Device * GetDevice(const std::string & sernum) {return devices[sernum];}
    
    
    void Connect(const std::string & l, uint16_t vid, uint16_t pid, const std::string & s);
};

class ScopeServerFinder: public ServerFinder {
    std::vector<std::string> servers;
  public:
    ScopeServerFinder();
    
    std::vector<std::string> & GetServers() {return servers;}
    
    void Query(const std::set<uint32_t> & VIDPIDs);
    
    virtual bool HandleResponse(uint8_t * buffer, size_t msgLen, sockaddr_in & srcAddr, socklen_t srcAddrLen);
};


//******************************************************************************
#endif // SCOPEV_MODEL_H
