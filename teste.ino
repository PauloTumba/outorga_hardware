#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>

#define SS_PIN D8  // Pino para SDA/SS do módulo RFID
#define RST_PIN D0 // Pino para RST do módulo RFID

// Credenciais da rede WiFi
const char* ssid = "Madalorian";
const char* password = "TUKBJ4B4CT";

// Instância da classe MFRC522 para controle do módulo RFID
MFRC522 rfid(SS_PIN, RST_PIN);

// Criação do servidor web assíncrono na porta 80
AsyncWebServer server(80);
// Criação do servidor WebSocket na porta 81
WebSocketsServer webSocket = WebSocketsServer(81);

// Variáveis para armazenar o UID do cartão RFID
byte readcard[4];
char str[9];
String StrUID;

void setup() {
  Serial.begin(115200); // Inicializa a comunicação serial
  SPI.begin();          // Inicializa o barramento SPI
  rfid.PCD_Init();      // Inicializa o módulo RFID

  // Conecta à rede WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print("."); // Imprime pontos no console serial enquanto tenta conectar
  }
  Serial.println("Connected to WiFi"); // Imprime mensagem de conexão bem-sucedida

  // Define a rota raiz do servidor web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    // Envia uma resposta HTML simples que se conecta ao WebSocket
    request->send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>RFID Reader</title>
      </head>
      <body>
        <h1>RFID Reader</h1>
        <div id="message"></div>
        <script>
        // var ws = new WebSocket('ws://' + window.location.hostname + ':81/');
          var ws = new WebSocket('ws://192.168.0.119:81/');
          //var ws = new WebSocket('ws://127.0.0.1:81/');

          ws.onmessage = function(event) {
            var message = event.data;
            document.getElementById('message').innerHTML = 'UID: ' + message;
            if (message.startsWith('reload:')) {-
              var uid = message.split(':')[1];
              window.location.href = 'http://127.0.0.1:8003/admin/formado/pesqusaView/' + uid;
            }
          };
        </script>
      </body>
      </html>
    )rawliteral");
  });

  server.begin(); // Inicia o servidor web
  webSocket.begin(); // Inicia o servidor WebSocket
  webSocket.onEvent(webSocketEvent); // Define a função de callback para eventos do WebSocket

  Serial.println("WebSocket server started."); // Mensagem indicando que o servidor WebSocket iniciou
Serial.println(WiFi.localIP());
}

void loop() {
  webSocket.loop(); // Mantém o servidor WebSocket em execução

  if (getid()) { // Verifica se um cartão RFID foi lido
    String UIDresultSend = StrUID; // Converte o UID lido em string
    String message = "reload:" + UIDresultSend; // Cria a mensagem WebSocket para enviar
    webSocket.broadcastTXT(message); // Envia a mensagem WebSocket para todos os clientes conectados
    Serial.println("UID sent: " + UIDresultSend); // Imprime o UID enviado no console serial
    delay(2000); // Aguarda 1 segundo antes de continuar
  }
}

// Função de callback para eventos do WebSocket
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
  // Você pode adicionar lógica aqui para tratar eventos do WebSocket se necessário
}

// Função para ler e obter o UID do cartão
int getid() {
  if (!rfid.PICC_IsNewCardPresent()) { // Verifica se um novo cartão está presente
    return 0;
  }
  if (!rfid.PICC_ReadCardSerial()) { // Verifica se o cartão pode ser lido
    return 0;
  }

  Serial.print("UID do cartão escaneado: "); // Imprime mensagem no console serial

  for (int i = 0; i < 4; i++) { // Lê o UID do cartão
    readcard[i] = rfid.uid.uidByte[i];
  }
  array_to_string(readcard, 4, str); // Converte o UID lido em string
  StrUID = str;

  rfid.PICC_HaltA(); // Finaliza a comunicação com o cartão
  return 1; // Retorna 1 para indicar que um cartão foi lido
}

// Função para converter o UID de array para string hexadecimal
void array_to_string(byte array[], unsigned int len, char buffer[]) {
  for (unsigned int i = 0; i < len; i++) {
    byte nib1 = (array[i] >> 4) & 0x0F; // Obtém o nibble alto
    byte nib2 = (array[i] >> 0) & 0x0F; // Obtém o nibble baixo
    buffer[i * 2 + 0] = nib1 < 0xA ? '0' + nib1 : 'A' + nib1 - 0xA; // Converte o nibble alto em caractere
    buffer[i * 2 + 1] = nib2 < 0xA ? '0' + nib2 : 'A' + nib2 - 0xA; // Converte o nibble baixo em caractere
  }
  buffer[len * 2] = '\0'; // Termina a string com um caractere nulo
}
