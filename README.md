# 🚧 Pedágio Inteligente

Sistema de pedágio com ESP32, RFID, servo motor e dashboard web em tempo real.

---

## O que este projeto faz

Ao aproximar uma tag RFID do leitor, o sistema identifica o veículo, verifica o saldo, abre a cancela automaticamente e atualiza o dashboard ao vivo no navegador — mostrando fluxo de veículos, tempo de espera estimado e status da cancela.

---

## Arquivos do projeto

```
pedagio-inteligente/
├── pedagio_esp32.ino        ← código do ESP32 (Arduino IDE)
└── pedagio-inteligente.html ← dashboard web (abrir no navegador)
```

---

## O que você vai precisar

### Hardware

| Componente | Quantidade |
|---|---|
| ESP32 (qualquer modelo com Wi-Fi) | 1 |
| Leitor RFID MFRC522 | 1 |
| Tags RFID (cartões ou chaveiros) | 2 ou mais |
| Servo motor SG90 | 1 |
| LED verde | 1 |
| LED vermelho | 1 |
| Resistores 220Ω | 2 |
| Protoboard e jumpers | — |
| Cabo USB para o ESP32 | 1 |

### Software

- [Arduino IDE](https://www.arduino.cc/en/software) — versão 2.x recomendada
- Navegador web qualquer (Chrome, Firefox, Edge)

---

## Passo 1 — Instalar o Arduino IDE e configurar o ESP32

> Se você já usa Arduino IDE com ESP32, pule para o Passo 2.

1. Baixe e instale o [Arduino IDE](https://www.arduino.cc/en/software).
2. Abra o Arduino IDE e vá em: **File → Preferences**.
3. No campo **"Additional boards manager URLs"**, cole o endereço abaixo e clique em OK:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
4. Vá em: **Tools → Board → Boards Manager**.
5. Pesquise por `esp32`, localize **"esp32 by Espressif Systems"** e clique em **Install**.
6. Após instalar, vá em **Tools → Board** e selecione o seu modelo de ESP32.
   - Modelo mais comum: **"ESP32 Dev Module"**

---

## Passo 2 — Instalar as bibliotecas necessárias

No Arduino IDE, vá em **Tools → Manage Libraries** e instale uma por uma:

| Biblioteca | O que faz |
|---|---|
| `MFRC522` | Controla o leitor RFID |
| `ESP32Servo` | Controla o servo motor no ESP32 |

> As bibliotecas `WiFi` e `WebServer` já vêm instaladas com o pacote ESP32.

---

## Passo 3 — Montar o circuito

Conecte os componentes ao ESP32 conforme a tabela abaixo.

### RFID MFRC522

| Pino do MFRC522 | Pino do ESP32 |
|---|---|
| SDA (SS) | GPIO 5 |
| SCK | GPIO 18 |
| MOSI | GPIO 23 |
| MISO | GPIO 19 |
| RST | GPIO 22 |
| GND | GND |
| 3.3V | 3.3V |

> ⚠️ Use **3.3V**, não 5V. O MFRC522 não suporta 5V e pode ser danificado.

### Servo Motor

| Fio do servo | Pino do ESP32 |
|---|---|
| Sinal (laranja/amarelo) | GPIO 13 |
| VCC (vermelho) | 5V |
| GND (marrom/preto) | GND |

### LEDs

| LED | Resistor 220Ω | Pino do ESP32 |
|---|---|---|
| Verde | Em série | GPIO 26 |
| Vermelho | Em série | GPIO 27 |

> Conecte o lado positivo (perna maior) do LED ao resistor → depois ao GPIO. O lado negativo vai ao GND.

---

## Passo 4 — Configurar o código do ESP32

Abra o arquivo `pedagio_esp32.ino` no Arduino IDE.

### 4.1 — Coloque o nome e senha do seu Wi-Fi

Localize as linhas abaixo (estão no início do arquivo):

```cpp
const char* ssid = "SEU_WIFI";
const char* password = "SUA_SENHA";
```

Substitua pelos dados da sua rede. Exemplo:

```cpp
const char* ssid = "MinhaRedeWiFi";
const char* password = "minha_senha_123";
```

> ⚠️ O ESP32 só conecta em redes **2.4 GHz**. Redes 5 GHz não funcionam.

---

### 4.2 — Cadastrar suas tags RFID

Cada tag tem um código único. Você precisa descobrir o código das suas tags e cadastrá-las no sistema.

**Como descobrir o código da sua tag:**

1. Carregue o código no ESP32 (Passo 6).
2. Abra o **Serial Monitor** no Arduino IDE (**Tools → Serial Monitor**).
3. Configure a velocidade para **115200 baud**.
4. Aproxime a tag do leitor.
5. O código aparecerá no monitor. Exemplo: `a3f21b09`

**Como cadastrar a tag no código:**

Localize este trecho no final do arquivo:

```cpp
if      (conteudo == "c20199ee") processarPagamento("FXI2B58", saldo1);
else if (conteudo == "95d67e05") processarPagamento("QWP7D24", saldo2);
else if (conteudo == "914d0ea4") processarPagamento("RLA9E75", saldo3);
```

Substitua os códigos pelos da suas tags e escolha um nome para cada usuário. Exemplo:

```cpp
if      (conteudo == "a3f21b09") processarPagamento("Carlos", saldo1);
else if (conteudo == "7c4e90d1") processarPagamento("Ana",    saldo2);
```

> O nome entre aspas (`"Carlos"`, `"Ana"`) é apenas para identificação no histórico. Pode ser qualquer texto.

---

### 4.3 — Definir o saldo inicial de cada tag

Localize as variáveis de saldo:

```cpp
float saldo1 = 1000;
float saldo2 = 5;
float saldo3 = 200;
```

- `saldo1` corresponde à tag da linha `processarPagamento(..., saldo1)`
- `saldo2` à da linha com `saldo2`, e assim por diante

Exemplo: se quiser que o primeiro usuário tenha R$ 50,00 e o segundo não tenha saldo suficiente (para testar o status **PENDENTE**):

```cpp
float saldo1 = 50;
float saldo2 = 3;   // menor que o valor do pedágio (R$ 8,00)
```

---

### 4.4 — Alterar o valor do pedágio (opcional)

```cpp
float pedagio = 8.0;
```

Mude para o valor que quiser. Exemplo para R$ 5,50:

```cpp
float pedagio = 5.5;
```

---

### 4.5 — Ajustar os ângulos da cancela (se necessário)

```cpp
const int FECHADA = 90;
const int ABERTA  = 0;
```

Dependendo de como o servo está posicionado fisicamente, pode ser necessário inverter ou ajustar os ângulos. Se a cancela abrir ao contrário, troque os valores:

```cpp
const int FECHADA = 0;
const int ABERTA  = 90;
```

Teste valores entre `0` e `180` até encontrar o movimento correto para o seu modelo físico.

---

## Passo 5 — Verificar os pinos (se o seu circuito for diferente)

Se você montou o circuito em pinos diferentes dos indicados na tabela, atualize as definições no início do código:

```cpp
#define SS_PIN  5    // pino SDA do RFID
#define RST_PIN 22   // pino RST do RFID
#define LED_VERDE    26
#define LED_VERMELHO 27
```

E na função `setup()`, o pino do servo:

```cpp
cancela.attach(13, 500, 2400);  // troque 13 pelo seu pino
```

---

## Passo 6 — Carregar o código no ESP32

1. Conecte o ESP32 ao computador via cabo USB.
2. No Arduino IDE, vá em **Tools → Port** e selecione a porta onde o ESP32 está conectado.
   - No Windows aparece como `COM3`, `COM4`, etc.
   - No Mac/Linux aparece como `/dev/ttyUSB0` ou similar.
3. Clique no botão **Upload** (ícone de seta →).
4. Aguarde a mensagem **"Done uploading"**.

> Se aparecer erro de porta, tente manter o botão **BOOT** do ESP32 pressionado enquanto o upload começa.

---

## Passo 7 — Descobrir o IP do ESP32

Após o upload, abra o **Serial Monitor** (**Tools → Serial Monitor**, velocidade **115200**).

Quando o ESP32 conectar ao Wi-Fi, ele imprime o IP na tela. Exemplo:

```
192.168.1.105
```

Anote esse número — você vai usá-lo no próximo passo.

---

## Passo 8 — Conectar o dashboard ao ESP32

Abra o arquivo `pedagio-inteligente.html` em qualquer editor de texto (Bloco de Notas, VS Code etc.).

Localize esta linha:

```javascript
const ESP32_URL = 'http://IP_DO_ESP32/dados';
```

Substitua `IP_DO_ESP32` pelo IP que você anotou no passo anterior. Exemplo:

```javascript
const ESP32_URL = 'http://192.168.1.105/dados';
```

Salve o arquivo.

---

## Passo 9 — Abrir o dashboard

1. Certifique-se de que o computador está na **mesma rede Wi-Fi** que o ESP32.
2. Abra o arquivo `pedagio-inteligente.html` diretamente no navegador (clique duplo ou arraste para o navegador).
3. O dashboard deve carregar e começar a receber dados automaticamente.

Se tudo estiver correto, o indicador no canto superior direito ficará **verde** e os dados serão atualizados a cada 2 segundos.

---

## Solução de problemas comuns

**O dashboard mostra "Sem comunicação com o ESP32"**
- Verifique se o computador e o ESP32 estão na mesma rede Wi-Fi.
- Confirme o IP no Serial Monitor e compare com o que está no `ESP32_URL`.
- Tente acessar `http://SEU_IP/dados` diretamente no navegador. Deve aparecer um texto em formato JSON.

**O ESP32 não conecta ao Wi-Fi**
- Confirme que o nome e a senha da rede estão corretos (letras maiúsculas e minúsculas importam).
- Confirme que a rede é 2.4 GHz.

**A tag não é reconhecida**
- Aproxime a tag a menos de 3 cm do leitor.
- Verifique no Serial Monitor se o código da tag está aparecendo.
- Compare o código exibido com o que está cadastrado no código (deve ser igual, em letras minúsculas).

**O servo não se move ou treme**
- Teste com a alimentação do servo direto nos 5V do ESP32. Se ainda tremer, use uma fonte externa de 5V para o servo.
- Ajuste os valores `FECHADA` e `ABERTA` conforme descrito no Passo 4.5.

**O upload falha no Arduino IDE**
- Segure o botão **BOOT** do ESP32 enquanto o upload começa.
- Troque o cabo USB (alguns cabos são apenas de carga, sem dados).

---

## Como adicionar uma nova tag RFID

1. Aproxime a nova tag do leitor com o Serial Monitor aberto e anote o código.
2. No código, adicione uma nova linha no bloco de identificação:

```cpp
// antes
if      (conteudo == "a3f21b09") processarPagamento("Carlos", saldo1);
else if (conteudo == "7c4e90d1") processarPagamento("Ana",    saldo2);

// depois — adicionando João com saldo3
if      (conteudo == "a3f21b09") processarPagamento("Carlos", saldo1);
else if (conteudo == "7c4e90d1") processarPagamento("Ana",    saldo2);
else if (conteudo == "b8d34c22") processarPagamento("Joao",   saldo3);
```

3. Declare o saldo correspondente no início do arquivo:

```cpp
float saldo3 = 100;
```

4. Faça o upload novamente.

---

## Diagrama de pinos resumido

```
ESP32
│
├── GPIO 5  ──→ RFID SDA
├── GPIO 18 ──→ RFID SCK
├── GPIO 19 ──→ RFID MISO
├── GPIO 22 ──→ RFID RST
├── GPIO 23 ──→ RFID MOSI
│
├── GPIO 13 ──→ Servo (sinal)
│
├── GPIO 26 ──→ Resistor 220Ω ──→ LED Verde (+)
└── GPIO 27 ──→ Resistor 220Ω ──→ LED Vermelho (+)

3.3V ──→ RFID VCC
5V   ──→ Servo VCC
GND  ──→ RFID GND, Servo GND, LEDs GND (-)
```
