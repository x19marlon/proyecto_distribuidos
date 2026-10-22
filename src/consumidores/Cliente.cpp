#include "consumidores/Cliente.hpp"

#include "comun/Utilidades.hpp"

Cliente::Cliente(zmq::context_t& contexto, const std::string& endpointGestor,
                 std::string idEstudiante)
    : gestor_(contexto, endpointGestor, std::chrono::milliseconds(1000), 3),
      idEstudiante_(std::move(idEstudiante)) {}

Mensaje Cliente::enviar(Mensaje peticion) {
    peticion.poner("idEstudiante", idEstudiante_);
    std::optional<Mensaje> respuesta = gestor_.solicitar(peticion);
    if (!respuesta) {
        return Mensaje::error("SIN_RESPUESTA",
                              "El gestor de transacciones no respondió después de 3 intentos");
    }
    return *respuesta;
}

Mensaje Cliente::consultarEventosMes(const std::string& mes) {
    return enviar(Mensaje("consultarEventosMes").poner("mes", mes));
}

Mensaje Cliente::reservar(const std::string& idFecha, int cupos) {
    return enviar(Mensaje("reservar")
                      .poner("idSolicitud", util::generarId("S"))
                      .poner("idFecha", idFecha)
                      .poner("cupos", static_cast<long long>(cupos)));
}
