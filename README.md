# Decisões de projeto
## Funções `send_packet`, `recv_packet`, `send_await_packet`
A função `send_await` é definida como
```cpp
int send_await_packet(uint8_t tipo, vector<uint8_t> &buf, struct packet_t *pkt);
```
Ela implementa uma comunicação que espera uma resposta. Ou seja, ela envia a menssagem
com um `tipo` e seus dados em `buf` e espera uma resposta para colocar em `pkt`. Ela implementa
os timeouts, ou seja se a menssagem não for respondida em um certo tempo `PACKET_TIMEOUT` então
`send_await` reenvia a menssagem. Ela vai fazer isso `PACKET_RETRANSMISSION` vezes, se não for respondida então
é retornado o código de erro `TIMEOUT_ERR`. Todos os códigos de erros que podem aparecer em `send_packet`
e `recv_packet` também podem retornados aqui. Se tudo certo então é retornado `OK`.

## Serialização e Desserialização de pacotes
Uma estrutura `packet_t` foi criada para poder ser o objeto recebido e enviado
pela rede. Para poder enviar a estrutura sem usar bitfields foi necessário enviar
um buffer que contém as informações descritas no protocolo. Para não utilizar o buffer
durante o código foi decidio serializar a estrutura `packet_t` no buffer ap enviar pela
rede, e desserializar do buffer ao receber da rede.

# Testes
Durante o desenvolvimento foram cogitados e observados algumas formas de testar a
aplicação. Seguem alguns deles

## Docker (local)
Essa alternativa não funciona, não se sabe ao certo o porquê, porém os pacotes
não são detctados para recepção, ou seja o `recv/recvfrom` não funciona. Porém a terceira opção
depende do ambiente subido por essa opção.

### Como compilar e testar no docker
O trabalho utiliza um abiente docker com dois containers que contém 
uma network entre eles.

### Requisitos
- `docker`
- `docker compose`

### Rodando
Execute o docker compose para subir os dois containers
```shell
docker compose up --build -d
```

Agora entre em cada container com uma shell
```shell
docker compose exec -it cliente /bin/bash
docker compose exec -it servidor /bin/bash
```
Agora compile
```shell
[cliente]: make
[cliente]: ./exe/cliente
[servidor]: make
[servidor]: ./exe/servidor
```
 ### Recompilando
 Se voçê fez uma atualização no código não precisa subir novamente os
 containers, basta para o serviço que você mudou (`cliente` ou `servidor`)
 recompilar e rodar. A pasta `src` e o `makefile` ficam sincronizados entre
 os containers e seu diretório.

### Parando o docker compose
O docker vai continuar rodando até que vocẽ pare ele. Rodando
```shell
docker compose down
```
## Loopback/lo (local)
__Essa opção deixa de funcionar quando temos ambos **cliente** e **servidor** enviando e
recebendo pacotes. Infelizmente ainda não achei a causa do problema, apenas que muitas
coisas esquisitas ocorrem__.

Voltou a funcionar, era só eu que programei tudo errado mesmo. heehhehehehehehe.

### Teste usando interface `loopback`
Para evitar de ouvir duas vezes o pacote, fazemos uso da API `recvfrom` em
uma versão debug, que permite acesso a informação de se o pacote estava 
descendo ou subindo na interface, através do valor de `sll_pkttype`.

### Solução final
Para a solução final vamos continuar usando a API `recv`, porém deve
ter cuidado ao escolher a interface de rede sem estar na versão de debug.

## Interface de rede do Docker (local)
Essa alternativa foi encontrada tentando arrumar a alternativa 1, como ela funciona eu não sei
mas serve o propósito e funciona com ambas as máquina fazendo `send` e `recv`.

### Como realizar os testes
Para realizar os teste primeiro precisa ser retirado a verificação de `OUTGOING`
da função que verifica se o pacote é válido, isso pois os pacotes são sempre deesse tipo.

Agora é preciso subir o ambiente da alternativa 1, basta subir os containers com
```shell
docker compose up -d
```
Agora no host verifique o nome da interface de rede que o docker criou. Provavelmente
vai ser algo parecido com `dm-7500128d0ac3`.
```shell
ip a
```
Agora utilize essa interface tanto no cliente quanto no servidor e pronto funciona. COMO????
Não faço a menor ideia.

