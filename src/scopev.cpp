

#include "remotedevice.h"

#ifdef MACOSX
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include <iostream>
#include <string>
#include <map>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <Magick++.h>

#include "rigoltmc.h"
#include "plotting.h"

using namespace std;
using namespace Magick;

inline void DisplayString(double x, double y, void * font, const char * str) {
    glRasterPos2d(x, y);
    for(const char * p = str; *p; ++p)
        glutBitmapCharacter(font, *p);
}


inline void DisplayString(double x, double y, double size, const char * str) {
    glPushMatrix();
    glTranslated(x, y, 0);
    glScaled(size, size, size);
    for(const char * p = str; *p; ++p)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, *p);
    glPopMatrix();
}

void Display();

void PlotWaveforms(const std::string foutPath, PlotOpts & opts, DS1000E & device);


// A device manages a local or remote connection to an instrument.
// A channel represents captured data, with sample rate and time reference.
// A capture collects channel data from a single device
//

struct Device {
    string location;
    string sernum;
    Socket * sock;
    DS1000E * device;
    bool connected;
    
    Device(const string & l, const string & s):
        location(l),
        sernum(s),
        sock(NULL),
        device(NULL),
        connected(false)
    {}
    
    void Connect() {
        cout << "Attempting to connect" << endl;
        while(!sock) {
            usleep(10000);
            sock = ClientConnect(location, "9393");
        }
        cout << "Connected to server: " << sock->other << endl;
        
        device = new DS1000E(new TMC_RemoteDevice(0x1AB1, 0x0588, sernum, sock->sockfd));
    }
};

class Capture {
    Device * device;
};


vector<Device *> devices;
vector<Capture *> captures;


void Capture(PlotOpts & opts)
{
    DS1000E & scope = *(devices.back()->device);
    
    cout << "Sending identify" << endl;
    IDN_Response idn = scope.Identify();
    cout << "Manufacturer: " << idn.manufacturer << endl;
    cout << "Model: " << idn.model << endl;
    cout << "Serial: " << idn.serial << endl;
    cout << "Version: " << idn.version << endl;
    
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
    PlotWaveforms("testout.tiff", opts, scope);
}


int main(int argc, const char * argv[])
{
    // int glutargc = 0;
    // char * glutargv[] = {};
    // glutInit(&glutargc, glutargv);
    // glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    // glutInitWindowPosition(128,128); 
    // glutInitWindowSize(800, 512); 
    // glutCreateWindow("Scope"); 
    // glutDisplayFunc(Display); 
    
    InitializeMagick(*argv);
    
    PlotOpts opts;
    
    try {
        // devices.push_back(new Device("localhost", "DS1EB134806939"));
        devices.push_back(new Device("192.168.1.38", "DS1EB134806939"));
        devices.back()->Connect();
        Capture(opts);
        // glutMainLoop();
    }
    catch(exception & err)
    {
        cout << "Caught exception: " << err.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}

void GetChannel(int ch, DS1000E & scope, Waveform & wfm)
{
    std::vector<uint8_t> resp;
    scope.WaveData(resp, 0);
    wfm.data.resize(resp.size() - 20);
    wfm.fs = scope.SampleRate(ch);
    cout << "Data points received: " << resp.size() << endl;
    cout << "sample rate: " << wfm.fs << endl;
    for(int j = 10; j < resp.size() - 10; ++j)
        wfm.data[j - 10] = ((125.0 - (float)resp[j])/250.0f)*10.0;
}

void PlotWaveforms(const std::string foutPath, PlotOpts & opts, DS1000E & scope)
{
    // Image image = Image("1024x512", "white");
    Image image = Image("2048x512", "white");
    
    std::list<Magick::Drawable> drawList;
    drawList.push_back(DrawablePushGraphicContext());
    drawList.push_back(DrawableViewbox(0, 0, image.columns(), image.rows()));
    
    drawList.push_back(DrawableFillColor(Color()));
    
    
    Waveform cap[2];
    WaveformView view;
    view.waveform = &cap[0];
    GetChannel(0, scope, cap[0]);
    
    opts.startT = view.StartT();
    opts.endT = view.EndT();
    cout << "opts.startT: " << opts.startT << endl;
    cout << "opts.endT: " << opts.endT << endl;
    
    
    drawList.push_back(DrawableStrokeWidth(1.0));
    if(opts.ch1enab) {
        drawList.push_back(DrawableStrokeColor("#F00"));
        view.smoothing = 1;
        view.offsetV = 0;
        PlotWaveform(image, drawList, opts, view);
        view.smoothing = 4;
        view.offsetV = 1;
        PlotWaveform(image, drawList, opts, view);
        view.smoothing = 16;
        view.offsetV = 2;
        PlotWaveform(image, drawList, opts, view);
        view.smoothing = -1;
        view.offsetV = 3;
        PlotWaveform(image, drawList, opts, view);
    }
    // if(opts.ch1enab) {
    //     drawList.push_back(DrawableStrokeColor("#00F"));
    //     PlotChannel(image, drawList, opts, wfm.channels[1]);
    // }
    
    PlotGrid(image, drawList, opts);
    
    // opts.startT = wfm.npoints/2.0 - 6*50e-6*wfm.fs;
    // opts.endT = wfm.npoints/2.0 - 5*50e-6*wfm.fs;;
    // // opts.startT = wfm.npoints/2.0 - 6*5e-3*wfm.fs;
    // // opts.endT = wfm.npoints/2.0 + 0;
    // // opts.endT = wfm.npoints/2.0 + 6*5e-3*wfm.fs;
    // drawList.push_back(DrawableStrokeColor("#0B0"));
    // PlotChannel(image, drawList, opts, wfm.channels[0]);
    
    drawList.push_back(DrawablePopGraphicContext());
    
    image.draw(drawList);
    image.write(std::string(foutPath));
}

void Display()
{
    
}
