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
