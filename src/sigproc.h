// Various signal processing utilities

#ifndef SIGPROC_H
#define SIGPROC_H


// http://en.wikipedia.org/wiki/Lanczos_resampling

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



#endif // SIGPROC_H
