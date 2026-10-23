#pragma once

#include <string>

#include "consumidores/Cliente.hpp"

// Menú de texto para que un estudiante consulte eventos y reserve cupos.
class ConsolaCliente {
public:
    explicit ConsolaCliente(Cliente& cliente);

    int ejecutar();

private:
    void consultarEventosMes();
    void reservar();

    std::string preguntar(const std::string& texto);
    int preguntarEntero(const std::string& texto);
    void mostrarError(const Mensaje& respuesta);

    Cliente& cliente_;
};
