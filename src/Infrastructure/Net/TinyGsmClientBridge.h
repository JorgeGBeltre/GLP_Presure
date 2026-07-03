#pragma once
/**
 * TinyGsmClientBridge.h
 *
 * Adapter que permite pasar un TinyGsmClient a ESP32_MQTTv5::begin(WiFiClient&, ...).
 *
 * ESP32_MQTTv5 almacena un puntero WiFiClient* internamente pero solo usa métodos
 * de la clase base Arduino::Client (read, write, connected, connect, stop, available).
 * TinyGsmClient implementa exactamente esa misma interfaz.
 *
 * Este wrapper hereda de WiFiClient y redirige todas las operaciones de Client
 * al TinyGsmClient subyacente, sin tocar la radio WiFi del ESP32.
 */

#include <WiFi.h>

// Forward declaration — evita incluir TinyGSM aquí y posibles dobles defines
class Client;

class TinyGsmClientBridge : public WiFiClient {
public:
    explicit TinyGsmClientBridge(Client& inner) : _inner(inner) {}

    // ── Métodos que ESP32_MQTTv5 usa vía puntero WiFiClient* ──────────────
    int connect(IPAddress ip, uint16_t port) override {
        return _inner.connect(ip, port);
    }
    int connect(const char* host, uint16_t port) override {
        return _inner.connect(host, port);
    }
    size_t write(uint8_t b) override {
        return _inner.write(b);
    }
    size_t write(const uint8_t* buf, size_t size) override {
        return _inner.write(buf, size);
    }
    int available() override {
        return _inner.available();
    }
    int read() override {
        return _inner.read();
    }
    int read(uint8_t* buf, size_t size) override {
        return _inner.read(buf, size);
    }
    int peek() override {
        return _inner.peek();
    }
    void flush() override {
        _inner.flush();
    }
    void stop() override {
        _inner.stop();
    }
    uint8_t connected() override {
        return _inner.connected();
    }
    operator bool() override {
        return _inner.connected();
    }

private:
    Client& _inner;
};
