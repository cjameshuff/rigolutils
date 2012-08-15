
//******************************************************************************
// Global config:
//  numKnownDevices: (int)count
//  device[(int)id]: (string)sernum
//
//  [(string)sernum].deviceName: (string)name
//  [(string)sernum].deviceAddress: (string)address
//

// Session config:
//  numDevices: (int)count
//  device[(int)id]: (string)sernum
//  
//  [(string)sernum].name: (string)name
//  [(string)sernum].address: (string)address
//  [(string)sernum].slaveTo: (string)sernum
//
//  numPlots: (int)count
//  plot[(int)id].name: (string)name
//  plot[(int)id].device: (string)sernum
//  plot[(int)id].channel: (int)channelNum
//  plot[(int)id].visible: true|false
//  plot[(int)id].color: #RRGGBB
// 
//******************************************************************************



#include "scopev_model.h"
#include "rigol_ds1k.h"
#include "selector.h"
#include "cfgmap.h"

#include <iostream>
#include <string>
#include <map>

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <gtk/gtk.h>
#include "gtkhelpers.h"

#include <Magick++.h>

using namespace std;
using namespace Magick;

ScopeModel * model = NULL;
GtkApplication * gtkapp = NULL;

std::map<std::string, std::string> globalConfig;
std::map<std::string, std::string> sessionConfig;

//******************************************************************************

void PlotWaveforms(const std::string foutPath, PlotOpts & opts, DS1000E & device);

static void startupApp(GApplication * app, gpointer userData);
static void activateApp(GtkApplication * app, gpointer userData);
bool UpdateCB(ScopeModel * model);

//******************************************************************************
class ScopeServerFinder: public ServerFinder {
    vector<string> servers;
  public:
    ScopeServerFinder(uint8_t * buffer, size_t msgLen, const std::string & a, uint16_t qp, uint16_t rp):
        ServerFinder(buffer, msgLen, a, qp, rp)
    {}
    
    vector<string> & GetServers() {return servers;}
    
    virtual bool HandleResponse(uint8_t * buffer, size_t msgLen, sockaddr_in & srcAddr, socklen_t srcAddrLen)
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
        char s[INET6_ADDRSTRLEN];
        struct sockaddr * sa = (struct sockaddr *)&srcAddr;
        if(sa->sa_family == AF_INET)
            inet_ntop(sa->sa_family, &(((struct sockaddr_in *)sa)->sin_addr), s, INET6_ADDRSTRLEN);
        else
            inet_ntop(sa->sa_family, &(((struct sockaddr_in6 *)sa)->sin6_addr), s, INET6_ADDRSTRLEN);
        cerr << "Found server \"" << response << "\" at " << s << endl;
        servers.push_back(response);
        return true;
    }
};

int main(int argc, char * argv[])
{
    globalConfig["connPort"] = "9393";
    globalConfig["discPort"] = "49393";
    globalConfig["discAddr_v4"] = "225.0.0.50";
    
    
    std::vector<uint8_t> query(PKT_HEADER_SIZE);
    PKT_SetCMD(query, CMD_PING);
    PKT_SetSeq(query, 0);
    PKT_SetPayloadSize(query, 0);
    ScopeServerFinder finder(&query[0], query.size(), DISC_BCAST_ADDR, DISC_QUERY_PORT, DISC_RESP_PORT);
    finder.PollFor(1.0);
    
    return EXIT_SUCCESS;
    
    
    Init_RigolDS1K();
    
    try {
        // devices.push_back(new Device("localhost", "DS1EB134806939"));
        // devices.push_back(new Device("192.168.1.21"));
        // devices.back()->Connect();
        // Capture(opts);
        model = new ScopeModel;
    }
    catch(exception & err)
    {
        cout << "Caught exception: " << err.what() << endl;
        return EXIT_FAILURE;
    }
    
    InitializeMagick(*argv);
    
    gtkapp = gtk_application_new("org.cjameshuff.scopev", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(gtkapp, "startup", G_CALLBACK(startupApp), NULL);
    g_signal_connect(gtkapp, "activate", G_CALLBACK(activateApp), NULL);
    g_set_application_name("ScopeView");
    
    int status = g_application_run(G_APPLICATION(gtkapp), argc, argv);
    g_object_unref(gtkapp);
    return status;
}


//******************************************************************************

void LoadCfgDlgResponse(GtkDialog * dlog, gint response, gpointer userData)
{
}

static void Action_load_cfg(GSimpleAction * action, GVariant * parameter, gpointer userData)
{
}

//******************************************************************************

void SaveCfgDlgResponse(GtkDialog * dlog, gint response, gpointer userData)
{
}

static void Action_save_cfg(GSimpleAction * action, GVariant * parameter, gpointer userData)
{
}

//******************************************************************************

void ConnectDlgResponse(GtkDialog * dlog, gint response, gpointer userData)
{
    WidgetDict * wdict = (WidgetDict*)userData;
    if(response == GTK_RESPONSE_ACCEPT)
    {
        GtkWidget * addrEntry = wdict->Get("addrEntry");
        GtkWidget * sernumEntry = wdict->Get("sernumEntry");
        string addr = gtk_entry_get_text(GTK_ENTRY(addrEntry));
        string sernum = gtk_entry_get_text(GTK_ENTRY(sernumEntry));
        
        printf("Connecting to %s ", sernum.c_str());
        printf("at %s\n", addr.c_str());
    }
    else {
        printf("response: %d\n", response);
    }
    delete wdict;
    gtk_widget_destroy(GTK_WIDGET(dlog));
}

static void Action_connect(GSimpleAction * action, GVariant * parameter, gpointer userData)
{
    WidgetDict * wdict = new WidgetDict;
    GtkWidget * window = NULL;//(GtkWidget *)userData;
    GtkWidget * dlog = gtk_dialog_new_with_buttons("Connect", GTK_WINDOW(window),
        (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_CONNECT, GTK_RESPONSE_ACCEPT,
        NULL);
    g_signal_connect(dlog, "response", G_CALLBACK(ConnectDlgResponse), wdict);
    
    GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dlog));
    GtkWidget * addrLabel = gtk_label_new("Address (leave blank for USB):");
    GtkWidget * addrEntry = gtk_entry_new();
    GtkWidget * sernumLabel = gtk_label_new("Serial Number:");
    GtkWidget * sernumEntry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(addrEntry), 24);
    gtk_entry_set_max_length(GTK_ENTRY(sernumEntry), 24);
    
    wdict->Add("addrEntry", addrEntry);
    wdict->Add("sernumEntry", sernumEntry);
    
    gtk_container_add(GTK_CONTAINER(content), addrLabel);
    gtk_container_add(GTK_CONTAINER(content), addrEntry);
    gtk_container_add(GTK_CONTAINER(content), sernumLabel);
    gtk_container_add(GTK_CONTAINER(content), sernumEntry);
    
    gtk_widget_show_all(dlog);
}

//******************************************************************************

static void Action_about(GSimpleAction *action, GVariant *parameter, gpointer userData)
{
    GtkWidget * window = NULL;//(GtkWidget *)userData;
    gtk_show_about_dialog(GTK_WINDOW(window),
        "program-name", "ScopeView",
        "version", "0.1a",
        "copyright", "© 2012 Christopher James Huff",
        "license-type", GTK_LICENSE_MIT_X11,
        "website", "https://github.com/cjameshuff/rigolutils",
        "title", "About ScopeView",
        "comments", "Oscilloscope viewing, control, and data acquisition for Rigol DS1000 series oscilloscopes.",
        NULL);
}

//******************************************************************************

static void Action_quit(GSimpleAction * action, GVariant * parameter, gpointer userData)
{
    GApplication * app = (GApplication*)userData;
    g_application_quit(app);
}

//******************************************************************************

static GActionEntry appMenuActions[] = {
    {"connect", Action_connect, NULL, NULL, NULL},
    {"save-cfg", Action_save_cfg, NULL, NULL, NULL},
    {"load-cfg", Action_load_cfg, NULL, NULL, NULL},
    // {"save-image", Action_save_image, NULL, NULL, NULL},
    // {"load-image", Action_load_image, NULL, NULL, NULL},
    // {"save-raw-wfm", Action_save_raw_wfm, NULL, NULL, NULL},
    // {"load-raw-wfm", Action_load_raw_wfm, NULL, NULL, NULL},
    // {"save-rigol-wfm", Action_save_rigol_wfm, NULL, NULL, NULL},
    // {"load-rigol-wfm", Action_load_rigol_wfm, NULL, NULL, NULL},
    // {"save-sess", Action_save_sess, NULL, NULL, NULL},
    // {"load-sess", Action_load_sess, NULL, NULL, NULL},
    {"about", Action_about, NULL, NULL, NULL},
    {"quit", Action_quit, NULL, NULL, NULL},
};


static void startupApp(GApplication * app, gpointer userData)
{
    g_action_map_add_action_entries(G_ACTION_MAP(app), appMenuActions, G_N_ELEMENTS(appMenuActions), app);
    
    MenuBuilder mb;
    mb.Item("_About", "app.about");
    {MB_Section sect(mb, "");
        mb.Item("_Quit", "app.quit", "<Primary>q");
    }
    GMenu * appMenu = mb.Close();
    
    mb.Reset();
    {MB_Submenu subm(mb, "File");
        mb.Item("_Connect", "app.connect", "<Primary>k");
        {MB_Section sect(mb, "");
            // mb.Item("_Load Image", "app.load-image");
            mb.Item("_Load Settings", "app.load-cfg");
            mb.Item("_Load Rigol Waveform", "app.load-rigol-wfm");
            mb.Item("_Load Raw Waveform", "app.load-raw-wfm");
            // mb.Item("_Load Session", "app.load-sess");
        }
        {MB_Section sect(mb, "");
            mb.Item("_Save Image", "app.save-image");
            mb.Item("_Save Settings", "app.save-cfg");
            mb.Item("_Save Waveform", "app.save-wfm");
            // mb.Item("_Save Session", "app.save-sess");
        }
    }
    {MB_Submenu subm(mb, "_Edit");
        {MB_Section sect(mb, "");
            mb.Item("_Cut", "app.cut", "<Primary>x");
            mb.Item("_Copy", "app.copy", "<Primary>c");
            mb.Item("_Paste", "app.paste", "<Primary>v");
        }
    }
    {MB_Submenu subm(mb, "_Scope");
        mb.Item("_Connect", "app.paste");
        {MB_Section sect(mb, "");
            mb.Item("_Add Waveform", "app.cut");
        }
    }
    GMenu * mainMenu = mb.Close();
    
    gtk_application_set_app_menu(GTK_APPLICATION(app), G_MENU_MODEL(appMenu));
    gtk_application_set_menubar(GTK_APPLICATION(app), G_MENU_MODEL(mainMenu));
    g_object_unref(appMenu);
    g_object_unref(mainMenu);
} // startupApp()
//******************************************************************************

static void activateApp(GtkApplication * app, gpointer userData)
{
    GtkWidget * window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "GTK Scratch");
    gtk_window_set_default_size(GTK_WINDOW(window), 768, 640);
    gtk_container_set_border_width(GTK_CONTAINER(window), 20);
    
    GtkWidget * waveformView = gtk_image_new();
    gtk_image_set_from_file(GTK_IMAGE(waveformView), "NewFile4.wfm.png");
    
    GtkWidget * scrollView = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(window), scrollView);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollView), waveformView);
    
    gtk_widget_show_all(window);
    
    // g_timeout_add(33, (GSourceFunc)UpdateCB, window);
} // activateApp()
//******************************************************************************

bool UpdateCB(ScopeModel * model)
{
    model->Update();
}

void Display()
{
    Image image = Image(Geometry(1024, 512), "white");
    int width = image.columns(), height = image.rows();
    
    uint8_t * imgbuffer = new uint8_t[width*height*4];
    const PixelPacket * pixpkt = image.getConstPixels(0, 0, width, height);
    for(size_t p = 0, j = 0, n = width*height; p < n; ++p, j += 4)
    {
        imgbuffer[j] = pixpkt[p].red >> 8;
        imgbuffer[j+1] = pixpkt[p].green >> 8;
        imgbuffer[j+2] = pixpkt[p].blue >> 8;
        imgbuffer[j+3] = pixpkt[p].opacity >> 8;
    }
    // int x = 0, y = 0;
    // PixelPacket & pixel = pixpkt[y*width + x];
    
    delete[] imgbuffer;
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

