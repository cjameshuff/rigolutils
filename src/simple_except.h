
#ifndef SIMPLE_EXCEPT_H
#define SIMPLE_EXCEPT_H

#include <string>
#include <stdexcept>
#include <cstdio>
#include <stdarg.h>

class FormattedError: public std::exception {
  protected:
    std::string msg;
  public:
    FormattedError(const std::string & format, ...) {
        va_list ap;
        va_start(ap, format);
#if(1)
        char * fstr;
        vasprintf(&fstr, format.c_str(), ap);
        if(fstr != NULL) {
            msg = fstr;
            free(fstr);
	}
#else
	char fstr[1024];
	vsnprintf(fstr, 1024, format.c_str(), ap);
	msg = fstr;
#endif
        va_end(ap);
    }
    virtual ~FormattedError() throw() {}
    virtual const char* what() const throw() {return msg.c_str();}
};

#endif // SIMPLE_EXCEPT_H
