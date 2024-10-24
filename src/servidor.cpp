#include "socket.h"
using namespace std;

void servidor(char *interface) {
    int socket = cria_raw_socket(interface);
    if (socket == -1){
        cout << "[ERRO]: Algo de errado com socket" << endl;
        exit(1);
    }
    struct connection_t conn(socket, MAX_MSG_LEN);

    while(1) {
        if (!conn.recv()){
            cout << "[ERRO]: Erro ao receber do socket: " << conn.erro2string();
            continue;
        }

        if (!conn.valida_packet())
            continue;

        vector<char> pck = conn.packet;
        cout << "MENSSAGEM RECEBIDA (size: " << pck.size() << "): ";
        for (int i = 0; i < pck.size(); i++)
            cout << pck[i];
        cout << endl;
    }
    cout << "Saindo do servidor" << endl;
}

int main(int argc, char **argv) {
    char *interface = "lo";
    cout << "Inicicalizando servidor (" << interface << ")" << endl;
    servidor(interface);
}
