
#include "plotting.h"

#include <iostream>

using namespace std;
using namespace Magick;

void PlotGrid(Image & image, std::list<Magick::Drawable> & drawList, PlotOpts & opts)
{
    // Vertical rules
    // Trigger delay, etc are referenced from the midpoint of the captured waveform
    double midT = (opts.endT - opts.startT)/2.0;
    double vrule;
    drawList.push_back(DrawableStrokeColor("#444"));
    for(int j = 1; j < 6; ++j) {
        vrule = (midT - opts.divT*j)*image.columns()/(opts.endT - opts.startT);
        drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
        vrule = (midT + opts.divT*j)*image.columns()/(opts.endT - opts.startT);
        drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
    }
    drawList.push_back(DrawableStrokeWidth(0.5));
    drawList.push_back(DrawableStrokeColor("#AAA"));
    for(int j = 1; j < 7; ++j) {
        for(int k = 1; k < 5; ++k) {
            vrule = (midT - opts.divT*(j - k/5.0))*image.columns()/(opts.endT - opts.startT);
            drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
            vrule = (midT + opts.divT*(j - k/5.0))*image.columns()/(opts.endT - opts.startT);
            drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
        }
    }
    
    drawList.push_back(DrawableStrokeWidth(2.0));
    drawList.push_back(DrawableStrokeColor("#000"));
    vrule = (midT - opts.divT*6)*image.columns()/(opts.endT - opts.startT);
    drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
    vrule = midT*image.columns()/(opts.endT - opts.startT);
    drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
    vrule = (midT + opts.divT*6)*image.columns()/(opts.endT - opts.startT);
    drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
    
    // Trigger position
    // drawList.push_back(DrawableStrokeWidth(1.0));
    // drawList.push_back(DrawableStrokeColor("#990"));
    // vrule = (midT - wfm.tdelay)*image.columns()/(opts.endT - opts.startT);
    // drawList.push_back(DrawableLine(vrule, 0, vrule, image.rows()));
    
    // Horizontal rules
    double hrule;
    drawList.push_back(DrawableStrokeColor("#444"));
    for(int j = 1; j < 4; ++j) {
        hrule = image.rows() - (4 - j)*(image.rows()/8.0);
        drawList.push_back(DrawableLine(0, hrule, image.columns(), hrule));
        hrule = image.rows() - (4 + j)*(image.rows()/8.0);
        drawList.push_back(DrawableLine(0, hrule, image.columns(), hrule));
    }
    drawList.push_back(DrawableStrokeWidth(0.5));
    drawList.push_back(DrawableStrokeColor("#AAA"));
    for(int j = 1; j < 5; ++j) {
        for(int k = 1; k < 5; ++k) {
            hrule = image.rows() - (4 + k/5.0 - j)*(image.rows()/8.0);
            drawList.push_back(DrawableLine(0, hrule, image.columns(), hrule));
            hrule = image.rows() - (4 - k/5.0 + j)*(image.rows()/8.0);
            drawList.push_back(DrawableLine(0, hrule, image.columns(), hrule));
        }
    }
    
    drawList.push_back(DrawableStrokeWidth(2.0));
    drawList.push_back(DrawableStrokeColor("#000"));
    hrule = image.rows() - (4)*(image.rows()/8.0);
    drawList.push_back(DrawableLine(0, hrule, image.columns(), hrule));
    hrule = image.rows() - (0)*(image.rows()/8.0);
    drawList.push_back(DrawableLine(0, hrule, image.columns(), hrule));
    hrule = image.rows() - (8)*(image.rows()/8.0);
    drawList.push_back(DrawableLine(0, hrule, image.columns(), hrule));
}


void PlotWaveform(Image & image, std::list<Magick::Drawable> & drawList, PlotOpts & opts, const WaveformView & view)
{
    double width = image.columns(), height = image.rows();
    
    std::list<Coordinate> vertices;
    double yscl = height/8.0/opts.divV;
    double yoff = 0;
    double wfmStartT = view.StartT();
    double wfmEndT = view.EndT();
    double viewStartT = fmax(opts.startT, wfmStartT);
    double viewEndT = fmin(opts.endT, wfmEndT);
    
    if(viewStartT > viewEndT)
        return;// plot not visible
    
    int startCol = (viewStartT - opts.startT)/(opts.endT - opts.startT)*width;
    int endCol = (viewEndT - opts.startT)/(opts.endT - opts.startT)*width;
    
    size_t nsamps = (viewEndT - viewStartT)*view.waveform->fs;
    if(width >= nsamps/2 || view.smoothing == -1)
    {
        // plot two sinc-interpolated points per column
        // cout << "dx: " << dx << endl;
        // cout << "startT: " << startT << endl;
        for(int j = startCol; j <= endCol; ++j) {
            double t = (double)(j - startCol)/(endCol - startCol)*(viewEndT - viewStartT);
            double y = SincReconstruct(view.waveform->data, t*view.waveform->fs, 64) + view.offsetV;
            // double y = view.waveform->data[t*view.waveform->fs];
            cout << t*view.waveform->fs << ", " << y << endl;
            y = (height/2 - (y + yoff)*yscl);
            vertices.push_back(Coordinate(j, y));
        }
    }
    else
    {
        int startSamp = (viewStartT - wfmStartT)/(wfmEndT - wfmStartT)*view.waveform->data.size()*view.waveform->fs;
        cout << startSamp << ", " << nsamps << endl;
        // int endSamp = viewEndT/(wfmEndT - wfmStartT)*view.waveform->data.size()*view.waveform->fs;
        for(int j = 0, n = nsamps; j < n; ++j)
        {
            double y = RollingAverage(view.waveform->data, startSamp + j, view.smoothing) + view.offsetV;
            // double y = view.waveform->data[startSamp + j] + 1;
            // cout << startSamp << ", " << (startSamp + j) << ", " << y << endl;
            vertices.push_back(Coordinate(((double)j/nsamps)*(endCol - startCol) + startCol, height/2 - (y + yoff)*yscl));
        }
    }
    drawList.push_back(DrawablePolyline(vertices));
}


