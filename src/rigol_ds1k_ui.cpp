
#include "rigol_ds1k_ui.h"

// TODO: Users of SI_SpinButton should divide by units to get properly scaled results.
// Disable "Force" when not in run mode
// In single capture mode, uncheck "Run" checkbox on when triggered.


using namespace std;

//******************************************************************************
#define READBACK_DELAY()  usleep(20000)

//==============================================================================
// Channel controls
//==============================================================================

void Sig_ChannelProbeCoupSelChanged(GtkComboBox * widget, ScopeChanCtlPane * controller) {
    if(controller->controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    controller->controller->device->tmcDevice->CmdF(":CHAN%d:COUP %s", controller->channel + 1, selection);
}

void Sig_ChannelProbeSelChanged(GtkComboBox * widget, ScopeChanCtlPane * controller) {
    if(controller->controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    controller->controller->device->tmcDevice->CmdF(":CHAN%d:PROB %s", controller->channel + 1, selection);
}

void Sig_ChannelEnabledToggled(GtkToggleButton * btn, ScopeChanCtlPane * controller) {
    if(controller->controller->ControlsMuted())
        return;
    
    if(gtk_toggle_button_get_active(btn))
        controller->controller->device->tmcDevice->CmdF(":CHAN%d:DISP ON", controller->channel + 1);
    else
        controller->controller->device->tmcDevice->CmdF(":CHAN%d:DISP OFF", controller->channel + 1);
}

void Sig_ChannelFilterToggled(GtkToggleButton * btn, ScopeChanCtlPane * controller) {
    if(controller->controller->ControlsMuted())
        return;
    
    if(gtk_toggle_button_get_active(btn))
        controller->controller->device->tmcDevice->CmdF(":CHAN%d:FILT ON", controller->channel + 1);
    else
        controller->controller->device->tmcDevice->CmdF(":CHAN%d:FILT OFF", controller->channel + 1);
}

void Sig_ChannelBWLimitToggled(GtkToggleButton * btn, ScopeChanCtlPane * controller) {
    if(controller->controller->ControlsMuted())
        return;
    
    if(gtk_toggle_button_get_active(btn))
        controller->controller->device->tmcDevice->CmdF(":CHAN%d:BWL ON", controller->channel + 1);
    else
        controller->controller->device->tmcDevice->CmdF(":CHAN%d:BWL OFF", controller->channel + 1);
}


void Sig_ChannelOffsetChanged(GtkSpinButton * spinBtn, ScopeChanCtlPane * controller) {
    if(controller->controller->ControlsMuted())
        return;
    
    GtkAdjustment * adjustment = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spinBtn));
    gdouble val = gtk_adjustment_get_value(adjustment);
    controller->controller->device->tmcDevice->CmdF(":CHAN%d:OFFS %f", controller->channel + 1, val);
    
    // Readback actual level set
    READBACK_DELAY();
    controller->controller->MuteControls();
    string newVal = controller->controller->device->tmcDevice->QueryF(":CHAN%d:OFFS?", controller->channel + 1);
    controller->offsetSISB->SetValue(strtod(newVal.c_str(), NULL));
    controller->controller->UnmuteControls();
}

void Sig_ChannelScaleChanged(GtkSpinButton * spinBtn, ScopeChanCtlPane * controller) {
    printf("-> Sig_ChannelScaleChanged\n");
    if(controller->controller->ControlsMuted())
        return;
    
    GtkAdjustment * adjustment = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spinBtn));
    gdouble val = gtk_adjustment_get_value(adjustment);
    controller->controller->device->tmcDevice->CmdF(":CHAN%d:SCAL %f", controller->channel + 1, val);
    
    // Readback actual level set
    READBACK_DELAY();
    controller->controller->MuteControls();
    string newVal = controller->controller->device->tmcDevice->QueryF(":CHAN%d:SCAL?", controller->channel + 1);
    controller->scaleSISB->SetValue(strtod(newVal.c_str(), NULL));
    controller->controller->UnmuteControls();
}


ScopeChanCtlPane::ScopeChanCtlPane(DS1k_Controller * scp, const char * name, int chan):
    controller(scp),
    channel(chan)
{
    frame = gtk_frame_new(name);
    GtkWidget * layoutGrid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(frame), layoutGrid);
    
    enabledChk = gtk_check_button_new_with_label("Enabled");
    g_signal_connect(enabledChk, "toggled", G_CALLBACK(Sig_ChannelEnabledToggled), this);
    filterChk = gtk_check_button_new_with_label("Filter");
    g_signal_connect(filterChk, "toggled", G_CALLBACK(Sig_ChannelFilterToggled), this);
    bwLimitChk = gtk_check_button_new_with_label("BW Limit");
    g_signal_connect(bwLimitChk, "toggled", G_CALLBACK(Sig_ChannelBWLimitToggled), this);
    
    coupSel = ComboBoxText("DC",
        "DC", "DC",
        "AC", "AC",
        "GND", "GND",
        NULL
    );
    g_signal_connect(coupSel, "changed", G_CALLBACK(Sig_ChannelProbeCoupSelChanged), this);
    
    probeSel = ComboBoxText("10",
        "1", "1x",
        "5", "5x",
        "10", "10x",
        "50", "50x",
        "100", "100x",
        "500", "500x",
        "1000", "1000x",
        NULL
    );
    g_signal_connect(probeSel, "changed", G_CALLBACK(Sig_ChannelProbeSelChanged), this);
    
    offsetSISB = new SI_SpinButton(gtk_adjustment_new(0.0, -500, 500, 0.01, 0.1, 0.0), "V");
    offsetSISB->OnChange(Sig_ChannelOffsetChanged, this);
    scaleSISB = new SI_SpinButton(gtk_adjustment_new(1.0, 2.0e-9, 50, 0.01, 0.1, 0.0), "V");
    scaleSISB->OnChange(Sig_ChannelScaleChanged, this);
    
    gtk_grid_attach(GTK_GRID(layoutGrid), enabledChk, 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), gtk_label_new("Coupling:"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), coupSel, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), bwLimitChk, 0, 4, 1, 1);
    // gtk_grid_attach(GTK_GRID(layoutGrid), filterChk, 1, 4, 1, 1);// Is this even supported? It's in the scope's own GUI...
    gtk_grid_attach(GTK_GRID(layoutGrid), gtk_label_new("Probe:"), 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), probeSel, 1, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), gtk_label_new("Scale (V/div):"), 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), scaleSISB->GetWidget(), 1, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), gtk_label_new("Offset (V):"), 0, 7, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), offsetSISB->GetWidget(), 1, 7, 1, 1);
}


//==============================================================================
// Trigger controls
//==============================================================================

//------------------------------------------------------------------------------
// Common trigger settings
//------------------------------------------------------------------------------
void SetVisibleTrigMode(DS1k_Controller * controller)
{
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(controller->trigCtl->modeSel));
    GtkWidget * desiredPage = controller->trigCtl->triggerGrids[selection];
    // Find and display the associated control page for this trigger source
    int n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(controller->trigCtl->trigNtbk));
    for(int j = 0; j < n; ++j)
    {
        if(gtk_notebook_get_nth_page(GTK_NOTEBOOK(controller->trigCtl->trigNtbk), j) == desiredPage) {
            gtk_notebook_set_current_page(GTK_NOTEBOOK(controller->trigCtl->trigNtbk), j);
            break;
        }
    }
}

void Sig_TrigModeChanged(GtkComboBox * widget, DS1k_Controller * controller)
{
    SetVisibleTrigMode(controller);
    if(controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(controller->trigCtl->modeSel));
    controller->device->tmcDevice->CmdF(":TRIG:MODE %s", selection);
}

void Sig_ForceClicked(GtkButton * btn, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    controller->device->tmcDevice->CmdF(":FORC");
}

void Sig_HOffChanged(GtkSpinButton * spinBtn, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    GtkAdjustment * adjustment = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spinBtn));
    gdouble val = gtk_adjustment_get_value(adjustment);
    controller->device->tmcDevice->CmdF(":TRIG:HOLD %f", val);
    
    // Readback actual level set
    READBACK_DELAY();
    controller->MuteControls();
    string newVal = controller->device->tmcDevice->QueryF(":TRIG:HOLD?");
    controller->trigCtl->hoffSISB->SetValue(strtod(newVal.c_str(), NULL));
    controller->UnmuteControls();
}


DS1k_TriggerCtlPane::DS1k_TriggerCtlPane(DS1k_Controller * scp):
    controller(scp)
{
    frame = gtk_frame_new("Trigger");
    GtkWidget * layoutGrid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(frame), layoutGrid);
    
    modeSel = ComboBoxText("EDGE",
        "EDGE", "Edge",
        "SLOPE", "Slope",
        "PULSE", "Pulse",
        "VIDEO", "Video",
        // "PATTERN", "Pattern",
        // "DURATION", "Duration",
        "ALTERNATION", "Alternation",
        NULL
    );
    g_signal_connect(modeSel, "changed", G_CALLBACK(Sig_TrigModeChanged), controller);
    
    forceTrigBtn = gtk_button_new_with_label("Force");
    g_signal_connect(forceTrigBtn, "clicked", G_CALLBACK(Sig_ForceClicked), controller);
    
    // valid holdoff range is 500 ns..1.5 s
    hoffSISB = new SI_SpinButton(gtk_adjustment_new(500.0e-9, 500.0e-9, 1.5, 0.01, 0.1, 0.0), "s");
    hoffSISB->OnChange(Sig_HOffChanged, controller);
    
    
    EdgeTriggerOpts();
    SlopeTriggerOpts();
    PulseTriggerOpts();
    VideoTriggerOpts();
    // PatternTriggerOpts();
    // DurationTriggerOpts();
    AlternationTriggerOpts();
    
    trigNtbk = gtk_notebook_new();
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(trigNtbk), false);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(trigNtbk), true);
    gtk_notebook_append_page(GTK_NOTEBOOK(trigNtbk), triggerGrids["EDGE"], gtk_label_new("Edge"));
    gtk_notebook_append_page(GTK_NOTEBOOK(trigNtbk), triggerGrids["SLOPE"], gtk_label_new("Slope"));
    gtk_notebook_append_page(GTK_NOTEBOOK(trigNtbk), triggerGrids["PULSE"], gtk_label_new("Pulse"));
    gtk_notebook_append_page(GTK_NOTEBOOK(trigNtbk), triggerGrids["VIDEO"], gtk_label_new("Video"));
    // gtk_notebook_append_page(GTK_NOTEBOOK(trigNtbk), triggerGrids["PATTERN"], gtk_label_new("Pattern"));
    // gtk_notebook_append_page(GTK_NOTEBOOK(trigNtbk), triggerGrids["DURATION"], gtk_label_new("Duration"));
    gtk_notebook_append_page(GTK_NOTEBOOK(trigNtbk), triggerGrids["ALTERNATION"], gtk_label_new("Alternation"));
    
    gtk_grid_attach(GTK_GRID(layoutGrid), gtk_label_new("Mode:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), modeSel, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), forceTrigBtn, 3, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), gtk_label_new("Holdoff:"), 0, 6, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), hoffSISB->GetWidget(), 1, 6, 1, 1);
    // gtk_grid_attach(GTK_GRID(layoutGrid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, 8, 4, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), trigNtbk, 0, 9, 3, 1);
}


//------------------------------------------------------------------------------
// Edge trigger settings
//------------------------------------------------------------------------------

// TODO:
// valid range of level depends on vertical scale of source
void Sig_EdgeTrigSourceSelChanged(GtkComboBox * widget, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    controller->device->tmcDevice->CmdF(":TRIG:EDGE:SOUR %s", selection);
}

void Sig_EdgeTrigLevelChanged(GtkSpinButton * spinBtn, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    GtkAdjustment * adjustment = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spinBtn));
    gdouble val = gtk_adjustment_get_value(adjustment);
    controller->device->tmcDevice->CmdF(":TRIG:EDGE:LEV %f", val);
    
    // Readback actual level set
    READBACK_DELAY();
    controller->MuteControls();
    string newLevel = controller->device->tmcDevice->Query(":TRIG:EDGE:LEV?");
    controller->trigCtl->edgeTrigLevelSISB->SetValue(strtod(newLevel.c_str(), NULL));
    controller->UnmuteControls();
}

void Sig_EdgeTrig50pctClicked(GtkButton * btn, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    controller->device->tmcDevice->CmdF(":Trig%%50");
    string newLevel = controller->device->tmcDevice->Query(":TRIG:EDGE:LEV?");
    controller->trigCtl->edgeTrigLevelSISB->SetValue(strtod(newLevel.c_str(), NULL));
}

void Sig_EdgeTrigSweepSelChanged(GtkComboBox * widget, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    controller->device->tmcDevice->CmdF(":TRIG:EDGE:SWE %s", selection);
}

void Sig_EdgeTrigCoupSelChanged(GtkComboBox * widget, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    controller->device->tmcDevice->CmdF(":TRIG:EDGE:COUP %s", selection);
}

void Sig_EdgeTrigSlopeSelChanged(GtkComboBox * widget, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    controller->device->tmcDevice->CmdF(":TRIG:EDGE:SLOP %s", selection);
}

void Sig_EdgeTrigSensChanged(GtkRange * range, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    gdouble val = gtk_range_get_value(range);
    controller->device->tmcDevice->CmdF(":TRIG:EDGE:SENS %f", val);
}


void DS1k_TriggerCtlPane::EdgeTriggerOpts()
{
    GtkWidget * trigGrid = gtk_grid_new();
    triggerGrids["EDGE"] = trigGrid;
    
    
    // valid trigger range is -6*scale..6*scale
    edgeTrigLevelSISB = new SI_SpinButton(gtk_adjustment_new(0.0, -50.0, 50.0, 1.0, 5.0, 0.0), "V");
    edgeTrigLevelSISB->OnChange(Sig_EdgeTrigLevelChanged, controller);
    
    edgeTrigLevel50PctBtn = gtk_button_new_with_label("50%");
    g_signal_connect(edgeTrigLevel50PctBtn, "clicked", G_CALLBACK(Sig_EdgeTrig50pctClicked), controller);
    
    edgeTrigSourceSel = ComboBoxText("CHAN1",
        "CHAN1", "Channel 1",
        "CHAN2", "Channel 2",
        "EXT", "External",
        "ACL", "AC Line",
        // "DIGn", "Digital n",
        NULL
    );
    g_signal_connect(edgeTrigSourceSel, "changed", G_CALLBACK(Sig_EdgeTrigSourceSelChanged), controller);
    
    edgeTrigSweepSel = ComboBoxText("NORM",
        "NORMAL", "Normal",
        "SINGLE", "Single",
        "AUTO", "Auto",
        NULL
    );
    g_signal_connect(edgeTrigSweepSel, "changed", G_CALLBACK(Sig_EdgeTrigSweepSelChanged), controller);
    
    edgeTrigCoupSel = ComboBoxText("dc",
        "DC", "DC",
        "AC", "AC",
        "HF", "HF",
        "LF", "LF",
        NULL
    );
    g_signal_connect(edgeTrigCoupSel, "changed", G_CALLBACK(Sig_EdgeTrigCoupSelChanged), controller);
    
    
    edgeTrigSlopeSel = ComboBoxText("pos",
        "POSITIVE", "Rising",
        "NEGATIVE", "Falling",
        NULL
    );
    g_signal_connect(edgeTrigSlopeSel, "changed", G_CALLBACK(Sig_EdgeTrigSlopeSelChanged), controller);
    
    edgeTrigSensSlid = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 1.0, 0.1);
    g_signal_connect(edgeTrigSensSlid, "value-changed", G_CALLBACK(Sig_EdgeTrigSensChanged), controller);
    
    gtk_grid_attach(GTK_GRID(trigGrid), gtk_label_new("Source:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), edgeTrigSourceSel, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), gtk_label_new("Level:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), edgeTrigLevelSISB->GetWidget(), 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), edgeTrigLevel50PctBtn, 3, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), gtk_label_new("Sweep:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), edgeTrigSweepSel, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), gtk_label_new("Coupling:"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), edgeTrigCoupSel, 1, 3, 1, 1);
    
    gtk_grid_attach(GTK_GRID(trigGrid), gtk_label_new("Slope:"), 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), edgeTrigSlopeSel, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), gtk_label_new("Sens. (divs):"), 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), edgeTrigSensSlid, 1, 5, 1, 1);
} // EdgeTriggerOpts()

//------------------------------------------------------------------------------
// Slope trigger settings
//------------------------------------------------------------------------------

void Sig_SlopeTrigModeSelChanged(GtkComboBox * widget, DS1k_Controller * controller) {
}

void Sig_SlopeTrigWindSelChanged(GtkComboBox * widget, DS1k_Controller * controller) {
}

void Sig_SlopeTrigTimeChanged(GtkSpinButton * btn, DS1k_Controller * controller) {
    
}

void Sig_SlopeTrigSensChanged(GtkScale * widget, DS1k_Controller * controller) {
}

void DS1k_TriggerCtlPane::SlopeTriggerOpts()
{
    GtkWidget * trigGrid = gtk_grid_new();
    triggerGrids["SLOPE"] = trigGrid;
    
    slopeTrigModeSel = ComboBoxText("gt",
        "+gt", "Pos >",
        "+lt", "Pos <",
        "+eq", "Pos =",
        "-gt", "Neg >",
        "-lt", "Neg <",
        "-eq", "Neg =",
        NULL
    );
    g_signal_connect(slopeTrigModeSel, "changed", G_CALLBACK(Sig_SlopeTrigModeSelChanged), controller);
    
    slopeTrigWindSel = ComboBoxText("gt",
        "a", "A",
        "b", "B",
        "ab", "AB",
        NULL
    );
    g_signal_connect(slopeTrigWindSel, "changed", G_CALLBACK(Sig_SlopeTrigWindSelChanged), controller);
    
    SI_SpinButton * slopeTrigTimeSISB = new SI_SpinButton(gtk_adjustment_new(20.0e-9, 500.0e-9, 10, 0.01, 0.1, 0.0), "s");
    slopeTrigTimeSISB->OnChange(Sig_SlopeTrigTimeChanged, controller);
    slopeTrigTimeEnt = slopeTrigTimeSISB->GetWidget();
    
    slopeTrigSensSlid = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.1, 1.0, 0.1);
    g_signal_connect(slopeTrigSensSlid, "value-changed", G_CALLBACK(Sig_SlopeTrigSensChanged), controller);
    
    gtk_grid_attach(GTK_GRID(trigGrid), gtk_label_new("Mode:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), slopeTrigModeSel, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), gtk_label_new("Slope:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), slopeTrigTimeEnt, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), gtk_label_new("Window:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), slopeTrigWindSel, 1, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), gtk_label_new("Sens. (divs):"), 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(trigGrid), slopeTrigSensSlid, 1, 3, 1, 1);
} // SlopeTriggerOpts()

//------------------------------------------------------------------------------
// Pulse trigger settings
//------------------------------------------------------------------------------
void DS1k_TriggerCtlPane::PulseTriggerOpts() {
    GtkWidget * trigGrid = gtk_grid_new();
    triggerGrids["PULSE"] = trigGrid;
}

//------------------------------------------------------------------------------
// Video trigger settings
//------------------------------------------------------------------------------
void DS1k_TriggerCtlPane::VideoTriggerOpts() {
    GtkWidget * trigGrid = gtk_grid_new();
    triggerGrids["VIDEO"] = trigGrid;
}

//------------------------------------------------------------------------------
// Pattern trigger settings
//------------------------------------------------------------------------------
// void DS1k_TriggerCtlPane::PatternTriggerOpts() {
//     GtkWidget * trigGrid = gtk_grid_new();
//     triggerGrids["PATTERN"] = trigGrid;
// }

//------------------------------------------------------------------------------
// Duration trigger settings
//------------------------------------------------------------------------------
// void DS1k_TriggerCtlPane::DurationTriggerOpts() {
//     GtkWidget * trigGrid = gtk_grid_new();
//     triggerGrids["DURATION"] = trigGrid;
// }

//------------------------------------------------------------------------------
// Alternation trigger settings
//------------------------------------------------------------------------------
void DS1k_TriggerCtlPane::AlternationTriggerOpts() {
    GtkWidget * trigGrid = gtk_grid_new();
    triggerGrids["ALTERNATION"] = trigGrid;
}


//==============================================================================
// Main controls
//==============================================================================

void Sig_SetNameClicked(GtkButton * btn, DS1k_Controller * controller) {
    gtk_label_set_text(GTK_LABEL(controller->tabLbl), "Name");
}

void Sig_DumpCfgClicked(GtkButton * btn, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    std::map<std::string, std::string> params;
    GetParams(controller->device->tmcDevice, params);
    
    map<string, string>::iterator pi;
    for(pi = params.begin(); pi != params.end(); ++pi)
        cerr << pi->first << ": " << pi->second << endl;
}

void Sig_LoadCfgClicked(GtkButton * btn, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    
    
    // SetParams(params);
}

void Sig_KeyLockToggled(GtkToggleButton * btn, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    if(gtk_toggle_button_get_active(btn))
        controller->device->tmcDevice->CmdF(":KEY:LOCK ENAB");
    else
        controller->device->tmcDevice->CmdF(":KEY:LOCK DIS");
}

void Sig_RunToggled(GtkToggleButton * btn, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    if(gtk_toggle_button_get_active(btn))
        controller->device->tmcDevice->CmdF(":RUN");
    else
        controller->device->tmcDevice->CmdF(":STOP");
}

void Sig_AutoClicked(GtkButton * btn, DS1k_Controller * controller) {
    if(!(controller->device))
        return;
    
    controller->device->tmcDevice->CmdF(":AUTO");
}

void Sig_TimebaseOffsetChanged(GtkSpinButton * spinBtn, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    GtkAdjustment * adjustment = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spinBtn));
    gdouble val = gtk_adjustment_get_value(adjustment);
    controller->device->tmcDevice->CmdF(":TIM:OFFS %f", val);
    
    // Readback actual level set
    READBACK_DELAY();
    controller->MuteControls();
    string newVal = controller->device->tmcDevice->QueryF(":TIM:OFFS?");
    controller->timebaseOffsetSISB->SetValue(strtod(newVal.c_str(), NULL));
    controller->UnmuteControls();
}

void Sig_TimebaseScaleChanged(GtkSpinButton * spinBtn, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    GtkAdjustment * adjustment = gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(spinBtn));
    gdouble val = gtk_adjustment_get_value(adjustment);
    controller->device->tmcDevice->CmdF(":TIM:SCAL %f", val);
    
    // Readback actual level set
    READBACK_DELAY();
    controller->MuteControls();
    string newVal = controller->device->tmcDevice->QueryF(":TIM:SCAL?");
    controller->timebaseScaleSISB->SetValue(strtod(newVal.c_str(), NULL));
    controller->UnmuteControls();
}

// TODO: Timebase offset/scale range depend on mode:
// Scale:
// YT: 2 ns - 50 s/div
// ROLL: 500 ms - 50 s/div
// Offset:
// NORMAL: 1 s - end of memory
// STOP: +- 500 s
// ROLL offset: +- 6*scale

void Sig_TimebaseFormatSelChanged(GtkComboBox * widget, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    controller->device->tmcDevice->CmdF(":TIM:FORM %s", selection);
}

void Sig_AcqTypeSelChanged(GtkComboBox * widget, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    controller->device->tmcDevice->CmdF(":ACQ:TYPE %s", selection);
}

void Sig_AcqModeSelChanged(GtkComboBox * widget, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    controller->device->tmcDevice->CmdF(":ACQ:MODE %s", selection);
}

void Sig_AcqAverSelChanged(GtkComboBox * widget, DS1k_Controller * controller) {
    if(controller->ControlsMuted())
        return;
    
    const gchar * selection = gtk_combo_box_get_active_id(GTK_COMBO_BOX(widget));
    controller->device->tmcDevice->CmdF(":ACQ:AVER %s", selection);
}


DS1k_Controller::DS1k_Controller(ScopeModel * m, const std::string & sn):
    model(m),
    controlsMuted(0),
    name(sn),
    sernum(sn)
{
    // Don't talk to device while GUI is being configured
    MuteControls();
    
    tabLbl = gtk_label_new(sernum.c_str());
    layoutGrid = gtk_grid_new();
    
    // Do basic initialization of scope (stop scope, setup memory depth, etc)
    device = model->GetDevice(sn);
    device->tmcDevice->CmdF(":STOP");
    device->tmcDevice->CmdF(":TIM:MODE MAIN");
    device->tmcDevice->CmdF(":CHAN1:INV OFF");// Inversion done on viewer side
    device->tmcDevice->CmdF(":CHAN2:INV OFF");
    device->tmcDevice->CmdF(":ACQ:MEMD NORM");
    // controller->device->tmcDevice->CmdF(":ACQ:MEMD LONG");
    device->tmcDevice->CmdF(":WAV:POIN:MODE NORM");
    // device->tmcDevice->CmdF(":WAV:POIN:MODE RAW");
    
    // Setup GUI
    GtkWidget * setNameBtn = gtk_button_new_with_label("Set Name");
    g_signal_connect(setNameBtn, "clicked", G_CALLBACK(Sig_SetNameClicked), this);
    GtkWidget * dumpCfgBtn = gtk_button_new_with_label("Dump Config");
    g_signal_connect(dumpCfgBtn, "clicked", G_CALLBACK(Sig_DumpCfgClicked), this);
    
    GtkWidget * loadCfgBtn = gtk_button_new_with_label("Load Config");
    g_signal_connect(loadCfgBtn, "clicked", G_CALLBACK(Sig_LoadCfgClicked), this);
    
    keyLockChk = gtk_check_button_new_with_label("Key Lock");
    g_signal_connect(keyLockChk, "toggled", G_CALLBACK(Sig_KeyLockToggled), this);
    
    runChk = gtk_check_button_new_with_label("Run");
    g_signal_connect(runChk, "toggled", G_CALLBACK(Sig_RunToggled), this);
    GtkWidget * autoBtn = gtk_button_new_with_label("Auto");
    g_signal_connect(autoBtn, "clicked", G_CALLBACK(Sig_AutoClicked), this);
    
    timebaseFormatSel = ComboBoxText("YT",
        "YT", "Y-T",
        "XY", "X-Y",
        "SCANNING", "Scan",
        NULL
    );
    g_signal_connect(timebaseFormatSel, "changed", G_CALLBACK(Sig_TimebaseFormatSelChanged), this);
    
    
    GtkWidget * timebaseFrame = gtk_frame_new("Timebase");
    GtkWidget * tbLayoutGrid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(timebaseFrame), tbLayoutGrid);
    timebaseOffsetSISB = new SI_SpinButton(gtk_adjustment_new(0.0, -500, 500, 0.01, 0.1, 0.0), "s");
    timebaseOffsetSISB->OnChange(Sig_TimebaseOffsetChanged, this);
    timebaseScaleSISB = new SI_SpinButton(gtk_adjustment_new(1.0, 2.0e-9, 50, 0.01, 0.1, 0.0), "s");
    timebaseScaleSISB->OnChange(Sig_TimebaseScaleChanged, this);
    
    gtk_grid_attach(GTK_GRID(tbLayoutGrid), gtk_label_new("Mode:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(tbLayoutGrid), timebaseFormatSel, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(tbLayoutGrid), gtk_label_new("Scale (s/div):"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(tbLayoutGrid), timebaseScaleSISB->GetWidget(), 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(tbLayoutGrid), gtk_label_new("Offset (s):"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(tbLayoutGrid), timebaseOffsetSISB->GetWidget(), 1, 2, 1, 1);
    
    
    acqTypeSel = ComboBoxText("NORMAL",
        "NORMAL", "Normal",
        "AVERAGE", "Average",
        "PEAKDETECT", "Peak Detect",
        NULL
    );
    g_signal_connect(acqTypeSel, "changed", G_CALLBACK(Sig_AcqTypeSelChanged), this);
    acqModeSel = ComboBoxText("RTIM",
        "RTIM", "Realtime",
        "ETIM", "Equal Time",
        NULL
    );
    g_signal_connect(acqModeSel, "changed", G_CALLBACK(Sig_AcqModeSelChanged), this);
    acqAverSel = ComboBoxText("8",
        "2", "2",
        "4", "4",
        "8", "8",
        "16", "16",
        "32", "32",
        "64", "64",
        "128", "128",
        "256", "256",
        NULL
    );
    g_signal_connect(acqModeSel, "changed", G_CALLBACK(Sig_AcqAverSelChanged), this);
    
    GtkWidget * acqFrame = gtk_frame_new("Acquire");
    GtkWidget * acqGrid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(acqFrame), acqGrid);
    
    
    gtk_grid_attach(GTK_GRID(acqGrid), gtk_label_new("Type:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(acqGrid), acqTypeSel, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(acqGrid), gtk_label_new("Mode:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(acqGrid), acqModeSel, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(acqGrid), gtk_label_new("Aver:"), 0, 2, 1, 1);
    gtk_grid_attach(GTK_GRID(acqGrid), acqAverSel, 1, 2, 1, 1);
    
    
    
    chan[0] = new ScopeChanCtlPane(this, "Channel 1", 0);
    chan[1] = new ScopeChanCtlPane(this, "Channel 2", 1);
    trigCtl = new DS1k_TriggerCtlPane(this);
    
    GtkWidget * chTrigGrid = gtk_grid_new();
    gtk_grid_attach(GTK_GRID(layoutGrid), chTrigGrid, 0, 2, 3, 1);
    gtk_grid_attach(GTK_GRID(chTrigGrid), chan[0]->frame, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(chTrigGrid), chan[1]->frame, 2, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(chTrigGrid), trigCtl->frame, 4, 0, 1, 1);
    
    gtk_grid_attach(GTK_GRID(layoutGrid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, 3, 6, 1);
    // gtk_grid_attach(GTK_GRID(layoutGrid), setNameBtn, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), timebaseFrame, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), acqFrame, 1, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), autoBtn, 4, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), runChk, 5, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), keyLockChk, 2, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), dumpCfgBtn, 0, 5, 1, 1);
    gtk_grid_attach(GTK_GRID(layoutGrid), loadCfgBtn, 1, 5, 1, 1);
    
    // Get parameters from device so UI can be initialized to actual state
    std::map<std::string, std::string> params;
    GetParams(device->tmcDevice, params);
    SetParams(params);
    UnmuteControls();
}

// TODO: ACQ, trigger controls
void DS1k_Controller::SetParams(std::map<std::string, std::string> & params)
{
    // Don't talk to device while GUI is being configured
    MuteControls();
    
    // ":ACQ:TYPE?" returns: NORMAL AVERAGE PEAKDETECT, to set: NORM AVER PEAK
    // ":ACQ:MODE?" returns: REAL_TIME EQUAL_TIME, to set: RTIM ETIM
    // ":ACQ:AVER?" returns: 2, 4, 8, 16, 32, 64, 128, 256
    // ":ACQ:SAMP? CHAN1" returns: decimal floating point Hz
    // ":ACQ:SAMP? CHAN2" returns: decimal floating point Hz
    // ":ACQ:MEMD?" returns: LONG NORMAL, to set: LONG NORM
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(acqTypeSel), params[":ACQ:TYPE?"].c_str());
    
    if(params[":ACQ:MODE?"] == "REAL_TIME")
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(acqModeSel), "RTIM");
    else
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(acqModeSel), "ETIM");
    
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(acqAverSel), params[":ACQ:AVER?"].c_str());
    
    // None of this stuff is relevant...doing our own UI and display
    // ":DISP:TYPE?"
    // ":DISP:GRID?"
    // ":DISP:PERS?"
    // ":DISP:MNUD?"
    // ":DISP:MNUS?"
    // ":DISP:BRIG?"
    // ":DISP:INT?"
    
    // "Delayed scan" is actually a mode that displays a second magnified portion of the waveform.
    // We have our own means of doing this and more, so we only need to expose the main timebase
    // settings.
    // ":TIM:OFFS?" decimal floating point s
    // ":TIM:SCAL?" decimal floating point s/div
    // ":TIM:FORM?" X-Y Y-T SCANNING, XY YT SCAN
    timebaseOffsetSISB->SetValue(strtod(params[":TIM:OFFS?"].c_str(), NULL));
    timebaseScaleSISB->SetValue(strtod(params[":TIM:SCAL?"].c_str(), NULL));
    
    if(params[":TIM:FORM?"] == "Y-T")
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->modeSel), "YT");
    else if(params[":TIM:FORM?"] == "X-Y")
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->modeSel), "XY");
    else
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->modeSel), "SCANNING");
    
    
    // ":TRIG:MODE?"
    // ":TRIG:HOLD?"
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->modeSel), params[":TRIG:MODE?"].c_str());
    SetVisibleTrigMode(this);
    
    trigCtl->hoffSISB->SetValue(strtod(params[":TRIG:HOLD?"].c_str(), NULL));
    
    
    // ":TRIG:EDGE:SOUR?"
    // ":TRIG:EDGE:LEV?"
    // ":TRIG:EDGE:SWE?"
    // ":TRIG:EDGE:COUP?"
    // ":TRIG:EDGE:SLOP?"
    // ":TRIG:EDGE:SENS?"
    if(params[":TRIG:EDGE:SOUR?"] == "CH1")
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->edgeTrigSourceSel), "CHAN1");
    else if(params[":TRIG:EDGE:SOUR?"] == "CH2")
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->edgeTrigSourceSel), "CHAN2");
    else if(params[":TRIG:EDGE:SOUR?"] == "EXT")
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->edgeTrigSourceSel), "EXT");
    else
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->edgeTrigSourceSel), "ACL");
    
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->edgeTrigSweepSel), params[":TRIG:EDGE:SWE?"].c_str());
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->edgeTrigCoupSel), params[":TRIG:EDGE:COUP?"].c_str());
    gtk_combo_box_set_active_id(GTK_COMBO_BOX(trigCtl->edgeTrigSlopeSel), params[":TRIG:EDGE:SLOP?"].c_str());
    
    trigCtl->edgeTrigLevelSISB->SetValue(strtod(params[":TRIG:EDGE:LEV?"].c_str(), NULL));
    gtk_range_set_value(GTK_RANGE(trigCtl->edgeTrigSensSlid), strtod(params[":TRIG:EDGE:SENS?"].c_str(), NULL));
    
    
    // ":TRIG:PULS:SOUR?"
    // ":TRIG:PULS:LEV?"
    // ":TRIG:PULS:SWE?"
    // ":TRIG:PULS:COUP?"
    // ":TRIG:PULS:MODE?"
    // ":TRIG:PULS:SENS?"
    // ":TRIG:PULS:WIDT?"
    
    // ":TRIG:VIDEO:SOUR?"
    // ":TRIG:VIDEO:LEV?"
    // TODO
    
    // GtkWidget * slopeTrigModeSel;
    // GtkWidget * slopeTrigWindSel;
    // GtkWidget * slopeTrigTimeEnt;
    // GtkWidget * slopeTrigSensSlid;
    // ":TRIG:SLOP:SOUR?"
    // ":TRIG:SLOP:SWE?"
    // ":TRIG:SLOP:COUP?"
    // TODO
    
    // ":TRIG:PATT:SWE?"
    // TODO
    
    // ":TRIG:DUR:PATT?"
    // TODO
    
    // ":TRIG:ALT:SOUR?"
    // TODO
    
    // ":MATH:DISP?"
    // ":MATH:OPER?"
    // ":FFT:DISP?"
    
    //     ":CHANx:DISP?"
    //     ":CHANx:COUP?"
    //     ":CHANx:PROB?"
    //     ":CHANx:BWL?"
    //     ":CHANx:FILT?"
    //     ":CHANx:INV?"
    //     ":CHANx:OFFS?"
    //     ":CHANx:SCAL?"
    //     ":CHANx:MEMD?"
    //     ":CHANx:VERN?"
    for(int j = 0; j < 2; ++j)
    {
        string ch(string(":CHAN") + (char)('1' + j));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chan[j]->bwLimitChk), (params[ch + ":BWL?"] == "ON"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chan[j]->filterChk), (params[ch + ":FILT?"] == "ON"));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(chan[j]->enabledChk), (params[ch + ":DISP?"] == "1"));
        chan[j]->offsetSISB->SetValue(strtod(params[ch + ":OFFS?"].c_str(), NULL));
        chan[j]->scaleSISB->SetValue(strtod(params[ch + ":SCAL?"].c_str(), NULL));
        
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(chan[j]->coupSel), params[":COUP?"].c_str());
        
        // Reported attenuation is of the form 1.000e+01, convert to an integer
        char bfr[1024];
        snprintf(bfr, 1024, "%d", (int)strtod(params[ch + ":PROB?"].c_str(), NULL));
        gtk_combo_box_set_active_id(GTK_COMBO_BOX(chan[j]->probeSel), bfr);
    }
    
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(keyLockChk), (params[":KEY:LOCK?"] == "ENABLE"));
    
    UnmuteControls();
}

//******************************************************************************