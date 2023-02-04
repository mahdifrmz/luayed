#ifndef ANALYSIS_GENERATOR_H
#define ANALYSIS_GENERATOR_H

#include <src/generator.h>

class AnalysisGenerator : public BaseGenerator
{
private:
    string text;
    FuncTest *fn;

    void fn_stringify();
    void append(string str);
    void hex(size_t num);
    char hex_digit(size_t num);

public:
    string stringify();
};

#endif