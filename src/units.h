
#ifndef UNITS_H
#define UNITS_H




struct FmtUnit {
    double val;
    std::string unit;
    FmtUnit(double v, const std::string & u): val(v), unit(u) {}
    friend std::ostream & operator<<(std::ostream & ostrm, const FmtUnit & fmt) {
        if(fmt.val < 1e-9)
            ostrm << fmt.val*1e12 << " p" << fmt.unit;
        else if(fmt.val < 1e-6)
            ostrm << fmt.val*1e9 << " n" << fmt.unit;
        else if(fmt.val < 1e-3)
            ostrm << fmt.val*1e6 << " u" << fmt.unit;
        else if(fmt.val < 1.0)
            ostrm << fmt.val*1e3 << " m" << fmt.unit;
        else if(fmt.val < 1e3)
            ostrm << fmt.val << " " << fmt.unit;
        else if(fmt.val < 1e6)
            ostrm << fmt.val/1e3 << " k" << fmt.unit;
        else if(fmt.val < 1e9)
            ostrm << fmt.val/1e6 << " M" << fmt.unit;
        else
            ostrm << fmt.val/1e9 << " G" << fmt.unit;
        return ostrm;
    }
};

struct FmtSec: FmtUnit {FmtSec(double v): FmtUnit(v, "s") {}};
struct FmtHz: FmtUnit {FmtHz(double v): FmtUnit(v, "Hz") {}};
struct FmtV: FmtUnit {FmtV(double v): FmtUnit(v, "V") {}};
struct FmtA: FmtUnit {FmtA(double v): FmtUnit(v, "A") {}};
struct FmtOhms: FmtUnit {FmtOhms(double v): FmtUnit(v, "ohm") {}};
struct FmtF: FmtUnit {FmtF(double v): FmtUnit(v, "F") {}};
struct FmtH: FmtUnit {FmtH(double v): FmtUnit(v, "H") {}};


#endif // UNITS_H
