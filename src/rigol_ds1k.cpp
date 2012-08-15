
#include "rigol_ds1k.h"

#include <algorithm>

using namespace std;


map<string, ParamSet *> paramSets;

ParamSet * GetParamSet(const string & devName)
{
    map<string, ParamSet *>::iterator ps = paramSets.find(devName);
    return (ps == paramSets.end())? NULL : (ps->second);
}

void ParamSet::DefParam(const string & cat, const string & pname, const string & nicename)
{
    paramNames[cat].push_back(pname);
    niceNames[pname] = nicename;
}

//******************************************************************************

void Init_RigolDS1K()
{
    char bfr[1024];
    char bfr2[1024];
    char bfr3[1024];
    
    ParamSet * ps = new ParamSet;
    paramSets["DS1102E"] = ps;
    paramSets["DS1052E"] = ps;
    
    //==========================================================================
    // DS1xxxE
    //==========================================================================
    // Parameter categories
    string pc[] = {
        "ACQ", "DISP", "TIM", "TRIG", "MATH", "FFT",
        "KEY", "INFO", "COUN", "BEEP"
    };
    string tpc[] = {
        "TRIG:EDGE", "TRIG:PULS", "TRIG:VIDEO", "TRIG:SLOP", "TRIG:PATT"
    };
    ps->mainParamCats.resize(sizeof(pc)/sizeof(string));
    copy(pc, pc + ps->mainParamCats.size(), ps->mainParamCats.begin());
    ps->trigParamCats.resize(sizeof(pc)/sizeof(string));
    copy(pc, pc + ps->trigParamCats.size(), ps->trigParamCats.begin());
    
    ps->DefParam("ACQ", ":ACQ:TYPE?", "AcquireType");
    ps->DefParam("ACQ", ":ACQ:MODE?", "AcquireMode");
    ps->DefParam("ACQ", ":ACQ:AVER?", "AcquireAverages");
    ps->DefParam("ACQ", ":ACQ:SAMP? CHAN1", "Ch1_SamplingRate");
    ps->DefParam("ACQ", ":ACQ:SAMP? CHAN2", "Ch2_SamplingRate");
    ps->DefParam("ACQ", ":ACQ:MEMD?", "MemoryDepth");
    
    ps->DefParam("DISP", ":DISP:TYPE?", "DisplayType");
    ps->DefParam("DISP", ":DISP:GRID?", "DisplayGrid");
    ps->DefParam("DISP", ":DISP:PERS?", "DisplayPersist");
    ps->DefParam("DISP", ":DISP:MNUD?", "DisplayMenuDisplay");
    ps->DefParam("DISP", ":DISP:MNUS?", "DisplayMenuStatus");
    ps->DefParam("DISP", ":DISP:BRIG?", "DisplayBrightness");
    ps->DefParam("DISP", ":DISP:INT?", "DisplayIntensity");
    
    ps->DefParam("TIM", ":TIM:MODE?", "TimebaseMode");
    ps->DefParam("TIM", ":TIM:OFFS?", "TimebaseOffset");
    ps->DefParam("TIM", ":TIM:DEL:OFFS?", "DelayedTimebaseOffset");
    ps->DefParam("TIM", ":TIM:SCAL?", "TimebaseScale");
    ps->DefParam("TIM", ":TIM:DEL:SCAL?", "DelayedTimebaseScale");
    ps->DefParam("TIM", ":TIM:FORM?", "TimebaseFormat");
    
    ps->DefParam("TRIG", ":TRIG:MODE?", "TriggerMode");
    ps->DefParam("TRIG", ":TRIG:HOLD?", "TriggerHoldoff");
    ps->DefParam("TRIG", ":TRIG:STAT?", "TriggerStatus");
    
    // Trigger modes: EDGE, PULS, VIDEO, SLOP, PATT, ALT
    ps->DefParam("TRIG:EDGE", ":TRIG:EDGE:SOUR?", "TriggerSource");
    ps->DefParam("TRIG:EDGE", ":TRIG:EDGE:LEV?", "TriggerLevel");
    ps->DefParam("TRIG:EDGE", ":TRIG:EDGE:SWE?", "TriggerSweep");
    ps->DefParam("TRIG:EDGE", ":TRIG:EDGE:COUP?", "TriggerCoupling");
    ps->DefParam("TRIG:EDGE", ":TRIG:EDGE:SLOP?", "TriggerEdgeSlope");
    ps->DefParam("TRIG:EDGE", ":TRIG:EDGE:SENS?", "TriggerEdgeSensitivity");
    
    ps->DefParam("TRIG:PULS", ":TRIG:PULS:SOUR?", "TriggerSource");
    ps->DefParam("TRIG:PULS", ":TRIG:PULS:LEV?", "TriggerLevel");
    ps->DefParam("TRIG:PULS", ":TRIG:PULS:SWE?", "TriggerSweep");
    ps->DefParam("TRIG:PULS", ":TRIG:PULS:COUP?", "TriggerCoupling");
    ps->DefParam("TRIG:PULS", ":TRIG:PULS:MODE?", "TriggerPulseMode");
    ps->DefParam("TRIG:PULS", ":TRIG:PULS:SENS?", "TriggerPulseSensitivity");
    ps->DefParam("TRIG:PULS", ":TRIG:PULS:WIDT?", "TriggerPulseWidth");
    
        ps->DefParam("TRIG:VIDEO", ":TRIG:VIDEO:SOUR?", "TriggerSource");
        ps->DefParam("TRIG:VIDEO", ":TRIG:VIDEO:LEV?", "TriggerLevel");
        ps->DefParam("TRIG:VIDEO", ":TRIG:VIDEO:SWE?", "TriggerSweep");
        ps->DefParam("TRIG:VIDEO", ":TRIG:VIDEO:COUP?", "TriggerCoupling");
        // TODO
    
        ps->DefParam("TRIG:SLOP", ":TRIG:SLOP:SOUR?", "TriggerSource");
        ps->DefParam("TRIG:SLOP", ":TRIG:SLOP:LEV?", "TriggerLevel");
        ps->DefParam("TRIG:SLOP", ":TRIG:SLOP:SWE?", "TriggerSweep");
        ps->DefParam("TRIG:SLOP", ":TRIG:SLOP:COUP?", "TriggerCoupling");
        // TODO
    
        ps->DefParam("TRIG:PATT", ":TRIG:PATT:SOUR?", "TriggerSource");
        ps->DefParam("TRIG:PATT", ":TRIG:PATT:LEV?", "TriggerLevel");
        ps->DefParam("TRIG:PATT", ":TRIG:PATT:SWE?", "TriggerSweep");
        ps->DefParam("TRIG:PATT", ":TRIG:PATT:COUP?", "TriggerCoupling");
        // TODO
    
        ps->DefParam("TRIG:ALT", ":TRIG:ALT:SOUR?", "TriggerSource");
        ps->DefParam("TRIG:ALT", ":TRIG:ALT:LEV?", "TriggerLevel");
        ps->DefParam("TRIG:ALT", ":TRIG:ALT:SWE?", "TriggerSweep");
        ps->DefParam("TRIG:ALT", ":TRIG:ALT:COUP?", "TriggerCoupling");
        // TODO
    
    ps->DefParam("MATH", ":MATH:DISP?", "MathDisplay");
    ps->DefParam("MATH", ":MATH:OPER?", "MathOperation");
    ps->DefParam("FFT", ":FFT:DISP?", "FFT_Display");
    
    for(int j = 0; j < 2; ++j)
    {
        string chan(string(":CHAN") + (char)('1' + j));
        string ch(string("Ch") + (char)('1' + j));
        ps->mainParamCats.push_back("CHAN" + chan);
        ps->DefParam(chan, chan + ":BWL?", ch + "_BandwidthLimit");
        ps->DefParam(chan, chan + ":COUP?", ch + "_Coupling");
        ps->DefParam(chan, chan + ":DISP?", ch + "_Display");
        ps->DefParam(chan, chan + ":INV?", ch + "_Invert");
        ps->DefParam(chan, chan + ":OFFS?", ch + "_Offset");
        ps->DefParam(chan, chan + ":PROB?", ch + "_Probe");
        ps->DefParam(chan, chan + ":SCAL?", ch + "_Scale");
        ps->DefParam(chan, chan + ":FILT?", ch + "_Filter");
        ps->DefParam(chan, chan + ":MEMD?", ch + "_MemoryDepth");
        ps->DefParam(chan, chan + ":VERN?", ch + "_Vernier");
    }
    
    // ps->DefParam(":MEAS:?", "Measure");
    
    ps->DefParam("WAV", ":WAV:POIN:MODE?", "WaveformPointsMode");
    
    ps->DefParam("KEY", ":KEY:LOCK?", "KeysLocked");
    ps->DefParam("INFO", ":INFO:LANG?", "Language");
    ps->DefParam("COUN", ":COUN:ENAB?", "FreqCounterEnabled");
    ps->DefParam("BEEP", ":BEEP:ENAB?", "BeepEnabled");
    
    //==========================================================================
    // DS1xxxD
    //==========================================================================
    ps = new ParamSet;
    paramSets["DS1102D"] = ps;
    paramSets["DS1052D"] = ps;
    
    *ps = *paramSets["DS1102E"];
    
    ps->mainParamCats.push_back("LA");
    
    ps->DefParam("ACQ", ":ACQ:SAMP? DIGITAL", "DigitalSamplingRate");
    ps->DefParam("LA", ":LA:DISP?", "LogicDisplay");
    for(int j = 0; j < 16; ++j)
    {
        snprintf(bfr3, 1024, "DIG%d", j);
        ps->mainParamCats.push_back(bfr3);
        
        snprintf(bfr, 1024, ":DIG%d:TURN?", j);
        snprintf(bfr2, 1024, "Digital%d_Enabled", j);
        ps->DefParam(bfr3, bfr, bfr2);
        snprintf(bfr, 1024, ":DIG%d:POS?", j);
        snprintf(bfr2, 1024, "Digital%d_Position", j);
        ps->DefParam(bfr3, bfr, bfr2);
    }
    ps->DefParam("LA", ":LA:THR?", "LogicDisplay");
    ps->DefParam("LA", ":LA:GROU1?", "LogicGroup1");
    ps->DefParam("LA", ":LA:GROU1:SIZ?", "LogicGroup1_Size");
    ps->DefParam("LA", ":LA:GROU2?", "LogicGroup2");
    ps->DefParam("LA", ":LA:GROU2:SIZ?", "LogicGroup2_Size");
} // Init_RigolDS1K()


void GetParams(TMC_Device * device, const map<string, string> & params)
{
    IDN_Response idn = device->Identify();
    ParamSet * ps = GetParamSet(idn.model);
    
    std::vector<std::string> mainParamCats;
    std::vector<std::string> trigParamCats;
    std::map<std::string, std::vector<std::string> > paramNames;
    std::map<std::string, std::string> niceNames;
} // GetParams()

