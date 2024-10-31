#include <iostream>
#include "socket.h"
using namespace std;

void cliente(char *interface) {
    struct connection_t conn;
    if (!conn.connect(interface)) {
        cout << "[ERRO]: Erro ao criar conexão com interface (" << interface << ")" << endl;
        cout << "[ERRO]: " << strerror(errno) << endl;
        exit(1);
    }

    string msg;
    while(1) {
        cout << "Escreva uma menssagem (q para sair): ";
        cin >> msg;
        if (msg == "q")
            break;
        if (conn.send_packet(1, msg) < 0) {
            cout << "[ERROR]: " << strerror(errno) << endl;
        }
    }
    cout << "Saindo do cliente" << endl;
}

int main(int argc, char **argv) {
    char *interface = "lo";
    cout << "Inicicalizando cliente (" << interface << ")" << endl;
    cliente(interface);
}
