# Absent-mind-Arduino conexiones de arduino

#ESP32 CON SENSOR DE TEMPERATURA Y CONTROL DE RELE

rssi_temp_SPNOW_TX.ino --> contiene el código fuente arduino del microcontrolador ESP32 que monitoriza la temperatura y activa o desactiva el rele en funcion de un umbral para esta temperatura.
                       Además funcion como un emisor mediante el protocolo ESP_NOW enviando datos sobre una RSSI concreta.

![esp32_temp_rele](https://github.com/user-attachments/assets/0d4233a2-fece-43a2-a116-f92c88e1fc99)

#ESP32 CON SENSOR DE PRESENCIA PIR Y CONTROL DE RELE

Sensor_de_presencia.ino  --> contiene el código fuente arduino del microcontrolador ESP32 que monitoriza la deteccion de una presencia, en funcion de un sensor PIR, el cual registra 1 o 0 y
                         segun estos valores activa o no un rele. Este microcontrolador viene regulado por el microcontrolador ESP32 que funciona como emisor, es decir, que empieza a 
                         funcionar cuando la presencia del usuario empieza a estar presente. 

![esp_pir_rele](https://github.com/user-attachments/assets/8063ccef-ab19-4fa3-9c67-aafe10d313f1)

#ESP32 CON DFPLAYERMINI, ALTAVOZ Y BOTON

altavozpulsadorhora_RX_SPNOW.ino --> contiene el código fuente arduino del microcontrolador ESP32 que funciona como una alarma, el cual esta regulado por una hora y si hay un individuo presente.
                                     Este microcontrolador viene regulado por el microcontrolador ESP32 que funciona como emisor, es decir, que empieza a 
                                     funcionar cuando la presencia del usuario empieza a estar presente. 
                                  
![esp_altavoz_pusl](https://github.com/user-attachments/assets/eecc8e17-ab5d-4cdd-8017-f6c90863a45f)
