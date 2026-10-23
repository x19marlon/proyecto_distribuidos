#pragma once

#include <string>
#include <vector>

// Funciones de apoyo compartidas por todas las capas.
namespace util {

// Hora de pared en milisegundos desde 1970 (para marcar instantes).
long long ahoraMs();

// Reloj monótono en milisegundos (para medir latencias).
long long relojMs();

// Identificador aleatorio con prefijo, por ejemplo "S-5f3a9c0d12e4b7a1".
std::string generarId(const std::string& prefijo);

// Lee una variable de entorno o devuelve el valor por defecto.
std::string variableEntorno(const char* nombre, const std::string& porDefecto);

// Escapa '\\', saltos de línea y '|' para que un texto quepa en una sola línea.
std::string escapar(const std::string& texto);
std::string desescapar(const std::string& texto);

// Divide una línea por el separador respetando los escapes; devuelve las partes ya desescapadas.
std::vector<std::string> dividir(const std::string& linea, char separador);

// Escribe una línea de log con hora y componente en la salida de errores.
void log(const std::string& componente, const std::string& texto);

}  // namespace util
