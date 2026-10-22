#pragma once

#include <fstream>
#include <string>

#include "persistencia/Almacenamiento.hpp"

// Bitácora en un archivo de texto: una operación por línea, solo se agregan líneas al final.
class AlmacenamientoArchivo : public Almacenamiento {
public:
    explicit AlmacenamientoArchivo(std::string ruta);

    std::vector<Mensaje> cargar() override;
    void registrar(const Mensaje& operacion) override;

private:
    std::string ruta_;
    std::ofstream salida_;
};
