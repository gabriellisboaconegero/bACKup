# Decisões de projeto
## Organização dos arquivos
Existem 4 arquivos:
- `socket.cpp`
- `servidor.cpp`
- `cliente.cpp`
- `utils.h`

As funções e structs relacionadas ao funcionamento da rede estão em `socket.cpp`.
As funções especificas de cliente e servidor stão em seus respectivos arquivos
e implementam a sequencia de operações que cliente e servidor devem fazer. O arquivo
`utils.cpp` implementa funções que são de uso comum para servidor e cliente.

## Tipos de Erros
Alguns erros podem acontecer durante a comunicação entre cliente e
servidor. Para avisar o outro lado qual tipo de erro que ocorreu, os tipos
de erros que podem ir no pacote de tipo `PKT_ERRO`:
- `NO_FILE_ERRO`: Arquvio não econtrado.
- `NO_FILE_ACCESS_ERRO`: Sem permissão de acesso ao arquivo.
- `NO_DISK_SPACE_ERRO`: Sem espaço no disco para receber arquivo.
- `READING_FILE_ERRO`: Algum erro ocorreu durante a leitura do arquivo.

## Tipo `packet_t`
Esse tipo de dados guarda as informações sobre um pacote. Além de conter
as operações de serializar e deserializar um vetor de bytes.

## Tipo `connection_t`
Esse tipo armazena as informações sobre a conexão e o estado local de um usuário da
rede. Como por exemplo qual o próximo número de sequência que vai ser usado para
enviar um pacote, ultimo pacote lido e/ou enviado, descritor do socket, etc.

Além de manter o estado da rede o tipo possui as funções de envio e recebimento de
pacotes. Os pacotes que forem enviados corretamentes são salvos, também é possivel
salvar os pacotes que forem recebidos.

### A função `send_await_packet`
A função fica reenviando o pacote `s_pkt` até que receba um pacote que tenha algum
tipo em `esperados` senão ele é recusado, ela faz o reenvio `PACKET_RETRASMISSION_ROUNDS`
vezes. O timeout aumenta a cada pacote recusado, seguindo a sequência de fibonacci.
A função normalmente não envia nenhum outro pacote que não seja o passado pelo
parâmetro, porém se for ativado a flag de `nacks` a função ao receber um pacote
corrompido envia um `PKT_NACK` e continua a operação normalmente.

## Timeout
O timeout funciona de  uma forma peculiar, o funcionamento dele é o seguinte:
    1. Um timeout de `PACKET_TIMEOUT_INTERVAL` é colocado no socket da conexão.
    2. Ao receber um pacote é feita a verificação se o pacote é do protocolo.
        2.1 Se pacote não for do protocolo verifica se o tempo
        passado do incio do recebimento até o momento ja passou de
        `PACKET_TIMEOUT_INTERVAL`.
        2.2 Se o tempo ja passou retorna que deu timeout.
    3. Se for do protocolo verifica se foi corrompido
        3.1 Se for corrompido devolve que o pacote foi corrompido.
        3.2 Se não for corrompido devolve o tipo de pacote recebido.

Um comportamento que acontece devido a forma que foi implementado é que se
quem estiver recebendo receber uma sequência de `PACKET_RETRASMISSION_ROUNDS`
seguidos vai ser retornado que não foi possível receber uma resposta pois
deu tiemout, porém não passou o tempo definido para timeout. Isso ocorre
pois a retransmissão não se importa com o tempo, apenas o tiemout para um
pacote e não uma sequência.

## Sequência
Como já dito é armazenado o último pacote recebido, se o novo pacote
recebido tiver o mesmo número de sequência que `last_recv_pkt` então o
`last_send_pkt` é enviado.

## Escape dos bytes 0x88 e 0x81
Para evitar que a placa de rede "consuma" alguns bytes que indicam o marcador de início
do protocolo vlan foi colocado um byte 0xff depois de todo byte 0x88 ou 0x81. Essa inserção
ocorre antes de enviar o pacote para a placa de rede e é feito o processo inverso logo
depois de receber a mensagem da placa de rede.

## Troca de funcionalidade na operação de restaura
Ao fazer a operação de restaurar o cliente e servidor precisam inverter os papeis de
receptor e transmissor. Essa inversão ocorre quando o cliente recebe o pacote de tipo
`PKT_OK_TAM`, então ele começa a fazer papel de receptor e o servidor de transmissor.
Porém o cliente ao enviar o nome do arquivo precisa ficar fazendo tiemout para reenviar
o nome se necessario, entretanto o servidor pode ja ter começado a enviar o pacote de
`PKT_OK_TAM`, então ambos funcionam em um estado sobresposto que fazem timeout e também
podem enviar nacks. Nesse estado ambos fazem retransmissão e enviam nack em pacotes
corrompidos. O único problema conhecido é se após o cliente receber `PKT_OK_TAM` ele
enviar um `PKT_ERRO` e esse pacote for perdido. O servidor está em estado transmissor
e o cliente parou de escutar pois voltou para a tela de usuário, de escolher a
operação.

## Limpeza do buffer de recebimento
Para evitar que o cliente receba mensagens antigas que ficaram no buffer da placa de
rede, uma limpeza é feita antes de toda operação. Ela joga fora todas as mensagens que estiverem no buffer e só depois começa a operação.

## CRC da operação verifica
A operação de verificar utiliza uma implementação do CRC32, não utiliza nenhuma lib ou
biblioteca externa.

# Testes
Durante o desenvolvimento foram cogitados e observados algumas formas de
testar a aplicação. Seguem alguns deles

## AntiX
Teste utilizando o sistema [AntiX](https://antixlinux.com/). Por ser pequeno
então da para rodar em memória RAM e usar apenas um pendrive para isso.

### Instalando AntiX
Vá para a página principal https://antixlinux.com/ e siga o tutorial de como baixar, é simples.

Para cada máquina é preciso configurar o SO para que ele seja rodado em RAM. Depois fazer
continuar com a instalação do projeto.

### Bootando
Para o SO rodar em RAM é preciso habilitar a flag de `toram`. Para isso
faça o boot do AntiX e na primeira tela de boot vai ter a opção de clicar
`f4` para ativar algumas opções. Ativa a opção `toram` e siga em frente.

Após seguir em frente entre como `root` onde a senha também é `root`.

### Rodando o projeto
Em teoria é para o SO ter instalado alguns pré-requisitos:
- `openssh-client`
- `g++`
- `build-essential`

Se não tiver, verifique como instalar na hora e/ou tornar persistente essas alterações.

1. Clone o projeto da macalan (e reze para que esteja atualizado).
```bash
scp -r glc22@macalan.c3sl.ufpr.br:bACKup/ .
```
2. Compile o projeto
```bash
make # make debug para compilar com os logs
```
3. Execute em máquina diferentes os códigos
```bash
./exe/cliente
./exe/servidor
```

### Arrumando dependências na hora
Se as dependências não tiverem instaladas então vai ser preciso seguir esse passo a passo.
1. Instalar `openssh-client`. Para poder clonar o projeto da macalan. Antes de instalar é preciso atualizar a lista de repositórios do Debian
```bash
apt update
apt install openssh-client
```
2. Instalar as dependências para compilar o projeto. Essa etapa pode demorar alguns minutos, então já avise o professor ou vá instalando antes.
```bash
apt install g++ build-essential
```

Agora basta seguir as instruções anteriores de como rodar o projeto.

## Docker (local)
Utilizando uma rede criada pelo docker (`macvlan`) para testar.
Escolher a interface que o docker criou na hora de começar. Compile normalmente
o projeto e quando executar apenas use a network do docker ao invés do loopback.


### Rodando
Execute o docker compose para subir os dois containers e compile o projeto
```shell
docker compose up --build -d
make
```

Abra dois terminais e em um execute o cliente e no outro o servidor.
```shell
sudo ./exe/cliente
sudo ./exe/servidor
```
Agora escolha a interface de rede que o docker criou (provavelmente começa com
`dm-`).

