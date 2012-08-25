
//******************************************************************************
// Global config:
//  devices.[(string)sernum].name: (string)name
//  devices.[(string)sernum].address: (string)address
//

// Session config:
//  devices.sernums.[(string)sernum]
//  devices.[(string)sernum].name: (string)name
//  devices.[(string)sernum].address: (string)address
//  devices.[(string)sernum].slaveTo: (string)sernum
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
#include "rigol_ds1k_ui.h"

#include "units.h"

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

struct MainController {
    GtkWidget * appWindow;
    GtkWidget * instrumentsWindow;
    GtkWidget * instrumentsNtbk;
};

//******************************************************************************

MainController * mainCtl = NULL;
ScopeModel * model = NULL;
GtkApplication * gtkapp = NULL;

configmap_t globalConfig;
configmap_t sessionConfig;

//******************************************************************************

void PlotWaveforms(const std::string foutPath, PlotOpts & opts, DS1000E & device);

static void startupApp(GApplication * app, gpointer userData);
static void activateApp(GtkApplication * app, gpointer userData);
bool UpdateCB(GtkWidget * window);

//******************************************************************************
void ExitCleanup()
{
    libusb_exit(NULL);
}

//******************************************************************************
int main(int argc, char * argv[])
{
    globalConfig["connPort"] = "9393";
    globalConfig["discQPort"] = "49393";
    globalConfig["discRPort"] = "49394";
    globalConfig["discAddr_v4"] = "225.0.0.50";
    globalConfig["supportedVID_PID.DS1102E"] = "1AB1:0588";
    
    units::InitUnits();
    InitializeMagick(*argv);
    
    int r = libusb_init(NULL);
    if(r < 0) {
        cerr << "libusb_init() failed" << endl;
        return EXIT_FAILURE;
    }
    atexit(ExitCleanup);
    
    Init_RigolDS1K();
    
    try {
        model = new ScopeModel;
    }
    catch(exception & err)
    {
        cerr << "Caught exception: " << err.what() << endl;
        return EXIT_FAILURE;
    }
    
    gtkapp = gtk_application_new("org.cjameshuff.scopev", G_APPLICATION_FLAGS_NONE);
    mainCtl = new MainController;
    g_signal_connect(gtkapp, "startup", G_CALLBACK(startupApp), mainCtl);
    g_signal_connect(gtkapp, "activate", G_CALLBACK(activateApp), mainCtl);
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
struct ConnectController {
    GtkWidget * selectionCombo;
    GtkWidget * addrEntry;
    GtkWidget * sernumEntry;
    GtkWidget * scopeTypeCombo;
};

void Sig_ConnectDlgResponse(GtkDialog * dlog, gint response, ConnectController * conCtl)
{
    if(response == GTK_RESPONSE_ACCEPT)
    {
        string addr, sernum;
        uint16_t vid = 0xFFFF;
        uint16_t pid = 0xFFFF;
        
        const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(conCtl->selectionCombo));
        if(!selection || strcmp(selection, "address") == 0)
            printf("Manual connection\n");
        else
            printf("From list: %s\n", selection);
        
        if(selection && (strcmp(selection, "address") != 0))
        {
            vector<string> selInfo;
            split(selection, ':', selInfo);
            vid = strtol(selInfo[1].c_str(), NULL, 16) & 0xFFFF;
            pid = strtol(selInfo[2].c_str(), NULL, 16) & 0xFFFF;
            addr = selInfo[0];
            sernum = selInfo[3];
        }
        else
        {
            addr = gtk_entry_get_text(GTK_ENTRY(conCtl->addrEntry));
            sernum = gtk_entry_get_text(GTK_ENTRY(conCtl->sernumEntry));
        }
        printf("Connecting to %s", sernum.c_str());
        if(addr == "" || addr == "USB")
            printf(" on USB\n", addr.c_str());
        else
            printf(" at %s\n", addr.c_str());
        
        model->Connect(addr, vid, pid, sernum);
        DS1k_Controller * ctl = new DS1k_Controller(model, sernum);
        gtk_notebook_append_page(GTK_NOTEBOOK(mainCtl->instrumentsNtbk), ctl->layoutGrid, ctl->tabLbl);
        gtk_widget_show_all(mainCtl->instrumentsWindow);
    }
    else {
        printf("response: %d\n", response);
    }
    delete conCtl;
    gtk_widget_destroy(GTK_WIDGET(dlog));
}

static void Action_connect(GSimpleAction * action, GVariant * parameter, gpointer userData)
{
    ConnectController * conCtl = new ConnectController;
    GtkWidget * window = NULL;//(GtkWidget *)userData;
    GtkWidget * dlog = gtk_dialog_new_with_buttons("Connect", GTK_WINDOW(window),
        (GtkDialogFlags)(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_CONNECT, GTK_RESPONSE_ACCEPT,
        NULL);
    g_signal_connect(dlog, "response", G_CALLBACK(Sig_ConnectDlgResponse), conCtl);
    
    GtkWidget * content = gtk_dialog_get_content_area(GTK_DIALOG(dlog));
    
    conCtl->selectionCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(conCtl->selectionCombo), "address", "Connect by Address and Serial Number");
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(conCtl->selectionCombo), "address");
    
    configmap_t supportedDevs;
    EntriesWithPrefix(globalConfig, "supportedVID_PID.", supportedDevs);
    
    std::set<uint32_t> VIDPIDs;
    configmap_t::iterator di;
    for(di = supportedDevs.begin(); di != supportedDevs.end(); ++di)
    {
        vector<string> ids;
        split(di->second, ':', ids);
        if(ids.size() < 2)
            continue;
        uint32_t vid = strtol(ids[0].c_str(), NULL, 16) & 0xFFFF;
        uint32_t pid = strtol(ids[1].c_str(), NULL, 16) & 0xFFFF;
        VIDPIDs.insert((vid << 16) | pid);
    }
    
    vector<DevIdent> localDevices;
    ListUSB_Devices(VIDPIDs, localDevices);
    vector<DevIdent>::iterator ldi;
    for(ldi = localDevices.begin(); ldi != localDevices.end(); ++ldi)
    {
        char bfr[1024];
        snprintf(bfr, 1024, "USB:%04X:%04X:%s", ldi->vendID, ldi->prodID, ldi->sernum.c_str());
        char bfr2[1024];
        snprintf(bfr2, 1024, "USB:%04X:%04X:%s", ldi->vendID, ldi->prodID, ldi->sernum.c_str());
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(conCtl->selectionCombo), bfr, bfr2);
    }
    
    conCtl->scopeTypeCombo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(conCtl->scopeTypeCombo), "DS1102E", "DS1102E");
    
    // ScopeServerFinder finder;
    // finder.SendQuery(VIDPIDs);
    // finder.PollFor(1.0);
    
    
    GtkWidget * addrLabel = gtk_label_new("Address (leave blank for USB):");
    conCtl->addrEntry = gtk_entry_new();
    GtkWidget * sernumLabel = gtk_label_new("Serial Number:");
    conCtl->sernumEntry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(conCtl->addrEntry), 24);
    gtk_entry_set_max_length(GTK_ENTRY(conCtl->sernumEntry), 24);
    
    gtk_container_add(GTK_CONTAINER(content), conCtl->selectionCombo);
    gtk_container_add(GTK_CONTAINER(content), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL));
    gtk_container_add(GTK_CONTAINER(content), conCtl->scopeTypeCombo);
    gtk_container_add(GTK_CONTAINER(content), addrLabel);
    gtk_container_add(GTK_CONTAINER(content), conCtl->addrEntry);
    gtk_container_add(GTK_CONTAINER(content), sernumLabel);
    gtk_container_add(GTK_CONTAINER(content), conCtl->sernumEntry);
    
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
    GtkWidget * appWindow = gtk_application_window_new(app);
    mainCtl->appWindow = appWindow;
    gtk_window_set_title(GTK_WINDOW(appWindow), "Waveforms");
    gtk_window_set_default_size(GTK_WINDOW(appWindow), 768, 640);
    gtk_container_set_border_width(GTK_CONTAINER(appWindow), 20);
    
    GtkWidget * waveformView = gtk_image_new();
    gtk_image_set_from_file(GTK_IMAGE(waveformView), "data/NewFile4.wfm.png");
    
    GtkWidget * scrollView = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(appWindow), scrollView);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrollView), waveformView);
    
    
    GtkWidget * instrumentsWindow = gtk_application_window_new(app);
    mainCtl->instrumentsWindow = instrumentsWindow;
    gtk_window_set_title(GTK_WINDOW(instrumentsWindow), "Instruments");
    // gtk_window_set_default_size(GTK_WINDOW(instrumentsWindow), 512, 384);
    gtk_window_set_gravity(GTK_WINDOW(instrumentsWindow), GDK_GRAVITY_NORTH_EAST);
    gtk_window_move(GTK_WINDOW(instrumentsWindow), gdk_screen_width() - 512, 0);
    gtk_container_set_border_width(GTK_CONTAINER(instrumentsWindow), 20);
    
    GtkWidget * instrumentsNtbk = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(instrumentsNtbk), true);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(instrumentsNtbk), true);
    mainCtl->instrumentsNtbk = instrumentsNtbk;
    
    // DS1k_Controller * ctl = new DS1k_Controller(model, "123456789B");
    // gtk_notebook_append_page(GTK_NOTEBOOK(instrumentsNtbk), ctl->layoutGrid, ctl->tabLbl);
    
    gtk_container_add(GTK_CONTAINER(instrumentsWindow), instrumentsNtbk);
    
    // gtk_widget_show_all(instrumentsWindow);// Not shown until first connection is done
    gtk_widget_show_all(appWindow);
    
    g_timeout_add(33, (GSourceFunc)UpdateCB, NULL);
} // activateApp()

//******************************************************************************

bool UpdateCB(GtkWidget * window)
{
    model->Update();
    return true;
}
//******************************************************************************

/*void Display()
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
}*/

