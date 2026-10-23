#include "persistencia/BaseDatos.hpp"

#include <algorithm>

// ---------------------------------------------------------------- Datos de ejemplo

void BaseDatos::cargarDatosEjemplo() {
    using Tipo = LugarUniversidad::Tipo;
    auto teatro = std::make_shared<LugarUniversidad>("L-1", "Teatro Mayor", Tipo::TEATRO, 300);
    auto auditorio =
        std::make_shared<LugarUniversidad>("L-2", "Auditorio de Ingeniería", Tipo::EDIFICIO, 120);
    auto sala = std::make_shared<LugarUniversidad>("L-3", "Sala 301 Edificio Central",
                                                   Tipo::EDIFICIO, 5);
    agregarLugar(teatro);
    agregarLugar(auditorio);
    agregarLugar(sala);

    agregarEvento(Evento("EV-1", "Concierto de bienvenida", 0, teatro),
                  {"2026-11-05 18:00", "2026-11-12 18:00"});
    agregarEvento(Evento("EV-2", "Taller de ZeroMQ", 15000, sala),
                  {"2026-11-20 14:00", "2026-12-03 14:00"});
    agregarEvento(Evento("EV-3", "Feria de emprendimiento", 0,
                         std::make_shared<LugarExterno>("Plaza de Bolívar, Bogotá", 50)),
                  {"2026-11-28 09:00"});
    agregarEvento(Evento("EV-4", "Conferencia de sistemas distribuidos", 5000, auditorio),
                  {"2026-12-10 10:00", "2026-12-11 10:00"});

    agregarEstudiante({"EST-1", "Ana Gómez", "CC", "1010101010", "Ingeniería de Sistemas",
                       "Ingeniería", 8, "ana.gomez@universidad.edu.co", "3001112233"});
    agregarEstudiante({"EST-2", "Juan Pérez", "CC", "1020202020", "Ingeniería Electrónica",
                       "Ingeniería", 6, "juan.perez@universidad.edu.co", "3002223344"});
    agregarEstudiante({"EST-3", "Laura Martínez", "TI", "1030303030", "Administración",
                       "Ciencias Económicas", 2, "laura.martinez@universidad.edu.co",
                       "3003334455"});
}

void BaseDatos::agregarLugar(std::shared_ptr<LugarUniversidad> lugar) {
    lugares_[lugar->id()] = std::move(lugar);
}

void BaseDatos::agregarEvento(Evento evento, const std::vector<std::string>& fechas) {
    for (const std::string& fecha : fechas) {
        const std::string id = "F-" + std::to_string(siguienteFecha_++);
        fechas_.emplace(id, FechaEvento(id, evento.id(), fecha));
    }
    const std::string id = evento.id();
    eventos_.emplace(id, std::move(evento));
}

void BaseDatos::agregarEstudiante(Estudiante::Datos datos) {
    const std::string codigo = datos.codigo;
    estudiantes_.emplace(codigo, Estudiante(std::move(datos)));
}

// ---------------------------------------------------------------- Consulta

Fila BaseDatos::filaFecha(const FechaEvento& fecha) const {
    const Evento& evento = eventos_.at(fecha.idEvento());
    return {{"idFecha", fecha.id()},
            {"idEvento", evento.id()},
            {"evento", evento.nombre()},
            {"fecha", fecha.fecha()},
            {"lugar", evento.lugar().descripcion()},
            {"precio", std::to_string(evento.precio())},
            {"limite", std::to_string(evento.limite())},
            {"reservas", std::to_string(fecha.reservas())},
            {"disponibles", std::to_string(evento.limite() - fecha.reservas())}};
}

Mensaje BaseDatos::listarFechasMes(const Mensaje& peticion) const {
    const std::string mes = peticion.texto("mes");
    std::vector<const FechaEvento*> encontradas;
    for (const auto& [id, fecha] : fechas_) {
        if (fecha.mes() == mes) encontradas.push_back(&fecha);
    }
    std::sort(encontradas.begin(), encontradas.end(),
              [](const FechaEvento* a, const FechaEvento* b) { return a->fecha() < b->fecha(); });

    Mensaje respuesta("OK");
    for (const FechaEvento* fecha : encontradas) respuesta.agregarFila(filaFecha(*fecha));
    respuesta.poner("total", static_cast<long long>(encontradas.size()));
    return respuesta;
}

// ---------------------------------------------------------------- Reserva

Mensaje BaseDatos::crearReserva(const Mensaje& peticion) {
    const std::string idEstudiante = peticion.texto("idEstudiante");
    const std::string idFecha = peticion.texto("idFecha");
    const int cupos = static_cast<int>(peticion.entero("cupos"));

    // 1. Validaciones (no cambian nada)
    if (estudiantes_.count(idEstudiante) == 0) {
        return Mensaje::error("ESTUDIANTE_NO_REGISTRADO",
                              "El estudiante " + idEstudiante + " no está registrado");
    }
    auto itFecha = fechas_.find(idFecha);
    if (itFecha == fechas_.end()) {
        return Mensaje::error("FECHA_NO_EXISTE", "No existe la fecha " + idFecha);
    }
    FechaEvento& fecha = itFecha->second;
    const Evento& evento = eventos_.at(fecha.idEvento());

    const std::string clave = idEstudiante + "|" + evento.id();
    if (auto it = reservaPorEvento_.find(clave); it != reservaPorEvento_.end()) {
        return Mensaje::error("RESERVA_DUPLICADA",
                              "Ya tienes la reserva " + it->second + " en este evento");
    }
    if (!fecha.hayCupo(cupos, evento.limite())) {
        return Mensaje::error("CUPOS_INSUFICIENTES",
                              "Quedan " + std::to_string(evento.limite() - fecha.reservas()) +
                                  " cupos en esta fecha");
    }

    // 2. Cambios (ya no puede fallar nada)
    const std::string idReserva = "R-" + std::to_string(siguienteReserva_++);
    reservas_.emplace(idReserva, Reserva(idReserva, idEstudiante, evento.id(), idFecha, cupos));
    fecha.ocupar(cupos);
    reservaPorEvento_[clave] = idReserva;
    const long long monto = evento.precio() * cupos;
    transacciones_.emplace_back("T-" + std::to_string(siguienteTransaccion_++),
                                peticion.texto("idSolicitud"), idReserva, monto,
                                peticion.entero("instante"));

    return Mensaje("OK")
        .poner("idReserva", idReserva)
        .poner("idFecha", idFecha)
        .poner("cupos", static_cast<long long>(cupos))
        .poner("monto", monto)
        .poner("disponibles", static_cast<long long>(evento.limite() - fecha.reservas()));
}

// ---------------------------------------------------------------- Idempotencia

const Mensaje* BaseDatos::respuestaPrevia(const std::string& clave) const {
    auto it = respuestas_.find(clave);
    return it == respuestas_.end() ? nullptr : &it->second;
}

void BaseDatos::guardarRespuesta(const std::string& clave, const Mensaje& respuesta) {
    respuestas_[clave] = respuesta;
}
