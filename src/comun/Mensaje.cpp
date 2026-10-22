#include "comun/Mensaje.hpp"

#include <sstream>

#include "comun/Utilidades.hpp"

Mensaje::Mensaje(std::string tipo) : tipo_(std::move(tipo)) {}

Mensaje Mensaje::error(const std::string& codigo, const std::string& detalle) {
    Mensaje mensaje("ERROR");
    mensaje.poner("codigo", codigo).poner("detalle", detalle);
    return mensaje;
}

Mensaje& Mensaje::poner(const std::string& clave, const std::string& valor) {
    campos_[clave] = valor;
    return *this;
}

Mensaje& Mensaje::poner(const std::string& clave, const char* valor) {
    return poner(clave, std::string(valor));
}

Mensaje& Mensaje::poner(const std::string& clave, long long valor) {
    return poner(clave, std::to_string(valor));
}

bool Mensaje::tiene(const std::string& clave) const {
    auto it = campos_.find(clave);
    return it != campos_.end() && !it->second.empty();
}

std::string Mensaje::texto(const std::string& clave, const std::string& porDefecto) const {
    auto it = campos_.find(clave);
    return it == campos_.end() ? porDefecto : it->second;
}

long long Mensaje::entero(const std::string& clave, long long porDefecto) const {
    auto it = campos_.find(clave);
    if (it == campos_.end() || it->second.empty()) return porDefecto;
    try {
        size_t leidos = 0;
        const long long valor = std::stoll(it->second, &leidos);
        return leidos == it->second.size() ? valor : porDefecto;
    } catch (...) {
        return porDefecto;
    }
}

void Mensaje::agregarFila(Fila fila) { filas_.push_back(std::move(fila)); }

std::string Mensaje::serializar() const {
    std::string salida = tipo_ + '\n';
    auto escribirCampos = [&salida](const Fila& campos) {
        for (const auto& [clave, valor] : campos) {
            salida += clave + '=' + util::escapar(valor) + '\n';
        }
    };
    escribirCampos(campos_);
    for (const Fila& fila : filas_) {
        salida += "--\n";
        escribirCampos(fila);
    }
    return salida;
}

Mensaje Mensaje::deserializar(const std::string& datos) {
    std::istringstream entrada(datos);
    std::string linea;
    Mensaje mensaje;
    std::getline(entrada, mensaje.tipo_);

    Fila* destino = &mensaje.campos_;
    while (std::getline(entrada, linea)) {
        if (linea == "--") {
            mensaje.filas_.emplace_back();
            destino = &mensaje.filas_.back();
            continue;
        }
        const auto igual = linea.find('=');
        if (igual == std::string::npos) continue;
        (*destino)[linea.substr(0, igual)] = util::desescapar(linea.substr(igual + 1));
    }
    return mensaje;
}
