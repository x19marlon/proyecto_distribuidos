#include "comun/Utilidades.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <mutex>
#include <random>

namespace util {

long long ahoraMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

long long relojMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

std::string generarId(const std::string& prefijo) {
    thread_local std::mt19937_64 generador{std::random_device{}() ^
                                           static_cast<unsigned long long>(relojMs())};
    char hex[17];
    std::snprintf(hex, sizeof(hex), "%016llx",
                  static_cast<unsigned long long>(generador()));
    return prefijo + "-" + hex;
}

std::string variableEntorno(const char* nombre, const std::string& porDefecto) {
    const char* valor = std::getenv(nombre);
    return (valor != nullptr && *valor != '\0') ? std::string(valor) : porDefecto;
}

std::string escapar(const std::string& texto) {
    std::string salida;
    salida.reserve(texto.size());
    for (char c : texto) {
        switch (c) {
            case '\\': salida += "\\\\"; break;
            case '\n': salida += "\\n"; break;
            case '\r': salida += "\\r"; break;
            case '|': salida += "\\|"; break;
            default: salida += c;
        }
    }
    return salida;
}

std::string desescapar(const std::string& texto) {
    std::string salida;
    salida.reserve(texto.size());
    for (size_t i = 0; i < texto.size(); ++i) {
        if (texto[i] == '\\' && i + 1 < texto.size()) {
            const char siguiente = texto[++i];
            salida += siguiente == 'n' ? '\n' : siguiente == 'r' ? '\r' : siguiente;
        } else {
            salida += texto[i];
        }
    }
    return salida;
}

std::vector<std::string> dividir(const std::string& linea, char separador) {
    std::vector<std::string> partes;
    std::string actual;
    for (size_t i = 0; i < linea.size(); ++i) {
        if (linea[i] == '\\' && i + 1 < linea.size()) {
            actual += linea[i];
            actual += linea[++i];
        } else if (linea[i] == separador) {
            partes.push_back(desescapar(actual));
            actual.clear();
        } else {
            actual += linea[i];
        }
    }
    partes.push_back(desescapar(actual));
    return partes;
}

void log(const std::string& componente, const std::string& texto) {
    static std::mutex candado;
    const long long ms = ahoraMs();
    const std::time_t segundos = static_cast<std::time_t>(ms / 1000);
    std::tm local{};
    localtime_r(&segundos, &local);
    char hora[16];
    std::strftime(hora, sizeof(hora), "%H:%M:%S", &local);

    std::lock_guard<std::mutex> bloqueo(candado);
    std::cerr << '[' << hora << '.' << (ms % 1000 < 100 ? (ms % 1000 < 10 ? "00" : "0") : "")
              << ms % 1000 << "] [" << componente << "] " << texto << std::endl;
}

}  // namespace util
