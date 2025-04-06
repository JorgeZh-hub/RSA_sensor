#ifndef WIFI_MOD_H
#define WIFI_MOD_H
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Credenciales SSID
constexpr const char *ssid = "TP-Link_6DC4";
constexpr const char *password = "59655831";

// Prototipo de función
void conectarWiFi();
void verificarWiFi();
#endif
