#ifndef WIFI_MOD_H
#define WIFI_MOD_H
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// Credenciales SSID
constexpr const char *ssid = "Gateway0";
constexpr const char *password = "gateway123";

// Prototipo de función
void conectarWiFi();
void verificarWiFi();
#endif
