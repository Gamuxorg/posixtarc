#include <stdio.h>
#include "TarStream.h"

void outputCallback(char *data, size_t size){
    
}

int main() {
    setOutputCallback(outputCallback);
    addFile("/home/xiaoji/header.png", "header.png");
    addFile("/home/xiaoji/icon.png", "icon.png");

    return 0;
}
