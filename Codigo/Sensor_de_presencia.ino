#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <time.h>

// Pines del PIR y del relay
const int pirPin = 23;  // Pin del PIR
const int relayPin = 26; // Pin del relay

// Variables globales
int pirState = 0;          // Estado del sensor PIR
bool startPIRControl = false; // Indica si se debe activar el control por PIR

// Estructura de datos recibidos
typedef struct {
  int RSSIStatus;
} DataPacket;

DataPacket receivedData;

// Función para conectar a WiFi
void connectToWiFi() {
  WiFi.begin("Elrichard", "olaquetal123"); // Credenciales WiFi
  Serial.print("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Conectado a WiFi");
}

// Función para sincronizar la hora
void waitForTimeSync() {
  struct tm timeInfo;
  while (!getLocalTime(&timeInfo)) {
    Serial.println("Esperando sincronización de tiempo...");
    delay(1000);
  }
  Serial.println("Sincronización de tiempo completada.");
}

// Función para verificar si es después de 22:30
bool isAfter2230() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("No se pudo obtener la hora actual.");
    return false;
  }

  int currentHour = timeInfo.tm_hour;
  int currentMinute = timeInfo.tm_min;

  Serial.printf("Hora actual: %02d:%02d\n", currentHour, currentMinute);

  if (currentHour > 13 || (currentHour == 13 && currentMinute >= 28)) {
    return true;
  }
  return false;
}

// Callback para recibir datos de ESP-NOW
void onDataReceive(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
   Serial.println("Callback de recepción ejecutado.");
  Serial.println("Datos recibidos a través de ESP-NOW:");
  Serial.print("RSSIStatus: ");
  Serial.println(receivedData.RSSIStatus);

  if (receivedData.RSSIStatus <=-40 && isAfter2230()) {
    startPIRControl = true;
    Serial.println("Iniciando control basado en PIR (después de la hora objetivo).");
  } else {
    startPIRControl = false;
    digitalWrite(relayPin, LOW);
    Serial.println("Deteniendo control basado en PIR. Relay OFF.");
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(pirPin, INPUT);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  connectToWiFi();
  configTime(1 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  waitForTimeSync();

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al inicializar ESP-NOW.");
    return;
  }

  esp_now_register_recv_cb(onDataReceive);

  Serial.println("Receptor ESP-NOW inicializado.");
}

void loop() {
  Serial.print("Estado de startPIRControl: ");
  Serial.println(startPIRControl);

  if (startPIRControl) {
    pirState = digitalRead(pirPin);
    Serial.print("Estado del PIR en bucle principal: ");
    Serial.println(pirState);

    if (pirState == HIGH) {
      digitalWrite(relayPin, HIGH);
      Serial.println("Relay ON (control PIR)");
    } else {
      digitalWrite(relayPin, LOW);
      Serial.println("Relay OFF (control PIR)");
    }
  } else {
    Serial.println("Control PIR inactivo. Esperando condiciones.");
    delay(3000);
  }
}
