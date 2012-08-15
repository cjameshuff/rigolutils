
#ifndef RIGOL_DS1K_H
#define RIGOL_DS1K_H

#include <vector>
#include <string>
#include <map>

#include <stdio.h>

#include "freetmc.h"

//******************************************************************************

// TODO: move this stuff to separate paramset files someday
struct ParamSet {
    std::vector<std::string> mainParamCats;
    std::vector<std::string> trigParamCats;
    std::map<std::string, std::vector<std::string> > paramNames;
    std::map<std::string, std::string> niceNames;
    
    void DefParam(const std::string & cat, const std::string & pname, const std::string & nicename);
};

ParamSet * GetParamSet(const std::string & devName);


void Init_RigolDS1K();

void GetParams(TMC_Device * device, std::map<std::string, std::string> & params);
void SetParams(TMC_Device * device, const std::map<std::string, std::string> & params);


//******************************************************************************
#endif // RIGOL_DS1K_H
