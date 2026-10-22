#pragma once

#include <map>
#include <string>
#include <vector>

// Registro de campos clave → valor; se usa para las filas de las respuestas con listas.
using Fila = std::map<std::string, std::string>;

// Mensaje que viaja entre capas como una sola trama ZeroMQ.
//
// Formato de texto:
//   TIPO
//   clave=valor
//   --            (abre una fila nueva; las líneas siguientes son campos de esa fila)
//   clave=valor
//
// Los valores se escapan, así que nunca contienen saltos de línea.
class Mensaje {
public:
    Mensaje() = default;
    explicit Mensaje(std::string tipo);

    // Respuesta de error estándar: tipo ERROR con código y detalle.
    static Mensaje error(const std::string& codigo, const std::string& detalle);

    const std::string& tipo() const { return tipo_; }
    bool esError() const { return tipo_ == "ERROR"; }

    Mensaje& poner(const std::string& clave, const std::string& valor);
    Mensaje& poner(const std::string& clave, const char* valor);
    Mensaje& poner(const std::string& clave, long long valor);

    bool tiene(const std::string& clave) const;
    std::string texto(const std::string& clave, const std::string& porDefecto = "") const;
    long long entero(const std::string& clave, long long porDefecto = 0) const;
    const Fila& campos() const { return campos_; }

    void agregarFila(Fila fila);
    const std::vector<Fila>& filas() const { return filas_; }

    std::string serializar() const;
    static Mensaje deserializar(const std::string& datos);

private:
    std::string tipo_;
    Fila campos_;
    std::vector<Fila> filas_;
};
