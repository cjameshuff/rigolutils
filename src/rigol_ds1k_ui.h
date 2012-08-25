#ifndef RIGOL_DS1K_UI_H
#define RIGOL_DS1K_UI_H

#include <gtk/gtk.h>
#include "gtkhelpers.h"
#include "scopev_model.h"

#include <map>

//******************************************************************************
// GtkSpinButton that handles SI units.
// Adjusts input/output presentation scale to keep numeric values in range +-0-999.99...
// Adjusts increment to match current scale.
// Uses all multiple-of-1000 prefixes

// The value stored by the spin button is the actual numeric value in base units.
// The increments are adjusted to achieve the same decimal position increment in
// the displayed multiple.
class SI_SpinButton {
  private:
    GtkSpinButton * spinBtn;
    std::string unit;
    int scale;
    int defaultExponent;
    
    void (*changedHandler)(GtkSpinButton *,gpointer);
    gpointer changedUserData;
    
    static gint Sig_Input(GtkSpinButton * spinBtn, gdouble * newVal, SI_SpinButton * data) {
        return data->Input(newVal);
    }
    static gboolean Sig_Output(GtkSpinButton * spinBtn, SI_SpinButton * data) {
        return data->Output();
    }
    static void Sig_Changed(GtkSpinButton * spinBtn, SI_SpinButton * data) {
        data->Changed();
    }
    
    // If unit is omitted, assume default exponent.
    gint Input(gdouble * newVal)
    {
        const gchar * text = gtk_entry_get_text(GTK_ENTRY(spinBtn));
        GtkAdjustment * adjustment = gtk_spin_button_get_adjustment(spinBtn);
        
        char * unitStr = NULL;
        units::d_double val;
        try {
            val = units::StrToD(text, NULL, units::GetUnit(unit));
        }
        catch(units::DimensionError & err)
        {
            return GTK_INPUT_ERROR;
        }
        
        *newVal = val.Value();
        // gtk_spin_button_set_increments(spinBtn, step, page);
    }

    gboolean Output()
    {
        GtkAdjustment * adjustment = gtk_spin_button_get_adjustment(spinBtn);
        gdouble value = gtk_adjustment_get_value(adjustment);
        
        units::d_double dvalue(value, units::GetUnit(unit));
        units::FmtUnit fmt_s(unit);
        std::string str = fmt_s(dvalue);
        gtk_entry_set_text(GTK_ENTRY(spinBtn), str.c_str());
        return true;
    }

    //
    void Changed()
    {
        Output();
        if(changedHandler)
            changedHandler(spinBtn, changedUserData);
    }
    
  public:
    // SI_SpinButton(GtkAdjustment * adj, const std::string & u, gdouble rangeMin, gdouble rangeMax, gdouble step):
    SI_SpinButton(GtkAdjustment * adj, const std::string & u, guint digits = 4):
        unit(u),
        scale(0),
        defaultExponent(0),
        changedHandler(NULL),
        changedUserData(NULL)
    {
        spinBtn = GTK_SPIN_BUTTON(gtk_spin_button_new(adj, 1.0, digits));
        gtk_spin_button_set_numeric(spinBtn, false);
        // spinBtn = gtk_spin_button_new_with_range(rangeMin, rangeMax, step);
        g_signal_connect(spinBtn, "input", G_CALLBACK(Sig_Input), this);
        g_signal_connect(spinBtn, "output", G_CALLBACK(Sig_Output), this);
        g_signal_connect(spinBtn, "value-changed", G_CALLBACK(Sig_Changed), this);
    }
    
    template<typename T>
    void OnChange(void (*ch)(GtkSpinButton *,T*), gpointer userdat) {
        changedHandler = (void (*)(GtkSpinButton*, gpointer))ch;
        changedUserData = userdat;
    }
    
    void SetValue(gdouble val) {gtk_spin_button_set_value(spinBtn, val);}
    
    void SilentSetValue(gdouble val) {
        g_signal_handlers_block_by_func((gpointer)spinBtn, (gpointer)Sig_Changed, (gpointer)this);
        gtk_spin_button_set_value(spinBtn, val);
        g_signal_handlers_unblock_by_func((gpointer)spinBtn, (gpointer)Sig_Changed, (gpointer)this);
    }
    
    gdouble GetValue() {
        GtkAdjustment * adjustment = gtk_spin_button_get_adjustment(spinBtn);
        return gtk_adjustment_get_value(adjustment);
    }
    
    GtkWidget * GetWidget() {return GTK_WIDGET(spinBtn);}
};


//******************************************************************************

struct DS1k_Controller;
struct ScopeChanCtlPane {
    DS1k_Controller * controller;
    
    int channel;
    
    GtkWidget * frame;
    GtkWidget * enabledChk;
    // GtkWidget * invertChk;
    GtkWidget * filterChk;
    GtkWidget * bwLimitChk;
    GtkWidget * coupSel;
    GtkWidget * probeSel;
    SI_SpinButton * offsetSISB;
    SI_SpinButton * scaleSISB;
    
    ScopeChanCtlPane(DS1k_Controller * scp, const char * name, int chan);
    
    // void MuteControls() {controller->MuteControls();}
    // void UnmuteControls() {controller->UnmuteControls();}
    // bool ControlsMuted() {return controller->ControlsMuted();}
};


struct DS1k_TriggerCtlPane {
    DS1k_Controller * controller;
    GtkWidget * frame;
    
    GtkWidget * modeSel;
    SI_SpinButton * hoffSISB;
    GtkWidget * forceTrigBtn;
    
    GtkWidget * edgeTrigSourceSel;
    SI_SpinButton * edgeTrigLevelSISB;
    GtkWidget * edgeTrigLevel50PctBtn;
    GtkWidget * edgeTrigSweepSel;
    GtkWidget * edgeTrigCoupSel;
    GtkWidget * edgeTrigSlopeSel;
    GtkWidget * edgeTrigSensSlid;
    
    GtkWidget * slopeTrigModeSel;
    GtkWidget * slopeTrigWindSel;
    GtkWidget * slopeTrigTimeEnt;
    GtkWidget * slopeTrigSensSlid;
    
    GtkWidget * trigNtbk;
    std::map<std::string, GtkWidget *> triggerGrids;
    
    DS1k_TriggerCtlPane(DS1k_Controller * scp);
    
    void EdgeTriggerOpts();
    void SlopeTriggerOpts();
    void PulseTriggerOpts();
    void VideoTriggerOpts();
    // void PatternTriggerOpts();
    // void DurationTriggerOpts();
    void AlternationTriggerOpts();
    
    // void MuteControls() {controller->MuteControls();}
    // void UnmuteControls() {controller->UnmuteControls();}
    // bool ControlsMuted() {return controller->ControlsMuted();}
};

struct DS1k_Controller {
    ScopeModel * model;
    Device * device;
    int controlsMuted;
    
    std::string name;
    std::string sernum;
    GtkWidget * tabLbl;
    GtkWidget * layoutGrid;
    
    GtkWidget * timebaseFormatSel;
    SI_SpinButton * timebaseOffsetSISB;
    SI_SpinButton * timebaseScaleSISB;
    
    GtkWidget * acqTypeSel;
    GtkWidget * acqModeSel;
    GtkWidget * acqAverSel;
    
    GtkWidget * runChk;
    GtkWidget * keyLockChk;
    ScopeChanCtlPane * chan[2];
    DS1k_TriggerCtlPane * trigCtl;
    
    DS1k_Controller(ScopeModel * m, const std::string & sn);
    
    void SetParams(std::map<std::string, std::string> & params);
    
    // Enable/disable control of device
    // (used to suppress spurious commands when syncing the UI)
    void MuteControls() {++controlsMuted;}
    void UnmuteControls() {--controlsMuted;}
    bool ControlsMuted() {return controlsMuted != 0;}
};

//******************************************************************************
#endif // RIGOL_DS1K_UI_H
