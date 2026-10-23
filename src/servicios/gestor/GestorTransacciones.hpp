#pragma once

#include <deque>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "comun/Servicio.hpp"

// Gestor de Transacciones: punto de entrada de la Capa de Servicios de Negocio.
//
//   clientes (REQ) ──► frontend ROUTER ── GestorTransacciones ── backend ROUTER ◄── procesadores RMC (DEALER)
//
// - Las consultas y reservas se reparten entre los procesadores RMC libres (patrón
//   "load balancing broker" de ZeroMQ) y la respuesta vuelve al cliente en la misma
//   llamada (síncrono).
// - Los procesadores libres envían un latido cada segundo (patrón Paranoid Pirate);
//   si uno deja de hacerlo por 3 s se descarta. Si el gestor se reinicia, los latidos
//   vuelven a registrar a los procesadores.
class GestorTransacciones : public Servicio {
public:
    GestorTransacciones(std::string frontend, std::string backend);

protected:
    void inicializar() override;
    void iterar() override;

private:
    void atenderCliente();
    void atenderProcesador();
    void marcarLibre(const std::string& idProcesador);
    bool enviarAProcesador(const zmq::message_t& idCliente, const zmq::message_t& peticion);
    void responderCliente(const std::string& idCliente, const std::string& respuesta);

    std::string endpointFrontend_;  // donde se conectan los clientes
    std::string endpointBackend_;   // donde se conectan los procesadores RMC
    std::unique_ptr<zmq::socket_t> frontend_;
    std::unique_ptr<zmq::socket_t> backend_;
    std::deque<std::string> procesadoresLibres_;
    std::unordered_set<std::string> enCola_;                     // los mismos, para buscar rápido
    std::unordered_set<std::string> ocupados_;                   // atendiendo una petición
    std::unordered_map<std::string, long long> ultimoContacto_;  // idProcesador → relojMs
};
