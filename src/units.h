
#ifndef UNITS_H
#define UNITS_H

#include <iostream>
#include <map>
#include "simple_except.h"

namespace units {
//******************************************************************************

extern std::map<std::string, int> siPrefixToExponent;
extern std::map<int, std::string> siExponentToPrefix;
extern std::map<int, std::string> siExponentToLongPrefix;
extern std::map<int, std::string> siExponentToSymbol;

//******************************************************************************

FORMATTED_ERROR_CLASS(DimensionError, std::exception)

FORMATTED_ERROR_CLASS(ParseError, std::exception)

enum dim_t {
    kLengthDim,
    kCurrentDim,
    kMassDim,
    kTimeDim,
    kTemperatureDim,
    kLuminosityDim,
    kNumDims
};

class Dimension {
    int16_t dims[kNumDims];
    
  public:
    Dimension() {}
    explicit Dimension(dim_t dim) {MakeDimensionless(); dims[dim] = 1;}
    Dimension(const Dimension & rhs) {
        for(int j = 0; j < kNumDims; ++j)
            dims[j] = rhs.dims[j];
    }
    
    void MakeDimensionless() {
        for(int j = 0; j < kNumDims; ++j)
            dims[j] = 0;
    }
    bool IsDimensionless() const {
        for(int j = 0; j < kNumDims; ++j)
            if(dims[j] != 0)
                return false;
        return true;
    }
    
    Dimension & operator=(const Dimension & rhs) {
        for(int j = 0; j < kNumDims; ++j)
            dims[j] = rhs.dims[j];
        return *this;
    }
    
    Dimension operator*(const Dimension & rhs) const {
        Dimension result;
        for(int j = 0; j < kNumDims; ++j)
            result.dims[j] = dims[j] + rhs.dims[j];
        return result;
    }
    
    Dimension operator/(const Dimension & rhs) const {
        Dimension result;
        for(int j = 0; j < kNumDims; ++j)
            result.dims[j] = dims[j] - rhs.dims[j];
        return result;
    }
    
    bool operator==(const Dimension & rhs) const {
        for(int j = 0; j < kNumDims; ++j)
            if(dims[j] != rhs.dims[j])
                return false;
        return true;
    }
    
    bool operator!=(const Dimension & rhs) const {
        for(int j = 0; j < kNumDims; ++j)
            if(dims[j] != rhs.dims[j])
                return true;
        return false;
    }
    
    // An arbitrary ordering defined for the map containers
    bool operator<(const Dimension & rhs) const {
        for(int j = 0; j < kNumDims; ++j)
            if(dims[j] != rhs.dims[j])
                return (dims[j] < rhs.dims[j]);
        return false;
    }
    
    friend std::ostream & operator<<(std::ostream & ostrm, const Dimension & rhs) {
        ostrm << "<" << rhs.dims[0];
        for(int j = 1; j < kNumDims; ++j)
            ostrm << "," << rhs.dims[j];
        ostrm << ">";
        return ostrm;
    }
};

template<typename T>
class Dimensioned {
    Dimension dims;
    T value;
    
    Dimensioned(const T & val, const Dimension & d): dims(d), value(val) {}
    
  public:
    Dimensioned(const T & val = 0): value(val) {dims.MakeDimensionless();}
    Dimensioned(const T & val, dim_t dim): dims(dim), value(val) {}
    Dimensioned(const T & val, const Dimensioned<T> & rhs): dims(rhs.dims), value(val*rhs.value) {}
    Dimensioned(const Dimensioned<T> & rhs): dims(rhs.dims), value(rhs.value) {}
    
    const Dimension & Dims() const {return dims;}
    const T & Value() const {return value;}
    bool IsDimensionless() const {return dims.IsDimensionless();}
    void CheckDims(const Dimensioned<T> & rhs) const {
        if(dims != rhs.dims)
            throw DimensionError("Dimensioned unit mismatch");
    }
    
    Dimensioned<T> & operator=(const Dimensioned<T> & rhs) {
        dims = rhs.dims;
        value = rhs.value;
        return *this;
    }
    
    Dimensioned<T> operator+(const Dimensioned<T> & rhs) const {CheckDims(rhs); return Dimensioned(value + rhs.value, dims);}
    Dimensioned<T> operator-(const Dimensioned<T> & rhs) const {CheckDims(rhs); return Dimensioned(value - rhs.value, dims);}
    Dimensioned<T> operator*(const Dimensioned<T> & rhs) const {return Dimensioned(value*rhs.value, dims*rhs.dims);}
    Dimensioned<T> operator/(const Dimensioned<T> & rhs) const {return Dimensioned(value/rhs.value, dims/rhs.dims);}
    
    bool operator==(const Dimensioned<T> & rhs) const {CheckDims(rhs); return value == rhs.value;}
    bool operator!=(const Dimensioned<T> & rhs) const {CheckDims(rhs); return value != rhs.value;}
    bool operator>(const Dimensioned<T> & rhs) const {CheckDims(rhs); return value > rhs.value;}
    bool operator<(const Dimensioned<T> & rhs) const {CheckDims(rhs); return value < rhs.value;}
    bool operator>=(const Dimensioned<T> & rhs) const {CheckDims(rhs); return value >= rhs.value;}
    bool operator<=(const Dimensioned<T> & rhs) const {CheckDims(rhs); return value <= rhs.value;}
    
    friend std::ostream & operator<<(std::ostream & ostrm, const Dimensioned<T> & rhs) {
        ostrm << rhs.value << ":" << rhs.dims;
        return ostrm;
    }
};

typedef Dimensioned<double> d_double;

const uint32_t long_names = (1 << 0); // Use names instead of symbols
enum fmt_mode_t {
    kBaseSI,      //  base SI units
    kBestMatch,   //  best match from unit set
    kUnit,        //  specified unit, no rescaling
    kRescaledUnit //  specified unit with rescaling
};

class FmtUnit {
    uint32_t flags;
    std::string outputUnitStr;
    fmt_mode_t mode;
  public:
    FmtUnit();
    FmtUnit(const std::string & ou, fmt_mode_t md = kRescaledUnit);
    
    std::string operator()(const d_double & val) const;
};

//******************************************************************************

// struct FmtUnit {
//     double val;
//     std::string unit;
//     FmtUnit(double v, const std::string & u): val(v), unit(u) {}
//     friend std::ostream & operator<<(std::ostream & ostrm, const FmtUnit & fmt);
// };
// 
// struct FmtSec: FmtUnit {FmtSec(double v): FmtUnit(v, "s") {}};
// struct FmtHz: FmtUnit {FmtHz(double v): FmtUnit(v, "Hz") {}};
// struct FmtV: FmtUnit {FmtV(double v): FmtUnit(v, "V") {}};
// struct FmtA: FmtUnit {FmtA(double v): FmtUnit(v, "A") {}};
// struct FmtOhms: FmtUnit {FmtOhms(double v): FmtUnit(v, "ohm") {}};
// struct FmtF: FmtUnit {FmtF(double v): FmtUnit(v, "F") {}};
// struct FmtH: FmtUnit {FmtH(double v): FmtUnit(v, "H") {}};

void InitUnits();

d_double StrToD(const char * str, const char ** endptr, const d_double & reqUnit);

d_double GetUnit(const std::string & unitStr);

//******************************************************************************
} // namespace units
#endif // UNITS_H
