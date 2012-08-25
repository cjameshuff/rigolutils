
#include "units.h"
#include <sstream>
#include <vector>
#include <map>
#include <math.h>

using namespace std;
namespace units {

map<string, int> siPrefixToExponent;// Map any form of any prefix to an exponent

map<int, string> siExponentToPrefix;// Map an exponent to preferred long form prefix
map<int, string> siExponentToSymbol;// Map an exponent to preferred short form (symbolic) prefix

vector<string> prefixNames;
vector<string> prefixSymbols;

map<string, d_double> units;

map<Dimension, string> dimToName;// Map of dimensions to unprefixed unit name
map<Dimension, string> dimToSymbol;// Map of dimensions to unprefixed unit symbol

//******************************************************************************
// std::ostream & operator<<(std::ostream & ostrm, const FmtUnit & fmt)
// {
//     if(fmt.val < 1e-9)
//         ostrm << fmt.val*1e12 << " p" << fmt.unit;
//     else if(fmt.val < 1e-6)
//         ostrm << fmt.val*1e9 << " n" << fmt.unit;
//     else if(fmt.val < 1e-3)
//         ostrm << fmt.val*1e6 << " u" << fmt.unit;
//     else if(fmt.val < 1.0)
//         ostrm << fmt.val*1e3 << " m" << fmt.unit;
//     else if(fmt.val < 1e3)
//         ostrm << fmt.val << " " << fmt.unit;
//     else if(fmt.val < 1e6)
//         ostrm << fmt.val/1e3 << " k" << fmt.unit;
//     else if(fmt.val < 1e9)
//         ostrm << fmt.val/1e6 << " M" << fmt.unit;
//     else
//         ostrm << fmt.val/1e9 << " G" << fmt.unit;
//     return ostrm;
// }

//******************************************************************************

static void MapExp(const string & pref, const string & sym, int e) {
    siPrefixToExponent[pref] = e;
    siPrefixToExponent[sym] = e;
    siExponentToPrefix[e] = pref;
    siExponentToSymbol[e] = sym;
    
    prefixNames.push_back(pref);
    prefixSymbols.push_back(sym);
}
static void MapAlias(const string & pref, int e) {
    siPrefixToExponent[pref] = e;
    prefixNames.push_back(pref);
}
static void MapSymAlias(const string & sym, int e) {
    siPrefixToExponent[sym] = e;
    prefixSymbols.push_back(sym);
}

static d_double SI_Unit(const string & name, const string & sym, const d_double & unit) {
    units[name] = unit;
    units[sym] = unit;
    dimToName[unit.Dims()] = name;
    dimToSymbol[unit.Dims()] = sym;
    
    // Register units for prefix symbol + unit symbol and prefix string + unit string.
    // Do prefix symbol + unit string as well?
    vector<string>::iterator pfx;
    for(pfx = prefixNames.begin(); pfx != prefixNames.end(); ++pfx)
        units[*pfx + name] = unit*pow(10, siPrefixToExponent[*pfx]);
    
    for(pfx = prefixSymbols.begin(); pfx != prefixSymbols.end(); ++pfx)
        units[*pfx + sym] = unit*pow(10, siPrefixToExponent[*pfx]);
    
    return unit;
} // SI_Unit()

// static void SI_Unit(const string & pref, const string & sym, const string & unit) {
// }

void InitUnits()
{
    MapExp("yotta", "Y", 24);
    MapExp("zetta", "Z", 21);
    MapExp("exa",   "E", 18);
    MapExp("peta",  "P", 15);
    MapExp("tera",  "T", 12);
    MapExp("giga",  "G", 9);
    MapExp("mega",  "M", 6);
    MapExp("kilo",  "k", 3);
    MapExp("hecto", "h", 2);
    MapExp("deca",  "da", 1);
    MapExp("deci",  "d", -1);
    MapExp("centi", "c", -2);
    MapExp("milli", "m", -3);
    MapExp("micro", "µ", -6);
    MapSymAlias("u", -6);
    MapExp("nano",  "n", -9);
    MapExp("pico",  "p", -12);
    MapExp("femto", "f", -15);
    MapExp("atto",  "a", -18);
    MapExp("zepto", "z", -21);
    MapExp("yocto", "y", -24);
    
    d_double m = SI_Unit("meter", "m", d_double(1, kLengthDim));
    d_double g = SI_Unit("gram", "g", d_double(0.001, kMassDim));
    d_double s = SI_Unit("second", "s", d_double(1, kTimeDim));
    d_double A = SI_Unit("ampere", "A", d_double(1, kCurrentDim));
    d_double L = SI_Unit("kelvin", "K", d_double(1, kTemperatureDim));
    d_double cd = SI_Unit("candela", "cd", d_double(1, kLuminosityDim));
    d_double rad = SI_Unit("radian", "rad", d_double(1));
    d_double sr = SI_Unit("steradian", "sr", d_double(1));
    
    d_double kg = d_double(1, kMassDim);
    d_double s2 = s*s;
    d_double m2 = m*m;
    
    SI_Unit("hertz", "Hz", d_double(1.0)/s);
    d_double N = SI_Unit("newton", "N", kg*m/s2);
    SI_Unit("pascal", "Pa", N/m2);
    d_double J = SI_Unit("joule", "J", N*m);
    SI_Unit("watt", "W", J/s);
    d_double C = SI_Unit("coulomb", "C", A*s);
    d_double V = SI_Unit("volt", "V", J/C);
    d_double F = SI_Unit("farad", "F", C/V);
    d_double ohm = SI_Unit("ohm", "Ω", V/A);
    SI_Unit("siemens", "S", A/V);
    d_double Wb = SI_Unit("weber", "Wb", J/A);
    d_double tes = SI_Unit("tesla", "T", Wb/m2);
    d_double H = SI_Unit("henry", "H", Wb/A);
    
    // lux, lumen, katal
    
    // TODO: work out way to handle ambiguity here
    // SI_Unit("becquerel", "Bq", d_double(1.0)/s);
    // SI_Unit("gray", "Gy", J/kg);
    // SI_Unit("sievert", "Sv", J/kg);
    
    // cerr << "kg: " << kg << endl;
    // cerr << "d_double(1, kTimeDim)): " << d_double(1, kTimeDim) << endl;
    // cerr << "s: " << s << endl;
    // cerr << "s2: " << s2 << endl;
    // cerr << "s*s: " << s*s << endl;
    // cerr << "N: " << N << endl;
    // cerr << "V: " << V << endl;
    cerr << units.size() << " units defined" << endl;
    // map<string, d_double>::iterator u;
    // for(u = units.begin(); u != units.end(); ++u)
    //     cerr << u->first << " = " << u->second << endl;
} // InitUnits()

//******************************************************************************
d_double GetUnit(const std::string & unitStr)
{
    // Break into parts separated by * and /
    // Extract exponent
    // Lookup base unit
    map<string, d_double>::iterator unt = units.find(unitStr);
    if(unt == units.end())
        throw ParseError("Could not parse unit: %s", unitStr.c_str());
    return unt->second;
} // GetUnit()

//******************************************************************************
// parse input of the form NUMBER[SPACE]PREFIXunit
// If no units are provided, scaling by 10^defaultExponent is assumed.
// May throw ParseError or DimensionError
d_double StrToD(const char * str, const char ** endptr, const d_double & reqUnit)
{
    // TODO: more robust parsing to better distinguish E in exponent from E in exa prefix
    char * crsr = NULL;
    double val = strtod(str, &crsr);
    
    if(endptr)
        *endptr = str;
    
    if(crsr == str)
        return 0;
    
    // Eat any whitespace before unit string
    while(*crsr != '\0' && isspace(*crsr))
        ++crsr;
    
    if(*crsr == '\0')
    {
        if(endptr)
            *endptr = crsr;
        
        return d_double(val, reqUnit);
    }
    else
    {
        // Followed by what should be a unit string. Allow any unit string of compatible dimensions.
        // TODO: better handle number followed by non-unit string?
        // Fail only if followed by alphabetic character?
        char * unitStrStart = crsr;
        // Find the end of the unit string (first space character or end of string)
        while(*crsr != '\0' && !isspace(*crsr))
            ++crsr;
        
        // Match the unit string to a unit
        d_double inpUnit = GetUnit(string(unitStrStart, crsr));
        if(reqUnit.Dims() != inpUnit.Dims())
            throw DimensionError("Dimensioned unit mismatch in UnitsStrToD()");
        
        if(endptr)
            *endptr = crsr;
        
        return d_double(val, inpUnit);
    }
} // StrToD()

//******************************************************************************

FmtUnit::FmtUnit():
    flags(0), outputUnitStr(), mode(kBaseSI)
{}

FmtUnit::FmtUnit(const std::string & ou, fmt_mode_t md):
    flags(0), outputUnitStr(ou), mode(md)
{}

// Select an appropriate multiple-of-3 exponent from -24 to 24 for the given value
int SelectExponent(double val) {
    val = fabs(val);
    for(int j = -21; j <= 24; j += 3)
        if(val < pow(10, j))
            return j - 3;
    return 24;
}

std::string FormatBaseSI(const d_double & val) {
    // TODO: implement printing of the form m^POWER*kg^POWER...
    // Optionally: use / instead of negative powers
    ostringstream ostrm;
    ostrm << val;
    return ostrm.str();
} // FormatBaseSI()

std::string FormatUnit(const d_double & val, const string & outputUnitStr) {
    d_double outUnit = GetUnit(outputUnitStr);
    ostringstream ostrm;
    ostrm << val.Value()/outUnit.Value() << " " << outputUnitStr;
    return ostrm.str();
} // FormatUnit()

std::string FormatRescaledUnit(const d_double & val, const string & outputUnitStr)
{
    d_double outUnit = GetUnit(outputUnitStr);
    map<Dimension, string>::iterator si = dimToSymbol.find(outUnit.Dims());
    if(si == dimToSymbol.end())
        return FormatBaseSI(val);
    
    // TODO: handle zero detection better. Go to 0 exponent if printed value is rounded to zero,
    // based on formatting options.
    int ex = 0;
    if(val.Value() != 0)
        ex = SelectExponent(val.Value()/outUnit.Value());
    
    ostringstream ostrm;
    ostrm << val.Value()/pow(10, ex)/outUnit.Value() << " " << siExponentToSymbol[ex] << si->second;
    return ostrm.str();
} // FormatRescaledUnit()

string FmtUnit::operator()(const d_double & val) const
{
    switch(mode) {
        case kUnit: return FormatUnit(val, outputUnitStr);
        case kRescaledUnit: return FormatRescaledUnit(val, outputUnitStr);
        default: return FormatBaseSI(val);
    }
}

//******************************************************************************
} // namespace units

//******************************************************************************

// g++ -DTEST_UNITS units.cpp -o test_units
#ifdef TEST_UNITS

#include <list>
#include <vector>
#include <stdarg.h>

using namespace units;

int testsFailed = 0;

#define TEST_FMT(code, expect) { \
    if(code != expect) {\
        cerr << "Test failed: " << #code << endl << "Expected \"" << expect << "\", got \"" << code << "\"" << endl; \
        ++testsFailed; \
    } \
}

int main(int argc, char * argv[])
{
    InitUnits();
    
    TEST_FMT(StrToD("1.0", NULL, GetUnit("s")),     d_double(1, kTimeDim))
    TEST_FMT(StrToD("1.0s", NULL, GetUnit("s")),    d_double(1, kTimeDim))
    TEST_FMT(StrToD("1.0 s", NULL, GetUnit("s")),   d_double(1, kTimeDim))
    TEST_FMT(StrToD("1.0 ms", NULL, GetUnit("s")),  d_double(0.001, kTimeDim))
    TEST_FMT(StrToD("1.0 µs", NULL, GetUnit("s")),  d_double(0.000001, kTimeDim))
    TEST_FMT(StrToD("1.0 mg", NULL, GetUnit("kg")),  d_double(0.000001, kMassDim))
    
    FmtUnit fmt_s("s");
    TEST_FMT(fmt_s(StrToD("10.0 Ys", NULL, GetUnit("s"))),   "10 Ys")
    TEST_FMT(fmt_s(StrToD("1.0 Ys", NULL, GetUnit("s"))),    "1 Ys")
    TEST_FMT(fmt_s(StrToD("1.0 Zs", NULL, GetUnit("s"))),    "1 Zs")
    TEST_FMT(fmt_s(StrToD("1.0 Gs", NULL, GetUnit("s"))),    "1 Gs")
    TEST_FMT(fmt_s(StrToD("1.0 s", NULL, GetUnit("s"))),     "1 s")
    TEST_FMT(fmt_s(StrToD("1.0 ms", NULL, GetUnit("s"))),    "1 ms")
    TEST_FMT(fmt_s(StrToD("1.0 us", NULL, GetUnit("s"))),    "1 µs")
    TEST_FMT(fmt_s(StrToD("1.0 µs", NULL, GetUnit("s"))),    "1 µs")
    TEST_FMT(fmt_s(StrToD("1.0 as", NULL, GetUnit("s"))),    "1 as")
    TEST_FMT(fmt_s(StrToD("1.0 zs", NULL, GetUnit("s"))),    "1 zs")
    TEST_FMT(fmt_s(StrToD("1.0 ys", NULL, GetUnit("s"))),    "1 ys")
    TEST_FMT(fmt_s(StrToD("0.1 ys", NULL, GetUnit("s"))),    "0.1 ys")
    
    TEST_FMT(fmt_s(StrToD("999.0 Ys", NULL, GetUnit("s"))),    "999 Ys")
    TEST_FMT(fmt_s(StrToD("999.0 Zs", NULL, GetUnit("s"))),    "999 Zs")
    TEST_FMT(fmt_s(StrToD("999.0 Gs", NULL, GetUnit("s"))),    "999 Gs")
    TEST_FMT(fmt_s(StrToD("999.0 s", NULL, GetUnit("s"))),     "999 s")
    TEST_FMT(fmt_s(StrToD("999.0 ms", NULL, GetUnit("s"))),    "999 ms")
    TEST_FMT(fmt_s(StrToD("999.0 us", NULL, GetUnit("s"))),    "999 µs")
    TEST_FMT(fmt_s(StrToD("999.0 µs", NULL, GetUnit("s"))),    "999 µs")
    TEST_FMT(fmt_s(StrToD("999.0 zs", NULL, GetUnit("s"))),    "999 zs")
    TEST_FMT(fmt_s(StrToD("999.0 ys", NULL, GetUnit("s"))),    "999 ys")
    
    TEST_FMT(fmt_s(StrToD("1000.0 Ys", NULL, GetUnit("s"))),    "1000 Ys")
    TEST_FMT(fmt_s(StrToD("1000.0 Zs", NULL, GetUnit("s"))),    "1 Ys")
    TEST_FMT(fmt_s(StrToD("1000.0 Gs", NULL, GetUnit("s"))),    "1 Ts")
    TEST_FMT(fmt_s(StrToD("1000.0 s", NULL, GetUnit("s"))),     "1 ks")
    TEST_FMT(fmt_s(StrToD("1000.0 ms", NULL, GetUnit("s"))),    "1 s")
    TEST_FMT(fmt_s(StrToD("1000.0 us", NULL, GetUnit("s"))),    "1 ms")
    TEST_FMT(fmt_s(StrToD("1000.0 µs", NULL, GetUnit("s"))),    "1 ms")
    TEST_FMT(fmt_s(StrToD("0.001 fs", NULL, GetUnit("s"))),     "1 as")
    // TEST_FMT(fmt_s(StrToD("1000.0 zs", NULL, GetUnit("s"))),    "1 as")// Test fails due to binary/decimal conversion
    TEST_FMT(fmt_s(StrToD("1000.0 ys", NULL, GetUnit("s"))),    "1 zs")
    
    FmtUnit fmt_V("V");
    TEST_FMT(fmt_V(StrToD("1.0 GV", NULL, GetUnit("V"))),    "1 GV")
    TEST_FMT(fmt_V(StrToD("1.0 V", NULL, GetUnit("V"))),     "1 V")
    TEST_FMT(fmt_V(StrToD("1.0 mV", NULL, GetUnit("V"))),    "1 mV")
    TEST_FMT((StrToD("1.0 GV", NULL, GetUnit("V"))/GetUnit("V")).Value(),    1e9)
    TEST_FMT((StrToD("1.0 V", NULL, GetUnit("V"))/GetUnit("V")).Value(),     1)
    TEST_FMT((StrToD("1.0 mV", NULL, GetUnit("V"))/GetUnit("V")).Value(),    0.001)
    
    
    // TEST_FMT(SelectExponent(1000.0e-21), -18)
    
    FmtUnit fmt_ms("ms", kUnit);
    TEST_FMT(fmt_ms(StrToD("1.0 s", NULL, GetUnit("s"))),    "1000 ms")
    TEST_FMT(fmt_ms(StrToD("1.0 ms", NULL, GetUnit("s"))),    "1 ms")
    TEST_FMT(fmt_ms(StrToD("1.0 µs", NULL, GetUnit("s"))),    "0.001 ms")
    
    d_double v = StrToD("13.2 V", NULL, GetUnit("V"));
    cerr << v << endl;
    cerr << fmt_V(v) << endl;
    
    cerr << testsFailed << " tests failed" << endl;
    
    return EXIT_SUCCESS;
}
#endif

