#pragma once

#include <atomic>
#include <string>

#include "comun/Zmq.hpp"

// Clase base de todos los procesos servidores (gestor, procesadores, cancelación, persistencia).
//
// Método plantilla: ejecutar() llama a inicializar() una vez y luego a iterar()
// hasta recibir SIGINT/SIGTERM; al final llama a finalizar().
class Servicio {
public:
    explicit Servicio(std::string nombre);
    virtual ~Servicio() = default;

    Servicio(const Servicio&) = delete;
    Servicio& operator=(const Servicio&) = delete;

    int ejecutar();

    static void solicitarDetencion() { activo_ = false; }

protected:
    virtual void inicializar() = 0;
    // Debe volver en poco tiempo (usar poll con timeout) para poder detenerse.
    virtual void iterar() = 0;
    virtual void finalizar() {}

    void log(const std::string& texto) const;
    zmq::context_t& contexto() { return contexto_; }
    const std::string& nombre() const { return nombre_; }

private:
    std::string nombre_;
    zmq::context_t contexto_;
    static std::atomic<bool> activo_;
};
