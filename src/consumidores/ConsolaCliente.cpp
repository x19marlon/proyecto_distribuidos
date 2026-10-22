#include "consumidores/ConsolaCliente.hpp"

#include <iostream>

ConsolaCliente::ConsolaCliente(Cliente& cliente) : cliente_(cliente) {}

int ConsolaCliente::ejecutar() {
    std::cout << "Sistema de reservas de eventos universitarios – estudiante "
              << cliente_.idEstudiante() << "\n";
    while (std::cin) {
        std::cout << "\n 1. Consultar eventos de un mes\n"
                     " 2. Reservar cupos\n"
                     " 0. Salir\n";
        const std::string opcion = preguntar("Opción");
        if (opcion == "0" || !std::cin) break;
        if (opcion == "1") consultarEventosMes();
        else if (opcion == "2") reservar();
        else std::cout << "Opción no válida\n";
    }
    return 0;
}

void ConsolaCliente::consultarEventosMes() {
    const Mensaje respuesta = cliente_.consultarEventosMes(preguntar("Mes (AAAA-MM)"));
    if (respuesta.esError()) return mostrarError(respuesta);
    if (respuesta.filas().empty()) {
        std::cout << "No hay eventos en ese mes\n";
        return;
    }
    for (const Fila& f : respuesta.filas()) {
        std::cout << "  " << f.at("idFecha") << "  " << f.at("fecha") << "  " << f.at("evento")
                  << " – " << f.at("lugar") << "\n"
                  << "        cupos: " << f.at("reservas") << "/" << f.at("limite")
                  << " ocupados, " << f.at("disponibles") << " disponibles; precio por cupo: $"
                  << f.at("precio") << "\n";
    }
}

void ConsolaCliente::reservar() {
    const std::string idFecha = preguntar("Id de la fecha (por ejemplo F-1)");
    const int cupos = preguntarEntero("Cantidad de cupos");
    const Mensaje respuesta = cliente_.reservar(idFecha, cupos);
    if (respuesta.esError()) return mostrarError(respuesta);
    std::cout << "Reserva " << respuesta.texto("idReserva") << " creada: "
              << respuesta.texto("cupos") << " cupos, valor $" << respuesta.texto("monto")
              << ". Quedan " << respuesta.texto("disponibles") << " cupos.\n";
}

std::string ConsolaCliente::preguntar(const std::string& texto) {
    std::cout << texto << ": " << std::flush;
    std::string linea;
    std::getline(std::cin, linea);
    return linea;
}

int ConsolaCliente::preguntarEntero(const std::string& texto) {
    try {
        return std::stoi(preguntar(texto));
    } catch (...) {
        return 0;  // el procesador RMC lo rechaza como dato inválido
    }
}

void ConsolaCliente::mostrarError(const Mensaje& respuesta) {
    std::cout << "Error " << respuesta.texto("codigo") << ": " << respuesta.texto("detalle")
              << "\n";
}
