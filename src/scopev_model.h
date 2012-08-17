
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

#include "freetmc_remote.h"
#include "plotting.h"
#include "rigoltmc.h"

#include "selector.h"

#include <iostream>
#include <string>
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
    std::string sernum;
    DS1000E * device;
    std::vector<Channel> channels;
    bool connected;
    
    Device(const std::string & l, const std::string & s);
    ~Device();
    
    void Connect();
};


class ScopeModel {
  protected:
    std::map<sel_t, Device *> devices;
    
  public:
    ScopeModel();
    ~ScopeModel();
    
    void Update();
    
    void Connect(const std::string & l, uint16_t vid, uint16_t pid, const std::string & s);
};


//******************************************************************************
#endif // SCOPEV_MODEL_H
