
#include "scopev_model.h"
#include "freetmc_local.h"
#include "freetmc_remote.h"

using namespace std;

//******************************************************************************
Device::Device(const std::string & l, const std::string & s):
    location(l),
    sernum(s),
    device(NULL),
    connected(false)
{
    
}

Device::~Device()
{
    if(device)
        delete device;
}

void Device::Connect()
{
    std::cerr << "Attempting to connect" << std::endl;
    Socket * sock = NULL;
    while(!sock) {
        usleep(10000);
        sock = ClientConnect(location, "9393");
    }
    std::cerr << "Connected to server: " << sock->other << std::endl;
    
    channels.resize(2);
    for(size_t j = 0; j < channels.size(); ++j)
    {
        channels[j].enabled = false;
        channels[j].updated = false;
    }
    device = new DS1000E(new TMC_RemoteDevice(0x1AB1, 0x0588, sernum, sock->sockfd));
    sock->sockfd = -1;// TMC_RemoteDevice takes ownership of fd
    delete sock;
    connected = true;
}

//******************************************************************************

void UpdateWaveforms(PlotOpts & opts, Device * dev)
{
    DS1000E & scope = *(dev->device);
    
    scope.Run();
    sleep(1);
    scope.Force();
    sleep(1);
    scope.Stop();
    // cout << "Channel 1 scale: " << scope.ChanScale(0) << endl;
    // cout << "Channel 1 offset: " << scope.ChanOffset(0) << endl;
    // cout << "Time scale: " << scope.TimeScale() << endl;
    // cout << "Time offset: " << scope.TimeOffset() << endl;
    
    cout << ":WAV:POIN:MODE? -> " << scope.Query(":WAV:POIN:MODE?") << endl;
    scope.RawMode();
    
    opts.divT = scope.TimeScale();
    opts.divV = 1.0;
    cout << "opts.divT: " << opts.divT << endl;
    // cout << ":WAV:POIN:MODE? -> " << scope.Query(":WAV:POIN:MODE?") << endl;
}

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
    map<sel_t, Device *>::iterator di;
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
    TMC_Device * dev = NULL;
    if(loc == "USB" || loc == "")
    {
        dev = new TMC_LocalDevice(vid, pid, sernum);
    }
    else
    {
        std::cerr << "Attempting to connect" << std::endl;
        Socket * sock = NULL;
        while(!sock) {
            usleep(10000);
            sock = ClientConnect(loc, "9393");
        }
        std::cerr << "Connected to server: " << sock->other << std::endl;
        
        dev = new TMC_RemoteDevice(vid, pid, sernum, sock->sockfd);
        sock->sockfd = -1;// TMC_RemoteDevice takes ownership of fd
        delete sock;
    }
    
    cout << "Sending identify" << endl;
    IDN_Response idn = dev->Identify();
    cout << "Manufacturer: " << idn.manufacturer << endl;
    cout << "Model: " << idn.model << endl;
    cout << "Serial: " << idn.serial << endl;
    cout << "Version: " << idn.version << endl;
    //device = new DS1000E(dev);
}

