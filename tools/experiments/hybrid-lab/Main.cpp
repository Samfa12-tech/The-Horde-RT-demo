#include "Lab.h"
#include <iostream>
#include <stdexcept>
int main(int argc,char** argv) {
    try {
        const std::string output=argc>1?argv[1]:"results";
        const int width=argc>2?std::stoi(argv[2]):640;
        const int height=argc>3?std::stoi(argv[3]):360;
        const int frames=argc>4?std::stoi(argv[4]):24;
        bool validation=false,query=false;
        for(int i=5;i<argc;i++){if(std::string(argv[i])=="--validation")validation=true;else if(std::string(argv[i])=="--query-baseline")query=true;else throw std::runtime_error("Unknown flag");}
        std::cout<<RunHordeHybridLab(output,width,height,frames,validation,query)<<std::endl;
        return 0;
    } catch(const std::exception& e) {std::cerr<<e.what()<<std::endl;return 1;}
}
