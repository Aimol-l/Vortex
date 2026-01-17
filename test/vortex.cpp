#include <iostream>
#include "Application.h"

int main(int argc, char const *argv[]){

    auto app = std::make_unique<Application>();

    app->run();
    
    return 0;
}
