#include <stdio.h>
#include "TarStream.h"

void outputCallback(char *data, size_t size){
    
}

int main() {

    TarStream stream;

    stream.setOutputCallback(outputCallback);
    stream.addFile("/home/maicss/header.png", "header.png");
    stream.addFile("/home/maicss/icon.png", "icon.png");

    return 0;
}
