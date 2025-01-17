#include <WiFi.h>
#include <esp_now.h>
#include <time.h>  // Importa la librería time para usar getLocalTime
#include "DFRobotDFPlayerMini.h"

// Configuración WiFi
const char* ssid = "Elrichard"; // Reemplaza con tu SSID
const char* password = "olaquetal123"; // Reemplaza con tu contraseña

// Pines del DFPlayer
#define TX_PIN 17  // TX del DFPlayer a RX de la ESP32
#define RX_PIN 16  // RX del DFPlayer a TX de la ESP32

// Pin del botón
#define BOTON_PIN 18

// Instancia del DFPlayer Mini
HardwareSerial mySerial(1); // Usaremos el UART1 de la ESP32
DFRobotDFPlayerMini myDFPlayer;

// Variables de la alarma
int alarmaHora = -1;
int alarmaMinuto = -1;
bool alarmaActiva = false; // Indica si la alarma está configurada
bool condicionRecibida = false; // Indica si se recibió la señal del emisor

// Estructura para recibir datos ESP-NOW
typedef struct {
  int RSSIStatus; // Estado de la RSSI (E)
} struct_message;

struct_message receivedData; // Datos recibidos

// Callback para recibir datos ESP-NOW
void onDataRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
  // Copia el dato recibido a la estructura
  memcpy(&receivedData, data, sizeof(receivedData));

  Serial.printf("Datos recibidos de MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
                recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
  Serial.printf("RSSIStatus recibido: %d\n", receivedData.RSSIStatus);

  // Verifica si la condición del emisor se cumple
  if (receivedData.RSSIStatus <= -40) {
    condicionRecibida = true; // Activa la condición
    Serial.println("Condición recibida: Loop habilitado");
  }
}

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

void setup() {
  Serial.begin(115200);

  // Configura el botón
  pinMode(BOTON_PIN, INPUT_PULLUP); // Resistencia pull-up para el botón

  // Conexión a WiFi
  Serial.println("Conectando al WiFi...");
  connectToWiFi();  // Usamos la función connectToWiFi()

  // Configura la sincronización de hora desde NTP
  configTime(1 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  waitForTimeSync();  // Esperamos a que se sincronice la hora

  // Configura el DFPlayer
  mySerial.begin(9600, SERIAL_8N1, RX_PIN, TX_PIN); // UART1
  if (!myDFPlayer.begin(mySerial)) {
    Serial.println("No se pudo inicializar el DFPlayer Mini.");
    while (true);
  }
  Serial.println("DFPlayer Mini inicializado correctamente.");
  myDFPlayer.volume(10); // Ajusta el volumen (0-30)

  // Configuración de ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error al inicializar ESP-NOW.");
    return;
  }
  esp_now_register_recv_cb(onDataRecv);
  Serial.println("Receptor ESP-NOW configurado.");

  // Configuración inicial de la alarma
  configurarAlarma();
}

// Función para configurar la alarma
void configurarAlarma() {
  Serial.println("Introduce la hora para configurar la alarma en formato HH:MM");
  while (true) {
    if (Serial.available()) {
      String entrada = Serial.readStringUntil('\n');
      entrada.trim(); // Elimina espacios en blanco
      int separador = entrada.indexOf(':');
      if (separador != -1) {
        String hora = entrada.substring(0, separador);
        String minuto = entrada.substring(separador + 1);
        alarmaHora = hora.toInt();
        alarmaMinuto = minuto.toInt();
        alarmaActiva = true;
        Serial.printf("Alarma configurada para las %02d:%02d\n", alarmaHora, alarmaMinuto);
        break;  // Salir del loop después de configurar la alarma
      } else {
        Serial.println("Formato incorrecto. Usa HH:MM");
      }
    }
  }
}

void loop() {
  // Espera a que se reciba la condición del emisor
  if (!condicionRecibida) {
    Serial.println("Esperando señal...");
    delay(1000);
    return;
  }

  struct tm timeInfo;
  if (!getLocalTime(&timeInfo)) {
    Serial.println("No se pudo obtener la hora actual.");
    return;
  }

  // Obtiene la hora actual
  int horaActual = timeInfo.tm_hour;
  int minutoActual = timeInfo.tm_min;
  int segundoActual = timeInfo.tm_sec;

  // Muestra la hora en el monitor serie
  Serial.printf("Hora actual: %02d:%02d:%02d\n", horaActual, minutoActual, segundoActual);

  // Verifica si la alarma está configurada
  if (alarmaActiva && horaActual == alarmaHora && minutoActual == alarmaMinuto) {
    Serial.println("¡Alarma activada! Presiona el botón para detener.");
    if (digitalRead(BOTON_PIN) == LOW) { // Si el botón está presionado
      Serial.println("Reproduciendo audio de alarma...");
      myDFPlayer.play(1); // Reproduce el archivo 0001.mp3
      delay(1000); // Anti-rebote
      alarmaActiva = false; // Desactiva la alarma
    }
  }

  delay(1000); // Actualiza cada segundo
}
