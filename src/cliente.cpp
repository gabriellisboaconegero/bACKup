#include <iostream>
#include "socket.h"
using namespace std;

int main(int argc, char **argv) {
    char *interface = "lo";
    cout << "Inicicalizando cliente (" << interface << ")" << endl;
    cliente(interface);
}
