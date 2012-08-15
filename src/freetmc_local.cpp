// g++ freetmc.cpp -L/usr/local/lib -lusb-1.0 -o usbcon

#include "freetmc_local.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <algorithm>

#include <libusb-1.0/libusb.h>

using namespace std;

const char * libusb_error_strs[] = {
	"LIBUSB_SUCCESS",
	"LIBUSB_ERROR_IO",
	"LIBUSB_ERROR_INVALID_PARAM",
	"LIBUSB_ERROR_ACCESS",
	"LIBUSB_ERROR_NO_DEVICE",
	"LIBUSB_ERROR_NOT_FOUND",
	"LIBUSB_ERROR_BUSY",
	"LIBUSB_ERROR_TIMEOUT",
	"LIBUSB_ERROR_OVERFLOW",
	"LIBUSB_ERROR_PIPE",
	"LIBUSB_ERROR_INTERRUPTED",
	"LIBUSB_ERROR_NO_MEM",
	"LIBUSB_ERROR_NOT_SUPPORTED",
	"LIBUSB_ERROR_OTHER",
};

#define libusb_error_str(x) libusb_error_strs[(-(x) > 12)? 13 : -(x)]

#define TMC_HDR_SIZE   (12)

#define INTERRUPT_ENDPOINT_IN   (0x83)
#define BULK_ENDPOINT_IN   (0x82)
#define BULK_ENDPOINT_OUT  (0x01)

static void print_dev(libusb_device * dev)
{
    struct libusb_device_descriptor desc;
    int r = libusb_get_device_descriptor(dev, &desc);
    if(r < 0) {
        fprintf(stderr, "failed to get device descriptor");
        return;
    }
    printf("%04x:%04x (bus %d, device %d)\n",
        desc.idVendor, desc.idProduct,
        libusb_get_bus_number(dev), libusb_get_device_address(dev));
}

const uint8_t DEV_DEP_MSG_OUT      = 1;   // OUT
const uint8_t REQ_DEV_DEP_MSG_IN   = 2;   // OUT
const uint8_t DEV_DEP_MSG_IN       = 2;   // IN
const uint8_t VEND_SPECIFIC_OUT    = 126; // OUT
const uint8_t REQ_VEND_SPECIFIC_IN = 127; // OUT
const uint8_t VEND_SPECIFIC_IN     = 127; // IN

static int NumTMC_LocalDevices = 0;

struct TMC_Descriptor {
    libusb_device_handle * hand;
    bool isRigol;
    
    uint8_t bTag; // Transfer identifier (sequence number). 1 < bTAG <= 255
    uint16_t maxPacketSizeOUT;
    uint16_t maxPacketSizeIN;
    
    TMC_Descriptor(libusb_device_handle * h):
        hand(h),
        isRigol(false),
        bTag(1),
        maxPacketSizeOUT(0),
        maxPacketSizeIN(0)
    {}
    ~TMC_Descriptor() {if(hand) libusb_close(hand);}
};



TMC_Descriptor * OpenTMC_LocalDevice(libusb_device * dev, libusb_device_handle * hand, struct libusb_device_descriptor & devdesc)
{
    int r;
    TMC_Descriptor * desc = new TMC_Descriptor(hand);
    // desc->isRigol = false;
    desc->isRigol = (devdesc.idVendor == 0x1AB1);
    
    fprintf(stderr, "libusb_get_config_descriptor:\n");
    struct libusb_config_descriptor * cfg;
    libusb_get_config_descriptor(dev, 0, &cfg);
    
    const struct libusb_interface * itf = cfg->interface;
    const struct libusb_interface_descriptor * alt = itf->altsetting;
    const struct libusb_endpoint_descriptor * epOUT = &(alt->endpoint[BULK_ENDPOINT_OUT & 0x7F]);
    const struct libusb_endpoint_descriptor * epIN = &(alt->endpoint[BULK_ENDPOINT_IN & 0x7F]);
    
    desc->maxPacketSizeOUT = epOUT->wMaxPacketSize;
    desc->maxPacketSizeIN = epIN->wMaxPacketSize;
    
    fprintf(stderr, "libusb_set_configuration:\n");
    r = libusb_set_configuration(desc->hand, 1);
    if(r < 0)
        throw FormattedError("libusb_set_configuration() failed (%d)", r);
    
    fprintf(stderr, "libusb_claim_interface:\n");
    r = libusb_claim_interface(desc->hand, 0);
    if(r < 0)
        throw FormattedError("libusb_claim_interface() failed (%d)", r);
    
    fprintf(stderr, "libusb_clear_halt IN:\n");
    r = libusb_clear_halt(desc->hand, BULK_ENDPOINT_IN);
    if(r < 0)
        throw FormattedError("libusb_clear_halt() failed on IN bulk endpoint (%d)", r);
    
    fprintf(stderr, "libusb_clear_halt OUT:\n");
    r = libusb_clear_halt(desc->hand, BULK_ENDPOINT_OUT);
    if(r < 0)
        throw FormattedError("libusb_clear_halt() failed on OUT bulk endpoint (%d)", r);
    
    fprintf(stderr, "Device opened and initialized\n");
    return desc;
}


TMC_LocalDevice::TMC_LocalDevice(uint16_t vendID, uint16_t prodID, const std::string & sn):
    desc(NULL),
    sernum(sn)
{
    int r;
    
    if(NumTMC_LocalDevices == 0)
    {
        r = libusb_init(NULL);
        if(r < 0)
            throw FormattedError("libusb_init() failed");
    }
    ++NumTMC_LocalDevices;
    
    printf("searching for matching device\n");
    
    libusb_device ** devs;
    ssize_t cnt = libusb_get_device_list(NULL, &devs);
    if(cnt < 0)
        throw FormattedError("libusb_get_device_list() failed");
    
    desc = NULL;
    for(libusb_device ** dev = devs; *dev != NULL && !desc; dev++)
    {
        libusb_device_handle * hand;
        fprintf(stderr, "libusb_open:\n");
        r = libusb_open(*dev, &hand);
        if(r < 0) {
            fprintf(stderr, "couldn't open device\n");
            continue;
        }
        
        struct libusb_device_descriptor devdesc;
        r = libusb_get_device_descriptor(*dev, &devdesc);
        if(r < 0)
            throw FormattedError("failed to get device descriptor");
        
        fprintf(stderr, "%04x:%04x (bus %d, device %d)\n", devdesc.idVendor, devdesc.idProduct,
            libusb_get_bus_number(*dev), libusb_get_device_address(*dev));
        
        if(devdesc.idVendor == vendID && devdesc.idProduct == prodID)
        {
            fprintf(stderr, "Found VID/PID match\n");
            char foundSernum[1024] = {'\0'};
            if(devdesc.iSerialNumber)
                libusb_get_string_descriptor_ascii(hand, devdesc.iSerialNumber, (uint8_t*)foundSernum, sizeof(foundSernum));
            
            fprintf(stderr, "Device serial number: %s\n", foundSernum);
            // Vendor and product ID match, check serial number or take first found if no serial number specified
            if(sernum == "" || sernum == foundSernum)
            {
                desc = OpenTMC_LocalDevice(*dev, hand, devdesc);
                hand = NULL;// desc has ownership of handle now
                fprintf(stderr, "Device %s opened\n", foundSernum);
            }
        } // if(vid and pid match)
        
        if(hand)
            libusb_close(hand);
    } // for(devices)
    
    libusb_free_device_list(devs, 1);
    if(!desc)
        throw FormattedError("No devices found");
}

TMC_LocalDevice::~TMC_LocalDevice()
{
    delete desc;
    --NumTMC_LocalDevices;
    if(NumTMC_LocalDevices == 0)
    {
        libusb_exit(NULL);
    }
}


#define TIMEOUT  10000

size_t TMC_LocalDevice::Write(const uint8_t * msg, size_t len)
{
    // TODO: multi-transfer writes
    int r;
    // "The total number of bytes in each Bulk-OUT transaction must be a multiple of 4.The Host must add 0
    // to a maximum of 3 extra alignment bytes to the last transaction payload to achieve 4-byte (32-bit)
    // alignment. The alignment bytes should be 0x00-valued, but this is not required."
    int size = ((len % 4) != 0)? (len - (len % 4) + 4) : (len);
    
    std::vector<uint8_t> bfr(size + TMC_HDR_SIZE);
    bfr[0] = DEV_DEP_MSG_OUT;
    bfr[1] = desc->bTag;
    bfr[2] = ~desc->bTag;
    desc->bTag = ((desc->bTag == 255)? 1 : desc->bTag + 1);
    bfr[3] = 0;
    // Transfer size
    bfr[4] = len & 0xFF;
    bfr[5] = (len >> 8) & 0xFF;
    bfr[6] = (len >> 16) & 0xFF;
    bfr[7] = (len >> 24) & 0xFF;
    // Attributes
    bfr[8] = 1;// Last transfer of message
    bfr[9] = 0;
    bfr[10] = 0;
    bfr[11] = 0;
    
    std::copy(msg, msg + len, bfr.begin() + TMC_HDR_SIZE);
    for(int j = len + TMC_HDR_SIZE; j < bfr.size(); ++j)
        bfr[j] = 0x00;
    
    int actualLength;
/*    if(desc->isRigol)// send header and payload separately
    {
        r = libusb_bulk_transfer(desc->hand, BULK_ENDPOINT_OUT, &bfr[0], TMC_HDR_SIZE, &actualLength, 1000);
        if(r < 0)
            throw FormattedError("libusb_bulk_transfer() failed: %s", libusb_error_str(r));
        
        r = libusb_bulk_transfer(desc->hand, BULK_ENDPOINT_OUT, &bfr[TMC_HDR_SIZE], bfr.size() - TMC_HDR_SIZE, &actualLength, 1000);
        if(r < 0)
            throw FormattedError("libusb_bulk_transfer() failed: %s", libusb_error_str(r));
    }
    else*/
    {
        r = libusb_bulk_transfer(desc->hand, BULK_ENDPOINT_OUT, &bfr[0], bfr.size(), &actualLength, 1000);
        if(r < 0)
            throw FormattedError("libusb_bulk_transfer() failed: %s", libusb_error_str(r));
    }
    return actualLength;
}


void TMC_LocalDevice::StartRead(uint8_t * msg, size_t nbytes)
{
    int r;
    uint8_t req[TMC_HDR_SIZE];
    req[0] = REQ_DEV_DEP_MSG_IN;
    
    uint8_t bTag = desc->bTag;
    desc->bTag = ((desc->bTag == 255)? 1 : desc->bTag + 1);
    req[1] = bTag;
    req[2] = ~bTag;
    req[3] = 0;
    
    // Transfer size
    req[4] = nbytes & 0xFF;
    req[5] = (nbytes >> 8) & 0xFF;
    req[6] = (nbytes >> 16) & 0xFF;
    req[7] = (nbytes >> 24) & 0xFF;
    
    // Attributes
    req[8] = 0;
    req[9] = 0;
    req[10] = 0;
    req[11] = 0;
    
    r = libusb_clear_halt(desc->hand, BULK_ENDPOINT_IN);
    if(r < 0)
        throw FormattedError("libusb_clear_halt() failed on IN bulk endpoint: %s", libusb_error_str(r));
    
    r = libusb_clear_halt(desc->hand, BULK_ENDPOINT_OUT);
    if(r < 0)
        throw FormattedError("libusb_clear_halt() failed on OUT bulk endpoint: %s", libusb_error_str(r));
    
    int actualLength;
    r = libusb_bulk_transfer(desc->hand, BULK_ENDPOINT_OUT, req, TMC_HDR_SIZE, &actualLength, TIMEOUT);
    // cerr << "r: " << r << endl;
    if(r < 0 || actualLength < TMC_HDR_SIZE)
        throw FormattedError("libusb_bulk_transfer() failed (TMC_BulkIN(), request phase): %s", libusb_error_str(r));
}


ssize_t TMC_LocalDevice::FinishRead(uint8_t * msg, size_t nbytes)
{
    int r;
    // Read data
    // cerr << "nbytes to transfer: " << nbytes << endl;
    std::vector<uint8_t> bfr(TMC_HDR_SIZE + nbytes);
    int actualLength;
    size_t bytesReceived = 0;
    do {
        if(desc->isRigol)
        {
            // Rigol scopes evidently split long responses into two transactions.
            int actualLength1;
            r = libusb_bulk_transfer(desc->hand, BULK_ENDPOINT_IN, &bfr[0], 64, &actualLength1, TIMEOUT);
            // cerr << "actualLength1: " << actualLength1 << endl;
            if(r < 0)
                throw FormattedError("libusb_bulk_transfer() failed (TMC_BulkIN(), data phase 1): %s", libusb_error_str(r));
        
            size_t headerSize = ((int)bfr[7] << 24) | ((int)bfr[6] << 16) | ((int)bfr[5] << 8) | (int)bfr[4];
            // cerr << "headerSize1: " << headerSize << endl;
            if((headerSize + TMC_HDR_SIZE) > 64)
            {
                r = libusb_bulk_transfer(desc->hand, BULK_ENDPOINT_IN, &bfr[actualLength1],
                    nbytes + TMC_HDR_SIZE - actualLength1, &actualLength, TIMEOUT);
                // r = libusb_bulk_transfer(desc->hand, BULK_ENDPOINT_IN, &bfr[actualLength1],
                //    headerSize + TMC_HDR_SIZE - actualLength1, &actualLength, TIMEOUT);
                actualLength += actualLength1;
                // cerr << "actualLength2: " << actualLength << endl;
                if(r < 0)
                    throw FormattedError("libusb_bulk_transfer() failed (TMC_BulkIN(), data phase 2): %s", libusb_error_str(r));
            }
            else
            {
                actualLength = actualLength1;
            }
        }
        else
        {
            r = libusb_bulk_transfer(desc->hand, BULK_ENDPOINT_IN, &bfr[0], nbytes + TMC_HDR_SIZE, &actualLength, TIMEOUT);
            // cerr << "actualLength: " << actualLength << endl;
            if(r < 0)
                throw FormattedError("libusb_bulk_transfer() failed (TMC_BulkIN(), data phase): %s", libusb_error_str(r));
        }
    
        size_t headerSize = ((int)bfr[7] << 24) | ((int)bfr[6] << 16) | ((int)bfr[5] << 8) | (int)bfr[4];
        // cerr << "bTag: " << (int)bTag << endl;
        // cerr << "actualLength: " << actualLength << endl;
        // cerr << "bfr[0]: " << (int)bfr[0] << endl;
        // cerr << "bfr[1]: " << (int)bfr[1] << endl;
        // cerr << "~bfr[2]: " << (int)(~bfr[2] & 0xFF) << endl;
        // cerr << "headerSize: " << headerSize << endl;
        if(bfr[0] != DEV_DEP_MSG_IN)
            throw FormattedError("TMC_BulkIN(): wrong message type received");
    
        // if(bfr[1] != bTag || bfr[2] != (~bTag & 0xFF))
        //     throw FormattedError("TMC_BulkIN(): bad sequence number");
    
        // TODO:
        // If sequence number is good but incorrect for request, ignore and try again?
    
        if(headerSize > actualLength - TMC_HDR_SIZE)
            throw FormattedError("TMC_BulkIN(): bad header size");
    
        // actualLength contains number of bytes actually received
        std::copy(bfr.begin() + TMC_HDR_SIZE, bfr.begin() + actualLength, msg + bytesReceived);
        bytesReceived += actualLength - TMC_HDR_SIZE;
    } while((bfr[8] & 0x01) == 0);
    return bytesReceived;
}
