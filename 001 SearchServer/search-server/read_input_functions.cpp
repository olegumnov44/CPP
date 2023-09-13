#include <iostream>
#include <string>

#include "read_input_functions.h"

using namespace std;


std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result;
    std::cin >> result;
    ReadLine();         //функции ReadLine(), ReadLineWithNumber() должны быть в одном файле
    return result;
}
