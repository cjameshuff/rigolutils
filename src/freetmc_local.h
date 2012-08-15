
#ifndef FREETMC_LOCAL_H
#define FREETMC_LOCAL_H

#include "freetmc.h"

struct TMC_Descriptor;
class TMC_LocalDevice: public TMC_Device {
    TMC_Descriptor * desc;
    std::string sernum;
  public:
    TMC_LocalDevice(uint16_t vendID, uint16_t prodID, const std::string & sn);
    virtual ~TMC_LocalDevice();
    
    virtual size_t Write(const uint8_t * msg, size_t len);
    
    virtual void StartRead(uint8_t * msg, size_t nbytes);
    virtual ssize_t FinishRead(uint8_t * msg, size_t nbytes);
};


#endif // FREETMC_LOCAL_H
