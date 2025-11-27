Servidor e Simulador de Automação para Controle de Acesso (Tranca Magnética)
Este projeto contém um simulador em Python do sistema de controle de acesso desenvolvido para o Trabalho de Conclusão de Curso (TCC) "Uma Arquitetura para Automação de Controle de Acesso a Ambientes Protegidos".

O simulador é composto por dois módulos principais:

Servidor (main.py): Simula o Backend (Sistema de Agendamento) que gera e envia permissões via TCP/IP.

Cliente/Simulador Arduino (arduino_simulator.py): Simula o microcontrolador Arduino, que recebe as permissões e as armazena localmente (representado pelo arquivo permissions.txt).

1. 🏗️ Arquitetura do Sistema
A solução proposta se baseia na integração de hardware embarcado com um sistema de agendamento remoto para garantir a validação de acesso. O simulador recria o fluxo de comunicação para o envio de permissões offline.

2. 📋 Pré-requisitos
Para rodar o simulador, você precisa ter instalado:

Python 3.x

Não são necessárias bibliotecas externas além das nativas do Python (socket, threading, time, json, itertools, datetime).

3. 📥 Como Baixar
Você pode clonar este repositório do GitHub usando o seguinte comando:

Bash

git clone [https://github.com/mugiwarajeff/tranca_magnetica.git]
cd [cliente_tcp_teste]
4. ⚙️ Configuração (Ajuste de IPs)
É crucial ajustar os endereços IP para que o Servidor e o Cliente possam se comunicar corretamente em sua rede local.

Servidor (main.py)
No arquivo main.py, ajuste a variável HOST para o endereço IP da máquina onde o servidor será executado. Geralmente, este deve ser o IP da sua máquina.

Python

HOST = "192.168.0.103"  # <-- AJUSTE AQUI: IP da máquina que roda o Servidor
PORT = 5000
Cliente/Simulador Arduino (arduino_simulator.py)
No arquivo arduino_simulator.py, ajuste a variável SERVER_HOST para o mesmo endereço IP que você definiu no main.py.

Python

SERVER_HOST = '192.168.56.1'  # <-- AJUSTE AQUI: Deve ser o IP do Servidor (main.py)
SERVER_PORT = 5000 
5. 🚀 Como Usar a Solução
Execute os módulos na ordem abaixo:

Passo 1: Iniciar o Servidor (Backend)
Inicie o servidor Python. Ele começará a aguardar conexões na porta definida. O servidor só tentará enviar agendamentos dentro do horário configurado (START_HOUR e END_HOUR no main.py).

Bash

python main.py
Saída esperada: Servidor TCP aguardando conexões na porta 5000...

Passo 2: Iniciar o Cliente (Simulador Arduino)
Em outro terminal, inicie o simulador do Arduino. Ele tentará se conectar ao servidor e aguardar o envio dos dados de permissão.

Bash

python arduino_simulator.py
Saída esperada: Conectando ao servidor [IP]:5000...

📝 Fluxo de Comunicação:
O servidor envia a mensagem <CLEAN>.

O servidor inicia o envio de permissões, encapsulando cada permissão em <START>...<END>.

O Cliente recebe os dados, salva cada linha no arquivo permissions.txt e envia um <ACK> (Acknowledge) de volta ao servidor, confirmando o recebimento.

O servidor finaliza o envio com <ENDTX>.

O arquivo permissions.txt será atualizado com os dados de agendamento recebidos pelo simulador.