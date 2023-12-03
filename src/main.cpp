#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

const char* ssid = "Livebox-7580";
const char* password = "florom2020M";

const int triggerPin = D2;
const int echoPin = D1;

const char* serverAddress = "http://192.168.1.22:3000";
const char* iotEndpoint = "/iot/";
const char* measurementEndpoint = "/measurement/";
WiFiClient client;
HTTPClient http;

String iotId;

void connectToWiFi();

void registerIoTDevice();

void sendMeasurement(int height);

String generateName();

String generateKey();

void performMeasurement();

void saveIoTId();

bool loadIoTId();

void setup() {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.begin(115200);
    pinMode(triggerPin, OUTPUT);
    pinMode(echoPin, INPUT);

    connectToWiFi();
    LittleFS.begin();

    if (!loadIoTId()) {
        registerIoTDevice();
        saveIoTId();
    }

    // Effectuer la première mesure
    performMeasurement();

    // Mettre l'ESP8266 en mode deep sleep pendant 1 heure (en microsecondes)
    EspClass::deepSleep(3600e6, WAKE_RF_DEFAULT);
}

void loop() {
    // Ne sera pas exécuté
}

void connectToWiFi() {
    Serial.print("Connexion au réseau Wi-Fi...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }

    Serial.println("\nConnecté au réseau Wi-Fi");
}

bool loadIoTId() {
    if (LittleFS.exists("/iotid.txt")) {
        File file = LittleFS.open("/iotid.txt", "r");
        if (file) {
            iotId = file.readString();
            file.close();
            return true;
        }
    }
    return false;
}

void saveIoTId() {
    File file = LittleFS.open("/iotid.txt", "w");
    if (file) {
        file.print(iotId);
        file.close();
    }
}

void registerIoTDevice() {
    Serial.println("Enregistrement de l'appareil IoT...");

    http.begin(client, String(serverAddress) + iotEndpoint);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument jsonDoc(256);
    jsonDoc["name"] = generateName();
    jsonDoc["key"] = generateKey();

    String requestBody;
    serializeJson(jsonDoc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode == 200) {
        String response = http.getString();
        Serial.println("Enregistrement réussi");
        Serial.print("Réponse de l'enregistrement : ");
        Serial.println(response);

        DynamicJsonDocument responseDoc(256);
        deserializeJson(responseDoc, response);

        iotId = responseDoc["_id"].as<String>();
    } else {
        Serial.print("Erreur lors de l'enregistrement de l'appareil IoT. Code d'erreur : ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

void performMeasurement() {
    // Déclencher une mesure de distance
    digitalWrite(triggerPin, LOW);
    delayMicroseconds(2);
    digitalWrite(triggerPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(triggerPin, LOW);

    // Mesurer le temps de voyage du signal ultrasonique
    long duration = pulseIn(echoPin, HIGH);

    // Convertir le temps de voyage en distance en centimètres
    int distance = duration * 0.034 / 2;

    // Envoyer les données de mesure au serveur
    sendMeasurement(distance);
}

void sendMeasurement(int height) {
    http.begin(client, String(serverAddress) + measurementEndpoint);
    http.addHeader("Content-Type", "application/json");

    DynamicJsonDocument jsonDoc(256);
    jsonDoc["ioT"] = iotId;
    jsonDoc["height"] = height;

    String requestBody;
    serializeJson(jsonDoc, requestBody);

    int httpResponseCode = http.POST(requestBody);

    if (httpResponseCode == 200) {
        Serial.println("Données de mesure envoyées avec succès");
    } else {
        Serial.print("Erreur lors de l'envoi des données de mesure. Code d'erreur : ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

String generateName() {
    // Générer un nom pour l'appareil IoT (vous pouvez personnaliser la logique ici)
    // Exemple : "Device123"
    return "Device" + String(random(1000));
}

String generateKey() {
    // Générer une clé pour l'appareil IoT (vous pouvez personnaliser la logique ici)
    // Exemple : "Key123"
    return "Key" + String(random(1000));
}