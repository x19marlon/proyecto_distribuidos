#pragma once

#include <memory>
#include <string>

// Entidades del dominio que guarda la capa de persistencia.

// ---------------------------------------------------------------- Lugares

// Un lugar define el límite de cupos de un evento.
class Lugar {
public:
    virtual ~Lugar() = default;
    virtual int capacidad() const = 0;
    virtual std::string descripcion() const = 0;
};

// Edificio o teatro de la universidad, registrado en la base de datos.
// El límite del evento es su capacidad.
class LugarUniversidad : public Lugar {
public:
    enum class Tipo { EDIFICIO, TEATRO };

    LugarUniversidad(std::string id, std::string nombre, Tipo tipo, int capacidad)
        : id_(std::move(id)), nombre_(std::move(nombre)), tipo_(tipo), capacidad_(capacidad) {}

    const std::string& id() const { return id_; }
    const std::string& nombre() const { return nombre_; }
    Tipo tipo() const { return tipo_; }
    int capacidad() const override { return capacidad_; }
    std::string descripcion() const override {
        return nombre_ + (tipo_ == Tipo::TEATRO ? " (teatro)" : " (edificio)");
    }

private:
    std::string id_;
    std::string nombre_;
    Tipo tipo_;
    int capacidad_;
};

// Lugar fuera de la universidad: dirección en texto libre y límite definido a mano.
class LugarExterno : public Lugar {
public:
    LugarExterno(std::string direccion, int capacidad)
        : direccion_(std::move(direccion)), capacidad_(capacidad) {}

    int capacidad() const override { return capacidad_; }
    std::string descripcion() const override { return "Externo: " + direccion_; }

private:
    std::string direccion_;
    int capacidad_;
};

// ---------------------------------------------------------------- Estudiante

class Estudiante {
public:
    struct Datos {
        std::string codigo;  // también es el idEstudiante
        std::string nombre;
        std::string tipoDocumento;
        std::string numeroDocumento;
        std::string carrera;
        std::string departamento;
        int semestre = 0;
        std::string correo;
        std::string telefono;
    };

    explicit Estudiante(Datos datos) : datos_(std::move(datos)) {}

    const std::string& codigo() const { return datos_.codigo; }
    const std::string& nombre() const { return datos_.nombre; }
    const Datos& datos() const { return datos_; }

private:
    Datos datos_;
};

// ---------------------------------------------------------------- Evento y fechas

class Evento {
public:
    Evento(std::string id, std::string nombre, long long precio, std::shared_ptr<const Lugar> lugar)
        : id_(std::move(id)), nombre_(std::move(nombre)), precio_(precio), lugar_(std::move(lugar)) {}

    const std::string& id() const { return id_; }
    const std::string& nombre() const { return nombre_; }
    long long precio() const { return precio_; }  // pesos por cupo; 0 = gratuito
    const Lugar& lugar() const { return *lugar_; }
    int limite() const { return lugar_->capacidad(); }

private:
    std::string id_;
    std::string nombre_;
    long long precio_;
    std::shared_ptr<const Lugar> lugar_;
};

// Una fecha de un evento, con su propio contador de cupos ocupados.
class FechaEvento {
public:
    FechaEvento(std::string id, std::string idEvento, std::string fecha)
        : id_(std::move(id)), idEvento_(std::move(idEvento)), fecha_(std::move(fecha)) {}

    const std::string& id() const { return id_; }
    const std::string& idEvento() const { return idEvento_; }
    const std::string& fecha() const { return fecha_; }  // "AAAA-MM-DD HH:MM"
    std::string mes() const { return fecha_.substr(0, 7); }
    int reservas() const { return reservas_; }

    bool hayCupo(int cupos, int limite) const { return reservas_ + cupos <= limite; }
    void ocupar(int cupos) { reservas_ += cupos; }

private:
    std::string id_;
    std::string idEvento_;
    std::string fecha_;
    int reservas_ = 0;
};

// ---------------------------------------------------------------- Reserva

// Reserva de un estudiante: cuántos cupos en qué fecha de un evento.
class Reserva {
public:
    Reserva(std::string id, std::string idEstudiante, std::string idEvento, std::string idFecha,
            int cupos)
        : id_(std::move(id)), idEstudiante_(std::move(idEstudiante)),
          idEvento_(std::move(idEvento)), idFecha_(std::move(idFecha)), cupos_(cupos) {}

    const std::string& id() const { return id_; }
    const std::string& idEstudiante() const { return idEstudiante_; }
    const std::string& idEvento() const { return idEvento_; }
    const std::string& idFecha() const { return idFecha_; }
    int cupos() const { return cupos_; }

private:
    std::string id_;
    std::string idEstudiante_;
    std::string idEvento_;
    std::string idFecha_;
    int cupos_;
};

// ---------------------------------------------------------------- Transacción

// Registro inmutable de cada reserva hecha.
class Transaccion {
public:
    Transaccion(std::string id, std::string idSolicitud, std::string idReserva, long long monto,
                long long instante)
        : id_(std::move(id)), idSolicitud_(std::move(idSolicitud)),
          idReserva_(std::move(idReserva)), monto_(monto), instante_(instante) {}

    const std::string& id() const { return id_; }
    const std::string& idSolicitud() const { return idSolicitud_; }
    const std::string& idReserva() const { return idReserva_; }
    long long monto() const { return monto_; }  // precio por cupo × cupos
    long long instante() const { return instante_; }

private:
    std::string id_;
    std::string idSolicitud_;
    std::string idReserva_;
    long long monto_;
    long long instante_;
};
