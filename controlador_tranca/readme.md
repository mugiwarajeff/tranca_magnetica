## 💻 `controlador_tranca.ino` | Código do Microcontrolador (Arduino UNO)

Este arquivo (`controlador_tranca.ino`) contém o código-fonte embarcado (sketch) para o microcontrolador **Arduino UNO** que implementa a **Instância 1** da Arquitetura de Controle de Acesso. Ele é responsável por gerenciar os periféricos (teclado, tranca), o armazenamento local (EEPROM/SD Card) e a comunicação TCP/IP com o Servidor de Agendamentos.

---

### 1. ⚙️ Hardware Necessário

O código foi desenvolvido para a seguinte configuração de hardware (Instância 1 - Arduino UNO):

* **Microcontrolador:** Arduino UNO.
* **Comunicação em Rede:** Shield Ethernet W5100.
* **Periféricos de Entrada:** Teclado Numérico (Keypad).
* **Tranca:** Tranca Eletrônica (ex: HDL).
* **Temporização:** Módulo RTC (DS3231/DS1307).
* **Saída/Feedback:** Display LCD 16x2.
* **Armazenamento Local:** EEPROM interna e/ou Módulo SD Card.

---

### 2. ⬇️ Bibliotecas (Libraries)

Para compilar e executar o código com sucesso, você precisará instalar as seguintes bibliotecas através do **Gerenciador de Bibliotecas da IDE do Arduino** (`Sketch > Incluir Biblioteca > Gerenciar Bibliotecas...`):

| Biblioteca | Header no Código | Finalidade |
| :--- | :--- | :--- |
| **Ethernet** | `<Ethernet.h>` | Comunicação TCP/IP com o servidor. |
| **Keypad** | `<Keypad.h>` | Leitura das teclas do teclado numérico. |
| **RTClib** | `<RTClib.h>` | Comunicação com o Módulo RTC (DS3231/DS1307) para sincronização de horário. |
| **SD** | `<SD.h>` | Gerenciamento do armazenamento no Cartão SD (se `SDCARD_ON` for definido). |
| **SPI** | `<SPI.h>` | Interface de comunicação usada pelo Ethernet Shield e Módulo SD. |
| **Wire** | `<Wire.h>` | Interface I2C usada pelo Módulo RTC. |

**Passos para Instalação:**

1.  Abra a IDE do Arduino.
2.  Vá em `Sketch` > `Incluir Biblioteca` > `Gerenciar Bibliotecas...`.
3.  Busque por cada nome (ex: "Keypad", "RTClib") e clique em **Instalar**.

---

### 3. 🛠️ Configuração Inicial (Ajuste de IPs)

Antes de compilar, você deve configurar o endereço IP da rede e o IP do servidor (o computador rodando `main.py`).

No início do arquivo `controlador_tranca.ino`, localize e ajuste as variáveis de configuração de rede (elas geralmente estão no `setup()` ou definidas globalmente):

1.  **MAC Address (Endereço Físico):** Defina um MAC address único para o seu Ethernet Shield.

    ```cpp
    byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
    ```

2.  **IP do Arduino (Endereço do Cliente):** Defina um IP estático para o seu Arduino na rede local.

    ```cpp
    IPAddress ip(192, 168, 0, 150); // Exemplo: IP do Arduino na rede
    ```

3.  **IP do Servidor (Backend):** **AJUSTE CRÍTICO!** Defina o endereço IP da máquina que está rodando o script `main.py` (o Servidor de Agendamentos).

    ```cpp
    IPAddress server(192, 168, 0, 103); // IP do Servidor TCP (main.py)
    const int port = 5000;              // Porta do Servidor
    ```

---

### 4. ⏫ Compilação e Uso

1.  **Abra o Código:** Abra o arquivo `controlador_tranca.ino` na IDE do Arduino.
2.  **Selecione a Placa:** Em `Ferramentas` > `Placa`, selecione **Arduino Uno**.
3.  **Selecione a Porta:** Em `Ferramentas` > `Porta`, selecione a porta serial correta à qual o Arduino está conectado.
4.  **Conecte o Hardware:** Certifique-se de que o Ethernet Shield, o RTC, o Teclado e a Tranca estejam conectados ao Arduino UNO conforme o seu esquema de circuito.
5.  **Compile e Carregue:** Clique no botão **Carregar** (a seta para a direita) para compilar o código e enviá-lo para a placa.

#### 📝 Operação

* O Arduino se conectará ao Servidor (`main.py` rodando) para receber a lista de permissões e armazená-las.
* Os usuários interagem com o teclado numérico, e a validação é feita localmente, permitindo a **operação offline**.