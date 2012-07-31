
#ifndef PLOTTING_H
#define PLOTTING_H

#include <iostream>
#include <string>
#include <exception>
#include <vector>
#include <list>
#include <map>

#include <cstdlib>
#include <cmath>
#include <cfloat>
#include <algorithm>

#include "units.h"
#include <Magick++.h>


// Do sinc reconstructionof original waveform using halfpts*2 samples centered on t.
// Time is in units of sample period.
// Note: reconstructing a waveform at an integer multiple of the original sampling
// frequency could be optimized to reuse sinc weights.
inline float SincReconstruct(const std::vector<float> & data, float t, int halfpts)
{
    if((t - (int)t) == 0)
    {
        return data[std::min((int)data.size() - 1, std::max(0, (int)t))];
    }
    else
    {
        float result = 0.0f;
        int start = std::max(0, (int)ceilf(t) - halfpts);
        int end = std::min((int)data.size() - 1, (int)floorf(t) + halfpts);
        for(int j = start; j <= end; ++j)
            result += data[j]*sinf(M_PI*(t - j))/(M_PI*(t - j));
        return result;
    }
}
inline float RollingAverage(const std::vector<float> & data, float t, int halfpts)
{
    float result = 0.0f;
    int start = std::max(0, (int)ceilf(t) - halfpts);
    int end = std::min((int)data.size() - 1, (int)floorf(t) + halfpts);
    for(int j = start; j <= end; ++j)
        result += data[j];
    return result/(end - start + 1);
}



struct Waveform {
    double fs;// sample rate, Hz
    double offsetT;// Time offset, seconds
    std::vector<float> data;// waveform data, volts
    
    Waveform():
        fs(1), offsetT(0)
    {}
};

struct WaveformView {
    int smoothing;
    double scaleV;// Volts/div
    double offsetV;// Volts
    double scaleT;
    double offsetT;// Time offset, seconds
    
    Waveform * waveform;
    
    WaveformView():
        smoothing(1),
        scaleV(1),
        offsetV(0),
        scaleT(1),
        offsetT(0),
        waveform(NULL)
    {}
    
    // Compute start and end time, referenced to the delayed trigger point
    // double StartT() const {return (-waveform->data.size()/2.0/waveform->fs + waveform->offsetT + offsetT)*scaleT;}
    // double EndT() const {return (waveform->data.size()/2.0/waveform->fs + waveform->offsetT + offsetT)*scaleT;}
    double SampT(int i) const {return ((double)(i - waveform->data.size())/2.0/waveform->fs + waveform->offsetT + offsetT);}
    double StartT() const {return (-(double)waveform->data.size()/2.0/waveform->fs + waveform->offsetT + offsetT);}
    double EndT() const {return ((double)waveform->data.size()/2.0/waveform->fs + waveform->offsetT + offsetT);}
    
//    double SampIdx(double t) const {return ((double)(i - waveform->data.size())/2.0/waveform->fs + waveform->offsetT + offsetT);}
};

struct PlotOpts {
    double startT, endT;// start and end time, in seconds
    double divV, divT;
    
    bool ch1enab;
    bool ch2enab;
    
    std::vector<WaveformView *> waveforms;
    
    
    PlotOpts():
        startT(DBL_MIN),
        endT(DBL_MAX),
        divV(1.0),
        divT(1.0),
        ch1enab(true),
        ch2enab(true)
    {}
};


// Plot waveform as DrawablePolyline
// smoothing: n consecutive points are averaged together to do a simple low pass filter
void PlotWaveform(Magick::Image & image, std::list<Magick::Drawable> & drawList, PlotOpts & opts, const WaveformView & view);
void PlotGrid(Magick::Image & image, std::list<Magick::Drawable> & drawList, PlotOpts & opts);


#endif // PLOTTING_H
