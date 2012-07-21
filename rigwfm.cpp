// g++  `GraphicsMagick++-config --cxxflags --cppflags  --ldflags --libs` rigwfm.cpp -o rigwfm
// for i in *.wfm; do ./rigwfm ${i} ${i}.png; done

#include <iostream>
#include <string>
#include <list>
#include <map>
#include <vector>

#include <cstdlib>
#include <cmath>

#include <Magick++.h>

#include "rigolwfm.h"

using namespace std;
using namespace rigwfm;
using namespace Magick;


// Plot waveform as DrawablePolyline
// smoothing: n consecutive points are averaged together to do a simple low pass filter
void PlotChannel(Image & image, std::list<Magick::Drawable> & drawList, int smoothing, const RigolData & channel)
{
    double width = image.columns(), height = image.rows();
    std::list<Coordinate> vertices;
    double yscl = height/8.0/channel.scale;
    double yoff = channel.offset;
    size_t npts = channel.data.size();
    for(int j = 4; (j + smoothing) < npts; j += smoothing) {
        double y = 0;
        for(int k = 0; k < smoothing; ++k)
            y += channel.data[j + k];
        y /= smoothing;
        vertices.push_back(Coordinate((double)j/npts*width, height - (y - yoff)*yscl));
    }
    drawList.push_back(DrawablePolyline(vertices));
}

void PlotWaveform(const std::string foutPath, const RigolWaveform & wfm);

int main(int argc, const char * argv[])
{
    if(argc < 3)
    {
        cerr << "Usage: rigwfm WAVEFORM_FILE IMAGE_FILE";
        return EXIT_FAILURE;
    }
    
    InitializeMagick(*argv);
    
    try {
        RigolWaveform waveform(argv[1]);
        cout << waveform << endl;
        PlotWaveform(argv[2], waveform);
    }
    catch(exception & err)
    {
        cout << "Caught exception: " << err.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}



void PlotWaveform(const std::string foutPath, const RigolWaveform & wfm)
{
    int smoothing = 4;
    
    RigolData tmpwfm;
    tmpwfm.scale = wfm.channels[0].scale;
    tmpwfm.attenuation = wfm.channels[0].attenuation;
    tmpwfm.offset = wfm.channels[0].offset;
    tmpwfm.data.resize(wfm.npoints);
    
    Image image = Image("1024x768", "white");
    
    std::list<Magick::Drawable> drawList;
    drawList.push_back(DrawablePushGraphicContext());
    drawList.push_back(DrawableViewbox(0, 0, image.columns(), image.rows()));
    
    drawList.push_back(DrawableFillColor(Color()));
    drawList.push_back(DrawableStrokeWidth(1.0));
    if(!wfm.channels[0].data.empty()) {
        drawList.push_back(DrawableStrokeColor("#F00"));
        PlotChannel(image, drawList, smoothing, wfm.channels[0]);
    }
    if(!wfm.channels[1].data.empty()) {
        drawList.push_back(DrawableStrokeColor("#00F"));
        PlotChannel(image, drawList, smoothing, wfm.channels[1]);
    }
    
    double trigPos = wfm.tdelay*wfm.fs*image.columns()/wfm.npoints;
    cout << "trigPos: " << trigPos << endl;
    
    // Trigger delay, etc are referenced from the midpoint of the captured waveform
    double midT = wfm.npoints/2.0/wfm.fs;
    double vrule;
    for(int j = 1; j < 6; ++j) {
        vrule = wfm.fs*(midT - wfm.tscale*j)*image.columns()/wfm.npoints;
        drawList.push_back(DrawableStrokeColor("#888"));
        drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
        vrule = wfm.fs*(midT + wfm.tscale*j)*image.columns()/wfm.npoints;
        drawList.push_back(DrawableStrokeColor("#888"));
        drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
    }
    
    drawList.push_back(DrawableStrokeWidth(2.0));
    drawList.push_back(DrawableStrokeColor("#000"));
    vrule = wfm.fs*(midT - wfm.tscale*6)*image.columns()/wfm.npoints;
    drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
    vrule = wfm.fs*midT*image.columns()/wfm.npoints;
    drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
    vrule = wfm.fs*(midT + wfm.tscale*6)*image.columns()/wfm.npoints;
    drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
    
    // Trigger position
    drawList.push_back(DrawableStrokeWidth(1.0));
    drawList.push_back(DrawableStrokeColor("#990"));
    vrule = wfm.fs*(midT - wfm.tdelay)*image.columns()/wfm.npoints;
    drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
    
/*        double switchT = 2850;
    double c1C = 100e-6;
    double c2C = 470e-6;
    double c1v = 0;
    double c2v = 6.2;
    double r1r = 0.1;
    double L = 5e-6;
    double current = 0;
    double tstep = 1.0/waveform.fs;
    for(int j = 0; j < waveform.npoints; ++j)
    {
        if(j < switchT)
            tmpwfm.data[j] = 0.0;
        else {
            tmpwfm.data[j] = 6.2 - c2v;
            double vdrop = r1r*current;
            current += (c1v - c2v - vdrop)/L*tstep;
            double c = current*tstep;
            c1v -= c/c1C;
            c2v += c/c2C;
        }
    }*/
    // for(int j = 0; j < waveform.npoints; ++j)
    // {
    //     double v = 5.14*exp(-(j - switchT)/waveform.fs/(0.4*1.0/(1.0/470e-6 + 1.0/100e-6))) + 1.06;
    //     if(j < switchT)
    //         tmpwfm.data[j] = 6.2;
    //     else
    //         // tmpwfm.data[j] = waveform.channels[0].data[j] - v + 3;
    //         tmpwfm.data[j] = v;
    // }
    // drawList.push_back(DrawableStrokeColor("#990"));
    // PlotChannel(image, drawList, tmpwfm);
    
    drawList.push_back(DrawablePopGraphicContext());
    
    image.draw(drawList);
    image.write(std::string(foutPath));
}
