#include "MicroBit.h"

MicroBit uBit;

int main() {
    uBit.init();

    while (true) {
        uBit.display.print("Hello world!");
        uBit.sleep(1000);
    }
}
