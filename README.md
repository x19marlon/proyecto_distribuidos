# proyecto_distribuidos

Proyecto final de Sistemas Distribuidos – equipo **crotolamo que gico**.

## ¿Qué es?

Un sistema distribuido de transacciones para los eventos de la universidad. Los
estudiantes pueden consultar los eventos de un mes, reservar cupos, modificar su
reserva y cancelarla (de forma asíncrona). Cada operación queda registrada como una
transacción, administrada por la clase `GestorTransacciones`.

El sistema tiene tres capas que corren como procesos separados y se comunican con
**ZeroMQ**:

```
Consumidores  ──►  Servicios de Negocio  ──►  Servicios de Almacenamiento
```

## Primera entrega

Implementación en **C++17 orientado a objetos** de la arquitectura base
([Figura 1](DOCS/diagramas/diagrama_proyecto.png)) con las operaciones de
**consulta de eventos del mes** y **reserva de cupos**. El informe en LaTeX está en
[informe/](informe/main.tex) y el detalle de la implementación en
[DOCS/implementacion.md](DOCS/implementacion.md).

### Componentes

Cada componente es un proceso independiente:

| Componente (Figura 1) | Ejecutable | Clases principales | Socket ZeroMQ |
|---|---|---|---|
| Clientes | `bin/cliente` | `Cliente`, `ConsolaCliente` | REQ → gestor :5555 |
| Gestor de Transacciones | `bin/gestor-transacciones` | `GestorTransacciones` | ROUTER :5555 (clientes), ROUTER :5556 (procesadores) |
| Procesamiento RMC | `bin/procesador-rmc` | `ProcesadorRMC` | DEALER → gestor :5556, REQ → persistencia :5560 |
| Servicio de Persistencia | `bin/persistencia` | `ServicioPersistencia`, `BaseDatos`, `AlmacenamientoArchivo` | REP :5560 |

```
src/comun/          Mensaje, ClienteConfiable, Servicio (clase base), utilidades
src/consumidores/   cliente del estudiante (menú por consola)
src/servicios/      gestor/ (GestorTransacciones) y rmc/ (ProcesadorRMC)
src/persistencia/   modelo de dominio, base de datos y bitácora en disco
scripts/            iniciar y detener el sistema
```

### Requisitos

- Linux (probado en Ubuntu)
- `g++` con soporte de C++17 y `make`
- ZeroMQ: `libzmq3-dev` y los encabezados de C++ `cppzmq-dev`

```bash
sudo apt install g++ make libzmq3-dev cppzmq-dev
```

Si ZeroMQ está instalado en otra carpeta (por ejemplo, compilado sin `sudo`), se
indica al compilar con `make ZMQ_PREFIX=/ruta/de/zeromq`.

### Compilar

Desde esta carpeta:

```bash
make            # genera bin/cliente, bin/gestor-transacciones, bin/procesador-rmc y bin/persistencia
make limpiar    # borra build/ y bin/
```

### Ejecutar en una sola máquina

1. **Levantar los servicios** (persistencia, gestor y procesadores RMC):

   ```bash
   scripts/iniciar_sistema.sh base        # 1 procesador RMC (arquitectura base)
   scripts/iniciar_sistema.sh escalable   # o 3 procesadores RMC
   ```

   Los registros de cada proceso quedan en `logs/` y los datos en `datos/bitacora.txt`.

2. **Abrir un cliente** (uno por terminal; se pueden abrir varios a la vez):

   ```bash
   bin/cliente EST-1
   ```

   Los estudiantes `EST-1`, `EST-2` y `EST-3` ya están registrados.

3. **Detener los servicios:**

   ```bash
   scripts/detener_sistema.sh
   ```

#### Ejemplo de uso

```
 1. Consultar eventos de un mes
 2. Reservar cupos
 0. Salir
Opción: 1
Mes (AAAA-MM): 2026-11
  F-1  2026-11-05 18:00  Concierto de bienvenida – Teatro Mayor (teatro)
        cupos: 0/300 ocupados, 300 disponibles; precio por cupo: $0
  F-2  2026-11-12 18:00  Concierto de bienvenida – Teatro Mayor (teatro)
        cupos: 0/300 ocupados, 300 disponibles; precio por cupo: $0
  F-3  2026-11-20 14:00  Taller de ZeroMQ – Sala 301 Edificio Central (edificio)
        cupos: 0/5 ocupados, 5 disponibles; precio por cupo: $15000
  F-5  2026-11-28 09:00  Feria de emprendimiento – Externo: Plaza de Bolívar, Bogotá
        cupos: 0/50 ocupados, 50 disponibles; precio por cupo: $0
Opción: 2
Id de la fecha (por ejemplo F-1): F-3
Cantidad de cupos: 2
Reserva R-1 creada: 2 cupos, valor $30000. Quedan 3 cupos.
Opción: 2
Id de la fecha (por ejemplo F-1): F-3
Cantidad de cupos: 1
Error RESERVA_DUPLICADA: Ya tienes la reserva R-1 en este evento
```

Los eventos de ejemplo están en **noviembre (`2026-11`) y diciembre (`2026-12`) de 2026**.
La fecha `F-3` (Taller de ZeroMQ, Sala 301) tiene solo 5 cupos y sirve para probar
el error `CUPOS_INSUFICIENTES`.

#### Errores que puede devolver el sistema

| Código | Causa |
|---|---|
| `DATOS_INVALIDOS` | mes mal escrito (debe ser `AAAA-MM`) o cupos ≤ 0 |
| `ESTUDIANTE_NO_REGISTRADO` | el id del estudiante no existe |
| `FECHA_NO_EXISTE` | el id de la fecha no existe |
| `RESERVA_DUPLICADA` | el estudiante ya tiene una reserva en ese evento |
| `CUPOS_INSUFICIENTES` | no quedan cupos suficientes en la fecha |
| `SIN_PROCESADORES` | no hay procesadores RMC conectados al gestor |
| `SIN_RESPUESTA` | el gestor no respondió después de 3 intentos |
| `PERSISTENCIA_NO_DISPONIBLE` | el procesador RMC no pudo comunicarse con persistencia |

### Ejecutar en varias máquinas

Cada proceso lee sus direcciones de variables de entorno (entre paréntesis, el valor
por defecto). Ejemplo con el gestor en `192.168.0.10` y persistencia en `192.168.0.12`:

```bash
# Máquina de almacenamiento
bin/persistencia                                    # PERSISTENCIA_BIND (tcp://*:5560), ARCHIVO_DATOS (datos/bitacora.txt)

# Máquina del gestor
bin/gestor-transacciones                            # GESTOR_FRONTEND_BIND (tcp://*:5555), GESTOR_BACKEND_BIND (tcp://*:5556)

# Máquina de procesamiento (uno o más procesadores)
GESTOR_BACKEND=tcp://192.168.0.10:5556 \
PERSISTENCIA_ENDPOINT=tcp://192.168.0.12:5560 \
bin/procesador-rmc

# Máquina de clientes
GESTOR_ENDPOINT=tcp://192.168.0.10:5555 bin/cliente EST-1
```

Los puertos 5555, 5556 y 5560 deben estar abiertos en el firewall.

### Notas

- La persistencia aplica cada reserva de forma atómica y la escribe en
  `datos/bitacora.txt` antes de responder; al reiniciarla, recupera las reservas.
  Para empezar con datos limpios, borre la carpeta `datos/` con el sistema detenido.
- Esta entrega no implementa tolerancia a fallos como requisito; aun así, el cliente
  reintenta 3 veces si el gestor no responde y el gestor deja de usar un procesador
  RMC que no envía latidos en 3 segundos.

## Integrantes

- mariananvv16
- martinezm-oe
- Nick_07
- x19marlon

## Documentación

- Diseño completo del sistema: [DOCS/](DOCS/README.md)
- Informe de la primera entrega (LaTeX, compilable en Overleaf con pdfLaTeX): [informe/](informe/main.tex)
