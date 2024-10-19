#include <iostream>
#include "socket.h"
using namespace std;

int main(int argc, char **argv) {
    cout << "Inicicalizando servidor" << endl;
    servidor("eth0");
}
