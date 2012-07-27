

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

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <Magick++.h>

#include "rigoltmc.h"

using namespace std;

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

int main(int argc, const char * argv[])
{
    int glutargc = 0;
    char * glutargv[] = {};
    glutInit(&glutargc, glutargv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
    glutInitWindowPosition(128,128); 
    glutInitWindowSize(800, 512); 
    glutCreateWindow("Scope"); 
    glutDisplayFunc(Display); 
    
    try {
        cout << "Attempting to connect" << endl;
        
        Socket * sock = NULL;
        while(!sock) {
            usleep(10000);
            // conn = client.Connect("127.0.0.1", "9393");
            sock = ClientConnect("localhost", "9393");
        }
        cout << "Connected to server: " << sock->other << endl;
        
        
        DS1000E scope(new TMC_RemoteDevice(0x1AB1, 0x0588, "", sock->sockfd));
        
        cout << "Sending identify" << endl;
        IDN_Response idn = scope.Identify();
        cout << "Manufacturer: " << idn.manufacturer << endl;
        cout << "Model: " << idn.model << endl;
        cout << "Serial: " << idn.serial << endl;
        cout << "Version: " << idn.version << endl;
        
        scope.Run();
        sleep(3);
        scope.Force();
        sleep(3);
        scope.Stop();
        cout << "Channel 1 scale: " << scope.ChanScale(0) << endl;
        cout << "Channel 1 offset: " << scope.ChanOffset(0) << endl;
        cout << "Time scale: " << scope.TimeScale() << endl;
        cout << "Time offset: " << scope.TimeOffset() << endl;
        
        cout << ":WAV:POIN:MODE? -> " << scope.Query(":WAV:POIN:MODE?") << endl;
        scope.RawMode();
        cout << ":WAV:POIN:MODE? -> " << scope.Query(":WAV:POIN:MODE?") << endl;
        std::vector<uint8_t> resp;
        scope.WaveData(resp, 0);
        cout << "Data points received: " << resp.size() << endl;
        for(int j = 0; j < std::min((int)resp.size(), 20); ++j)
            cout << ((125.0 - (float)resp[j])/250.0f)*10.0 << endl;
        
        
        glutMainLoop();
    }
    catch(exception & err)
    {
        cout << "Caught exception: " << err.what() << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}


void Display()
{
    
}
