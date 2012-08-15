
#include "cfgmap.h"
#include <stdio.h>
#include <iostream>

using namespace std;
//******************************************************************************

void EscapeStr(const string & input, string & output)
{
    string::const_iterator ch;
    for(ch = input.begin(); ch != input.end(); ++ch)
    {
        if(isprint(*ch) && *ch != '\\')
        {
            output += *ch;
        }
        else
        {
            char bfr[16];
            snprintf(bfr, 16, "\\%04X", (int)*ch);
            output += bfr;
        }
    }
} // EscapeStr()

void UnescapeStr(const string & input, string & output)
{
    string::const_iterator ch;
    ch = input.begin();
    while(ch != input.end())
    {
        if(*ch == '\\')
        {
            ++ch;
            char bfr[16];
            int j = 0;
            while(j < 4 && ch != input.end())
                bfr[j++] = *(ch++);
            bfr[j] = '\0';
            output += (char)strtol(bfr, NULL, 16);
        }
        else
        {
            output += *(ch++);
        }
    }
} // UnescapeStr()

void ReadParams(const std::string & path, std::map<std::string, std::string> & params)
{
    FILE * fin = fopen(path.c_str(), "rb");
    int ch = fgetc(fin);
    while(ch != EOF)
    {
        string key;
        while(ch != EOF && ch != ':') {
            key += (char)ch;
            ch = fgetc(fin);
        }
        
        if(ch == ':')
            ch = fgetc(fin);
        
        string encValue;
        while(ch != EOF && ch != '\n') {
            encValue += (char)ch;
            ch = fgetc(fin);
        }
        string value;
        UnescapeStr(encValue, value);
        params[key] = value;
        
        if(ch == '\n')
            ch = fgetc(fin);
    }
    fclose(fin);
} // ReadParams()

void WriteParams(const std::string & path, const std::map<std::string, std::string> & params)
{
    FILE * fout = fopen(path.c_str(), "wb");
    std::map<std::string, std::string>::const_iterator pitr;
    for(pitr = params.begin(); pitr != params.end(); ++pitr)
    {
        string escaped;
        EscapeStr(pitr->second, escaped);
        // Don't try to merge these. c_str() uses a static buffer.
        fprintf(fout, "%s:", pitr->first.c_str());
        fprintf(fout, "%s\n", escaped.c_str());
    }
    fclose(fout);
} // WriteParams()

//******************************************************************************

// g++ -DTEST_CFGMAP cfgmap.cpp -o test_cfgmap
#ifdef TEST_CFGMAP

void TestValue(const std::string & key, const std::string & value, const std::map<std::string, std::string> & params)
{
    std::map<std::string, std::string>::const_iterator pitr = params.find(key);
    if(pitr == params.end()) {
        cerr << "Test failed, key \"" << key << "\" not found in params" << endl;
        exit(EXIT_FAILURE);
    }
    if(pitr->second != value) {
        cerr << "Test failed, expected value \"" << value << "\", found \"" << pitr->second << "\"" << endl;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char * argv[])
{
    std::map<std::string, std::string> params;
    params["foo"] = "bar";
    params["Sernum"] = "51646135FA";
    params["addr"] = "192.168.3.21";
    params["complexString"] = "\\1, 2, 3,\n\\4, 5, 6";
    WriteParams("test.cfg", params);
    
    std::map<std::string, std::string> rparams;
    ReadParams("test.cfg", rparams);
    
    std::map<std::string, std::string>::const_iterator pitr;
    for(pitr = params.begin(); pitr != params.end(); ++pitr)
        TestValue(pitr->first, pitr->second, rparams);
    
    cerr << "Tests passed" << endl;
    
    return EXIT_SUCCESS;
}
#endif

