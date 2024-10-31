# Decisões de projeto
## Serialização e Desserialização de pacotes
Uma estrutura `packet_t` foi criada para poder ser o objeto recebido e enviado
pela rede. Para poder enviar a estrutura sem usar bitfields foi necessário enviar
um buffer que contém as informações descritas no protocolo. Para não utilizar o buffer
durante o código foi decidio serializar a estrutura `packet_t` no buffer ap enviar pela
rede, e desserializar do buffer ao receber da rede.

## Teste usando interface `loopback`
Para evitar de ouvir duas vezes o pacote, fazemos uso da API `recvfrom` em
uma versão debug, que permite acesso a informação de se o pacote estava 
descendo ou subindo na interface, através do valor de `sll_pkttype`.

## Solução final
Para a solução final vamos continuar usando a API `recv`, porém deve
ter cuidado ao escolher a interface de rede sem estar na versão de debug.

# Como compilar e testar no docker
O trabalho utiliza um abiente docker com dois containers que contém 
uma network entre eles.
## Requisitos
- `docker`
- `docker compose`

## Rodando
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
 ## Recompilando
 Se voçê fez uma atualização no código não precisa subir novamente os
 containers, basta para o serviço que você mudou (`cliente` ou `servidor`)
 recompilar e rodar. A pasta `src` e o `makefile` ficam sincronizados entre
 os containers e seu diretório.

## Parando o docker compose
O docker vai continuar rodando até que vocẽ pare ele. Rodando
```shell
docker compose down
```
