
#include "scopev_model.h"
#include "freetmc_local.h"
#include "freetmc_remote.h"

using namespace std;

//******************************************************************************
ScopeServerFinder::ScopeServerFinder():
    ServerFinder(DISC_BCAST_ADDR, DISC_QUERY_PORT, DISC_RESP_PORT)
{}

void ScopeServerFinder::Query(const set<uint32_t> & VIDPIDs)
{
    vector<uint8_t> query(PKT_HEADER_SIZE);
    PKT_SetCMD(query, CMD_PING);
    PKT_SetSeq(query, 0x55);
    PKT_SetPayloadSize(query, 0);
    
    SendQuery(&query[0], query.size());
}

bool ScopeServerFinder::HandleResponse(uint8_t * buffer, size_t msgLen, sockaddr_in & srcAddr, socklen_t srcAddrLen)
{
    cerr << "msgLen: " << msgLen << endl;
    
    if(msgLen < PKT_HEADER_SIZE) {
        cerr << "Dropped short discovery response (msg size: " << msgLen << ")" << endl;
        return false;
    }
    uint16_t cmd = PKT_CMD(buffer);
    uint8_t seqnum = PKT_SEQ(buffer);
    uint8_t seqnum2 = PKT_SEQ2(buffer);
    uint32_t payloadSize = PKT_PAYLOAD_SIZE(buffer);
    uint8_t * payload = PKT_PAYLOAD(buffer);
    
    if(!PKT_SEQ_GOOD(buffer)) {
        cerr << "Dropped discovery response with bad sequence number" << endl;
        return false;
    }
    
    if((payloadSize + PKT_HEADER_SIZE) != msgLen) {
        cerr << "Dropped discovery response with bad length" << endl;
        return false;
    }
    
    string response(payload, payload + payloadSize);
    // char s[INET6_ADDRSTRLEN];
    // struct sockaddr * sa = (struct sockaddr *)&srcAddr;
    // if(sa->sa_family == AF_INET)
    //     inet_ntop(sa->sa_family, &(((struct sockaddr_in *)sa)->sin_addr), s, INET6_ADDRSTRLEN);
    // else
    //     inet_ntop(sa->sa_family, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, INET6_ADDRSTRLEN);
    // cerr << "Found server \"" << response << "\" at " << s << endl;
    servers.push_back(response);
    return true;
}


//******************************************************************************
Device::Device(const std::string & l, uint16_t v, uint16_t p, const std::string & s):
    location(l),
    sernum(s),
    vid(v),
    pid(p),
    tmcDevice(NULL),
    connected(false)
{
    if(location == "USB" || location == "")
    {
        tmcDevice = new TMC_LocalDevice(vid, pid, sernum);
    }
    else
    {
        std::cerr << "Attempting to connect to " << location << std::endl;
        Socket * sock = NULL;
        while(!sock) {
            usleep(10000);
            sock = ClientConnect(location, "9393");
        }
        std::cerr << "Connected to server: " << sock->other << std::endl;
        
        tmcDevice = new TMC_RemoteDevice(vid, pid, sernum, sock->sockfd);
        sock->sockfd = -1;// TMC_RemoteDevice takes ownership of fd
        delete sock;
    }
    
    connected = true;
}

Device::~Device()
{
    if(tmcDevice)
        delete tmcDevice;
}

//******************************************************************************

// void UpdateWaveforms(PlotOpts & opts, Device * dev)
// {
//     DS1000E & scope = *(dev->device);
//     
//     scope.Run();
//     sleep(1);
//     scope.Force();
//     sleep(1);
//     scope.Stop();
//     // cout << "Channel 1 scale: " << scope.ChanScale(0) << endl;
//     // cout << "Channel 1 offset: " << scope.ChanOffset(0) << endl;
//     // cout << "Time scale: " << scope.TimeScale() << endl;
//     // cout << "Time offset: " << scope.TimeOffset() << endl;
//     
//     cout << ":WAV:POIN:MODE? -> " << scope.Query(":WAV:POIN:MODE?") << endl;
//     scope.RawMode();
//     
//     opts.divT = scope.TimeScale();
//     opts.divV = 1.0;
//     cout << "opts.divT: " << opts.divT << endl;
//     // cout << ":WAV:POIN:MODE? -> " << scope.Query(":WAV:POIN:MODE?") << endl;
// }

// Scope responds to :TRIG:STAT? with:
// RUN, STOP, T`D (check quote character), WAIT, or AUTO

// Update procedure:
// STATE_NEEDS_UPDATE
//   Send :WAV:DATA? CHANx query
//   StartRead()
//   Go to state STATE_UPDATE_IN_PROGRESS
// 
// STATE_UPDATE_IN_PROGRESS
//   Poll FinishRead()
//   If return value > 0, handle update and go to state STATE_UPDATED or STATE_NEEDS_UPDATE
// 
// STATE_UPDATED
//   Do nothing

ScopeModel::ScopeModel()
{
    
}

ScopeModel::~ScopeModel()
{
    
}

void ScopeModel::Update()
{
    map<string, Device *>::iterator di;
    for(di = devices.begin(); di != devices.end(); ++di)
    {
        Device & dev = *(di->second);
        for(size_t k = 0; k < dev.channels.size(); ++k)
        {
            if(dev.channels[k].updated)
            {
                
                dev.channels[k].updated = false;
            }
        }
    }
}


void ScopeModel::Connect(const std::string & loc, uint16_t vid, uint16_t pid, const std::string & sernum)
{
    Device * dev = new Device(loc, vid, pid, sernum);
    
    devices[sernum] = dev;
    
    cout << "Sending identify" << endl;
    IDN_Response idn = dev->tmcDevice->Identify();
    cout << "Manufacturer: " << idn.manufacturer << endl;
    cout << "Model: " << idn.model << endl;
    cout << "Serial: " << idn.serial << endl;
    cout << "Version: " << idn.version << endl;
    //device = new DS1000E(dev);
}

