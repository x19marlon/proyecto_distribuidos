#include "comun/Servicio.hpp"

#include <cerrno>
#include <csignal>

#include "comun/Utilidades.hpp"

std::atomic<bool> Servicio::activo_{true};

namespace {
void manejarSenal(int) { Servicio::solicitarDetencion(); }
}  // namespace

Servicio::Servicio(std::string nombre) : nombre_(std::move(nombre)), contexto_(1) {}

int Servicio::ejecutar() {
    std::signal(SIGINT, manejarSenal);
    std::signal(SIGTERM, manejarSenal);
    try {
        inicializar();
        log("iniciado");
        while (activo_) {
            try {
                iterar();
            } catch (const zmq::error_t& e) {
                if (e.num() != EINTR) throw;  // una señal interrumpió el poll
            }
        }
        finalizar();
    } catch (const std::exception& e) {
        log(std::string("error fatal: ") + e.what());
        return 1;
    }
    log("detenido");
    return 0;
}

void Servicio::log(const std::string& texto) const { util::log(nombre_, texto); }
