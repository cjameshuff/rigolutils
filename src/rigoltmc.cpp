// g++ freetmc.cpp rigoltmc.cpp -L/usr/local/lib -lusb-1.0 -o rigoltmc

#include "freetmc.h"
#include "rigoltmc.h"

#include <iostream>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

using namespace std;


int main(int argc, const char * argv[])
{
    try {
        DS1000E scope(new TMC_LocalDevice(0x1AB1, 0x0588));
        
        IDN_Response idn = scope.Identify();
        cout << "*IDN? manufacturer: " << idn.manufacturer << endl;
        cout << "*IDN? model: " << idn.model << endl;
        cout << "*IDN? serial: " << idn.serial << endl;
        cout << "*IDN? version: " << idn.version << endl;
        
        std::vector<uint8_t> resp;
        
        scope.Run();
        sleep(1);
        scope.Force();
        scope.Stop();
        cout << "Channel 1 scale: " << scope.ChanScale(0) << endl;
        cout << "Channel 1 offset: " << scope.ChanOffset(0) << endl;
        cout << "Time scale: " << scope.TimeScale() << endl;
        cout << "Time offset: " << scope.TimeOffset() << endl;
        
        cout << ":WAV:POIN:MODE? -> " << scope.Query(":WAV:POIN:MODE?") << endl;
        scope.RawMode();
        cout << ":WAV:POIN:MODE? -> " << scope.Query(":WAV:POIN:MODE?") << endl;
        scope.WaveData(resp, 0);
        cout << "Data points received: " << resp.size() << endl;
        for(int j = 0; j < std::min((int)resp.size(), 20); ++j)
            cout << ((125.0 - (float)resp[j])/250.0f)*10.0 << endl;
    }
    catch(exception & err)
    {
        cout << "Caught exception: " << err.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}


