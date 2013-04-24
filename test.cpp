#include <iostream>
#include <fstream>
#include "cmarc.hpp"


using namespace std;

int main() {
    string filename = "uco-vufind.mrc";
    ifstream ist;
    ist.open(filename.c_str());

    Marc::Iso2709Reader reader = Marc::Iso2709Reader(&ist);
    Marc::XmlWriter writer = Marc::XmlWriter(&cout);
    Marc::Record* record;
    while (reader >>record)
        writer <<record;
    
}

