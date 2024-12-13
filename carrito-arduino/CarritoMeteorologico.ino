#include <SPI.h>
#include <Ethernet.h>
#include <SD.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>
#include <Dns.h>
#include <DHT.h>


// Configuración de red y DHT
byte mac[] = { 0xAA, 0x1E, 0xDA, 0x3C, 0x4D, 0x5E };
IPAddress ip(192, 168, 0, 130);
IPAddress subnet(255, 255, 255, 0);
IPAddress gateway(192, 168, 0, 1);
IPAddress dns(8, 8, 8, 8);

#define DHTPIN 7
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Configuración de motores
const int Motor1A = 2, Motor1B = 6, pwmP1 = 3;
const int Motor2A = 8, Motor2B = 9, pwmP2 = 5;

// Pines SD y servidor web
EthernetServer server(8080);

// Configuración de base de datos
char host[] = "mysql-variety-tests-marcosjn.g.aivencloud.com";
int port = 11008;
char user[] = "avnadmin";
char password[] = "AVNS_LwGXg7DAUwxKDI4aeHI";
char database[] = "programables";
EthernetClient cliente;
MySQL_Connection conn((Client *)&cliente);
bool conState = false;

// Funciones de motores
// void manejarMotor(const String& accion) {
//   if (accion == "AVANZA") Serial.println("---- ESTADO: AVANZANDO ----");
//   else if (accion == "RETROCEDE") Serial.println("---- ESTADO: RETROCEDIENDO ----");
//   else if (accion == "IZQUIERDA") Serial.println("---- ESTADO: IZQUIERDA ----");
//   else if (accion == "DERECHA") Serial.println("---- ESTADO: DERECHA ----");
//   else if (accion == "DETENERSE") Serial.println("---- ESTADO: DETENIDO ----");
//   Serial.println(accion);
// }

void manejarMotor(const String& accion) {
  if (accion == "AVANZA") {
    Serial.println("---- ESTADO: AVANZANDO ----");
    digitalWrite(Motor1A, HIGH);
    digitalWrite(Motor1B, LOW);
    analogWrite(pwmP1, 120);
    digitalWrite(Motor2A, HIGH);
    digitalWrite(Motor2B, LOW);
    analogWrite(pwmP2, 120);
  } else if (accion == "RETROCEDE") {
    Serial.println("---- ESTADO: RETROCEDIENDO ----");
    digitalWrite(Motor1A, LOW);
    digitalWrite(Motor1B, HIGH);
    analogWrite(pwmP1, 120);
    digitalWrite(Motor2A, LOW);
    digitalWrite(Motor2B, HIGH);
    analogWrite(pwmP2, 120);
  } else if (accion == "IZQUIERDA") {
    Serial.println("---- ESTADO: IZQUIERDA ----");
    digitalWrite(Motor1A, LOW);
    digitalWrite(Motor1B, HIGH);
    analogWrite(pwmP1, 120);
    digitalWrite(Motor2A, HIGH);
    digitalWrite(Motor2B, LOW);
    analogWrite(pwmP2, 120);
  } else if (accion == "DERECHA") {
    Serial.println("---- ESTADO: DERECHA ----");
    digitalWrite(Motor1A, HIGH);
    digitalWrite(Motor1B, LOW);
    analogWrite(pwmP1, 120);
    digitalWrite(Motor2A, LOW);
    digitalWrite(Motor2B, HIGH);
    analogWrite(pwmP2, 120);
  } else if (accion == "DETENERSE") {
    Serial.println("---- ESTADO: DETENIDO ----");
    digitalWrite(Motor1A, LOW);
    digitalWrite(Motor1B, LOW);
    analogWrite(pwmP1, 0);
    digitalWrite(Motor2A, LOW);
    digitalWrite(Motor2B, LOW);
    analogWrite(pwmP2, 0);
  }
}

// Funciones base de datos
void intentarConectarBD() {
  if (conn.connected()) return;
  Serial.println("Intentando conexión a BD...");
  IPAddress serverIP;
  DNSClient dns;
  dns.begin(Ethernet.dnsServerIP());
  if (dns.getHostByName(host, serverIP) == 1 && conn.connect(serverIP, port, user, password)) {
    conState = true;
    Serial.println("Conexión a BD exitosa.");
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
    cur_mem->execute("USE programables");
    delete cur_mem;
  } else {
    conState = false;
    Serial.println("Fallo conexión a BD.");
  }
}

String insercion = "";
void insertarTemperatura(float temperatura) {
  if (!conState) {
    intentarConectarBD();
    if (!conState) return; // Si no hay conexión, salimos de la función
  }
  insercion = "USE programables; INSERT INTO lecturas(temperatura) Values(" + String(temperatura) + ");";
  // Usar sentencias preparadas para evitar inyección SQL
  char query[] = "INSERT INTO lecturas(temperatura) VALUES (?)";

  MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
  cur_mem -> execute( insercion.c_str() );
  delete cur_mem;
}

// Enviar página HTML
void enviarPaginaHTML(EthernetClient& client) {
  client.println("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
  File archivoHTML = SD.open("index.txt");
  if (archivoHTML) {
    while (archivoHTML.available()) client.write(archivoHTML.read());
    archivoHTML.close();
  } else {
    client.println("<html><body><h1>Error al cargar el archivo HTML</h1></body></html>");
  }
}

void setup(){
  pinMode(4, OUTPUT);
  pinMode(10, OUTPUT);
  dht.begin();
  // disable SD SPI
  digitalWrite(4,HIGH);
  // disable w5100 SPI
  digitalWrite(10,HIGH);

  Serial.begin(9600);
  delay ( 2000 );
  Ethernet.begin(mac);
  delay ( 2000 );
  Ethernet.setLocalIP(ip);
  delay ( 2000 );
  Serial.println("Conectado a la red.");
  Serial.println(Ethernet.localIP());
  Serial.println(Ethernet.gatewayIP());

  if (!SD.begin(4)) {
    Serial.println("Fallo en la inicialización de la tarjeta SD");
    while (true);
  }
  Serial.println("Tarjeta SD inicializada correctamente.");

  server.begin();

  pinMode(Motor1A, OUTPUT);
  pinMode(Motor1B, OUTPUT);
  pinMode(Motor2A, OUTPUT);
  pinMode(Motor2B, OUTPUT);
  pinMode(pwmP1, OUTPUT);
  pinMode(pwmP2, OUTPUT);
  intentarConectarBD();
}
String response;

void enviarTemperatura(EthernetClient& client, float temperatura) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close"); // Cierra la conexión después de la respuesta
    client.println();
    client.print("{\"temperature\":");
    client.print(temperatura, 2); // Redondea a 2 decimales
    client.println("}");
    delay(1); // Pequeña espera antes de cerrar
    client.stop(); // Cierra la conexión
}

int timeRunning = 0;
int timeWebRunning = 0;
void loop() {
  float temperatura = dht.readTemperature(false);
  //Serial.println ( temperatura ); 
  EthernetClient cliente = server.available();

  if (cliente) {
    timeWebRunning = 0;
    String peticion;
    while (cliente.connected()) {
      if (cliente.available()) {
        char c = cliente.read();
        peticion += c;

        if (c == '\n') {
          if (peticion.indexOf("GET /temperature") != -1) {
            temperatura = dht.readTemperature(false);
            enviarTemperatura(cliente, temperatura);
            break;
          } else if (peticion.indexOf("MOTOR=") != -1) {
            int startIndex = peticion.indexOf("MOTOR=") + 6;
            int endIndex = peticion.indexOf(' ', startIndex);
            String accion = peticion.substring(startIndex, endIndex == -1 ? peticion.length() : endIndex);
            manejarMotor(accion);
          }
          enviarPaginaHTML(cliente);
          break;
        }
      }
      delay(1);
      timeWebRunning+=1;
      if ( timeWebRunning % 5000 == 0 ){
        Serial.println(temperatura);
        // insertarTemperatura(temperatura);
        timeWebRunning = 0;
      }
    }
    delay(1);
    cliente.stop();
  }

  // Leer temperatura y enviarla a la base de datos
  // float temperatura = dht.readTemperature();
  // if (!isnan(temperatura)) {
  //   insertarTemperatura(temperatura);
  // }
  delay(100);
  timeRunning+=100;
  if ( timeRunning%5000 == 0 ) {
    Serial.println(temperatura);
    // insertarTemperatura(temperatura);
    timeRunning = 0;
  }
}
