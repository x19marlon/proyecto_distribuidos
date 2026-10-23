#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "comun/Mensaje.hpp"
#include "persistencia/Modelo.hpp"

// Datos del sistema en memoria y las operaciones sobre ellos.
//
// crearReserva es atómica: primero valida todo y solo después cambia los datos,
// así que se aplica completa o no se aplica. El servicio de persistencia atiende
// una petición a la vez, por lo que dos reservas concurrentes nunca pueden pasar
// el límite de una fecha.
class BaseDatos {
public:
    // Lugares, eventos, fechas y estudiantes de ejemplo.
    void cargarDatosEjemplo();

    Mensaje listarFechasMes(const Mensaje& peticion) const;
    // Recibe "instante", fijado por el servicio de persistencia.
    Mensaje crearReserva(const Mensaje& peticion);

    // Idempotencia: respuesta ya dada a una clave (tipo + claveIdempotencia).
    const Mensaje* respuestaPrevia(const std::string& clave) const;
    void guardarRespuesta(const std::string& clave, const Mensaje& respuesta);

private:
    void agregarLugar(std::shared_ptr<LugarUniversidad> lugar);
    void agregarEvento(Evento evento, const std::vector<std::string>& fechas);
    void agregarEstudiante(Estudiante::Datos datos);
    Fila filaFecha(const FechaEvento& fecha) const;

    std::map<std::string, std::shared_ptr<LugarUniversidad>> lugares_;
    std::map<std::string, Estudiante> estudiantes_;
    std::map<std::string, Evento> eventos_;
    std::map<std::string, FechaEvento> fechas_;
    std::map<std::string, Reserva> reservas_;
    std::vector<Transaccion> transacciones_;
    std::unordered_map<std::string, Mensaje> respuestas_;

    // idEstudiante|idEvento → idReserva (como máximo una reserva por evento)
    std::unordered_map<std::string, std::string> reservaPorEvento_;

    long long siguienteFecha_ = 1;
    long long siguienteReserva_ = 1;
    long long siguienteTransaccion_ = 1;
};
