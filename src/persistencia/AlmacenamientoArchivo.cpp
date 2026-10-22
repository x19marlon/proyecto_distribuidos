#include "persistencia/AlmacenamientoArchivo.hpp"

#include <cstdint>
#include <filesystem>
#include <stdexcept>

#include "comun/Utilidades.hpp"

AlmacenamientoArchivo::AlmacenamientoArchivo(std::string ruta) : ruta_(std::move(ruta)) {
    const std::filesystem::path carpeta = std::filesystem::path(ruta_).parent_path();
    if (!carpeta.empty()) std::filesystem::create_directories(carpeta);
}

std::vector<Mensaje> AlmacenamientoArchivo::cargar() {
    std::vector<Mensaje> operaciones;
    std::ifstream entrada(ruta_);
    std::string linea;
    std::uintmax_t bytesCompletos = 0;
    while (std::getline(entrada, linea)) {
        // Si el proceso cayó a mitad de una escritura, la última línea queda sin '\n':
        // esa operación nunca se respondió, así que se descarta y se recorta del archivo.
        if (entrada.eof()) {
            entrada.close();
            std::filesystem::resize_file(ruta_, bytesCompletos);
            break;
        }
        bytesCompletos += linea.size() + 1;
        if (!linea.empty()) operaciones.push_back(Mensaje::deserializar(util::desescapar(linea)));
    }
    return operaciones;
}

void AlmacenamientoArchivo::registrar(const Mensaje& operacion) {
    if (!salida_.is_open()) {
        salida_.open(ruta_, std::ios::app);
        if (!salida_) throw std::runtime_error("no se pudo abrir " + ruta_);
    }
    salida_ << util::escapar(operacion.serializar()) << '\n';
    salida_.flush();
    if (!salida_) throw std::runtime_error("no se pudo escribir en " + ruta_);
}
