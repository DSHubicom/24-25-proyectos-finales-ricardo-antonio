#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>            // Para comunicación I2C
#include <SparkFun_TMP117.h> // Librería del sensor TMP117

// Configuración del sensor TMP117
TMP117 sensor;

// Pines y configuración
const int PIN = 2;        // Pin de salida
const int CUTOFF = -40;   // Umbral de RSSI
const String TARGET_MAC = "AA:BB:DA:97:34:EE"; // MAC objetivo
const int relayPin = 26;   // Pin del relé

// Direcciones MAC de los receptores
uint8_t receiverMAC1[] = {0x24, 0x0A, 0xC4, 0xA6, 0xD9, 0x14}; // MAC del primer receptor
uint8_t receiverMAC2[] = {0xB4, 0xE6, 0x2D, 0x8D, 0x50, 0x6D}; // MAC del segundo receptor

// Variables para enviar datos
typedef struct {
  int RSSIStatus; // Estado de la RSSI (E)
} struct_message;

struct_message dataToSend;

// Variables de temporización
unsigned long relayOffStartTime = 0; // Momento en que empieza a cumplir la condición para apagar el relé
bool relayOffConditionMet = false;  // Indica si se cumplen las condiciones para apagar el relé

void setup() {
  Serial.begin(115200);

  // Configuración de pines
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN, LOW);
  pinMode(relayPin, OUTPUT);

  // Configuración del sensor TMP117
  Wire.begin();
  Wire.setClock(400000);
  if (sensor.begin() == true) {
    Serial.println("Sensor TMP117 inicializado.");
  } else {
    Serial.println("Error al inicializar TMP117.");
    while (1); // Detener en caso de error
  }

  // Configuración WiFi y ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al inicializar ESP-NOW");
    return;
  }

  // Registrar el primer receptor
  esp_now_peer_info_t peerInfo1;
  memcpy(peerInfo1.peer_addr, receiverMAC1, 6);
  peerInfo1.channel = 0; // Canal 0 = todos los canales
  peerInfo1.encrypt = false;

  if (esp_now_add_peer(&peerInfo1) != ESP_OK) {
    Serial.println("Error al añadir el primer receptor.");
    return;
  }

  // Registrar el segundo receptor
  esp_now_peer_info_t peerInfo2;
  memcpy(peerInfo2.peer_addr, receiverMAC2, 6);
  peerInfo2.channel = 0; // Canal 0 = todos los canales
  peerInfo2.encrypt = false;

  if (esp_now_add_peer(&peerInfo2) != ESP_OK) {
    Serial.println("Error al añadir el segundo receptor.");
    return;
  }

  Serial.println("Emisor ESP-NOW configurado con dos receptores.");
}

// Escanea redes y detecta el dispositivo objetivo
int readRSSi(int netWorkCount, int &bestRSSI) {
  for (int i = 0; i < netWorkCount; i++) {
    String ssid = WiFi.SSID(i);            // Obtiene el SSID de la red
    int rssi = WiFi.RSSI(i);               // Obtiene el RSSI de la red
    String bssid = WiFi.BSSIDstr(i);       // Obtiene la MAC de la red
    Serial.printf("Red: %s, MAC: %s, RSSI: %d dBm\n", ssid.c_str(), bssid.c_str(), rssi);

    if (bssid == TARGET_MAC && rssi > bestRSSI) { // Si la MAC coincide y el RSSI supera el umbral
      bestRSSI = rssi;
    }
  }

  if (bestRSSI > CUTOFF) {
    digitalWrite(PIN, HIGH); // Activa el pin
    Serial.printf("Dispositivo detectado. Mejor RSSI: %d dBm\n", bestRSSI);
    return HIGH;
  } else {
    digitalWrite(PIN, LOW); // Apaga el pin
    Serial.println("Dispositivo no detectado o RSSI bajo.");
    return LOW;
  }
}

// Callback de envío de ESP-NOW
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\nEstado del envío: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Éxito" : "Fallido");
}

void loop() {
  Serial.println("Escaneando redes...");
  int networkCount = WiFi.scanNetworks(); // Inicia el escaneo de redes
  int bestRSSI = CUTOFF; // Valor inicial para comparar la mejor señal

  int E = readRSSi(networkCount, bestRSSI);
  float tempC = sensor.readTempC();
  
  Serial.printf("Temperatura actual: %.2f°C\n", tempC);

  // Verifica condiciones y controla el relé
  if (E == 1 && tempC < 27.0) {
    digitalWrite(relayPin, LOW);
    Serial.println("Relé ON");
    relayOffStartTime = 0; // Reinicia el temporizador si la condición de apagado no se cumple
    relayOffConditionMet = false;
  } else if (E == 0 && tempC > 19.0) {
    if (!relayOffConditionMet) {
      relayOffStartTime = millis(); // Registra el tiempo actual
      relayOffConditionMet = true;
    } else if (millis() - relayOffStartTime >= 18000) { // Verifica si han pasado 3 minutos
      digitalWrite(relayPin, HIGH);
      Serial.println("Relé OFF");
    }
  } else {
    relayOffStartTime = 0; // Reinicia el temporizador si las condiciones no se cumplen
    relayOffConditionMet = false;
  }

  if (E == 0) {
    // Si se cumple la condición, envía los datos a ambos receptores
    dataToSend.RSSIStatus = bestRSSI; // Enviar el valor de RSSI
    Serial.println(bestRSSI);
    // Enviar datos al primer receptor
    esp_err_t result1 = esp_now_send(receiverMAC1, (uint8_t *)&dataToSend, sizeof(dataToSend));
    if (result1 == ESP_OK) {
      Serial.println("Datos enviados correctamente al primer receptor.");
    } else {
      Serial.println("Error al enviar datos al primer receptor.");
    }

    // Enviar datos al segundo receptor
    esp_err_t result2 = esp_now_send(receiverMAC2, (uint8_t *)&dataToSend, sizeof(dataToSend));
    if (result2 == ESP_OK) {
      Serial.println("Datos enviados correctamente al segundo receptor.");
    } else {
      Serial.println("Error al enviar datos al segundo receptor.");
    }
  }

  delay(3000); // Espera antes de la próxima iteración
}
