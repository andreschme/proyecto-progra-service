#include <iostream>
#include <vector>
#include <fstream>

#include "librerias/Alumno.h"
#include "librerias/httplib.h"
#include "librerias/json.hpp"

using namespace std;
using namespace httplib;

vector<Alumno> alumnos_db;
int proximo_id = 1;
mutex db_mutex;
const string archivo_datos = "alumnos.dat";

void guardar_datos_con_lock_adquirido() {
    json j_array = json::array();
    for (const auto &v: alumnos_db) {
        j_array.push_back(v);
    }

    ofstream archivo(archivo_datos);
    if (archivo.is_open()) {
        archivo << j_array.dump(4);
        archivo.close();
    } else {
        cerr << "Error al abrir " << archivo_datos << " para escritura." << endl;
    }
}


void cargar_datos() {
    lock_guard<mutex> lock(db_mutex);

    ifstream archivo(archivo_datos);
    if (archivo.is_open()) {
        try {
            json j_array_leido;
            archivo >> j_array_leido;
            if (j_array_leido.is_array()) {
                for (const auto &j_alumno: j_array_leido) {
                    Alumno v = j_alumno.get<Alumno>();
                    alumnos_db.push_back(v);

                    if (v.id >= proximo_id) {
                        proximo_id = v.id + 1;
                    }
                }
            }
            cout << "Alumnos cargados desde " << archivo_datos << endl;
        } catch (json::parse_error &e) {
            cerr << "Error al parsear JSON desde " << archivo_datos << ": " << e.what() << endl;
        } catch (json::type_error &e) {
            cerr << "Error de tipo JSON desde " << archivo_datos << ": " << e.what() << endl;
        }
        archivo.close();
    } else {
        cout << "No se encontró " << archivo_datos << ". Se iniciará con una base de datos vacía." << endl;
    }


    if (alumnos_db.empty() && proximo_id < 1) {
        proximo_id = 1;
    } else if (!alumnos_db.empty()) {
        int max_id = 0;
        for (const auto &v: alumnos_db) {
            if (v.id > max_id) max_id = v.id;
        }
        if (proximo_id <= max_id) {
            proximo_id = max_id + 1;
        }
    }
}


int main(void) {
    Server svr;

    cargar_datos();

    //metodo para crear alumnos
    svr.Post("/alumno", [&](const Request &req, Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        try {
            json j_body = json::parse(req.body);
            Alumno v_nuevo;

            cout << "Guardando Alumno" << j_body << endl;

            if (!j_body.contains("nombre") || !j_body.contains("edad") || !j_body.contains("genero") || !j_body.
                contains("grado")) {
                res.status = 400;
                res.set_content("Faltan campos obligatorios: nombre, edad, genero, grado", "text/plain; charset=utf-8");
                return;
            }
            from_json(j_body, v_nuevo);

            lock_guard<mutex> lock(db_mutex);
            v_nuevo.id = proximo_id++;
            alumnos_db.push_back(v_nuevo);
            guardar_datos_con_lock_adquirido();

            json j_respuesta = v_nuevo;
            res.set_content(j_respuesta.dump(4), "application/json; charset=utf-8");
            res.status = 201;
        } catch (json::parse_error &e) {
            res.status = 400;
            res.set_content("JSON mal formado: " + string(e.what()), "text/plain; charset=utf-8");
        } catch (json::type_error &e) {
            res.status = 400;
            res.set_content("Error en el tipo de datos del JSON o campo faltante: " + string(e.what()),
                            "text/plain; charset=utf-8");
        } catch (const exception &e) {
            res.status = 500;
            res.set_content("Error interno del servidor: " + string(e.what()), "text/plain; charset=utf-8");
        }
    });

    //metodo para recuperar todos los alumnos
    svr.Get("/alumnos", [&](const Request &req, Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        lock_guard<mutex> lock(db_mutex);
        json j_array_respuesta = alumnos_db;
        res.set_content(j_array_respuesta.dump(4), "application/json; charset=utf-8");
    });

    // metodo para eliminar alumno
    svr.Delete(R"(/alumno/(\d+))", [&](const Request& req, Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        int id_alumno = stoi(req.matches[1].str());

        lock_guard<mutex> lock(db_mutex);
        auto it = find_if(alumnos_db.begin(), alumnos_db.end(),
                               [id_alumno](const Alumno& v){ return v.id == id_alumno; });

        if (it != alumnos_db.end()) {
            alumnos_db.erase(it);
            guardar_datos_con_lock_adquirido();
            res.status = 204;
        } else {
            res.status = 404;
            res.set_content("Alumno no encontrado", "text/plain; charset=utf-8");
        }
    });

    // metodo para recupear un alumno por id
    svr.Get(R"(/alumno/(\d+))", [&](const Request &req, Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        int id_alumno = stoi(req.matches[1].str());

        lock_guard<mutex> lock(db_mutex);
        auto it = find_if(alumnos_db.begin(), alumnos_db.end(),
                               [id_alumno](const Alumno& v){ return v.id == id_alumno; });

        if (it != alumnos_db.end()) {
            json j_alumno_respuesta = *it;
            res.set_content(j_alumno_respuesta.dump(4), "application/json; charset=utf-8");
        } else {
            res.status = 404;
            res.set_content("Alumno no encontrado", "text/plain; charset=utf-8");
        }
    });

    //metodo para actualizar alumno
    svr.Put(R"(/alumno/(\d+))", [&](const Request &req, Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");

        int id_alumno = stoi(req.matches[1].str());

        try {
            json j_actualizacion = json::parse(req.body);

            lock_guard<mutex> lock(db_mutex);
            auto it = find_if(alumnos_db.begin(), alumnos_db.end(),
                                   [id_alumno](const Alumno& v){ return v.id == id_alumno; });

            if (it != alumnos_db.end()) {
                if (j_actualizacion.contains("nombre")) it->nombre = j_actualizacion["nombre"].get<string>();
                if (j_actualizacion.contains("edad")) it->edad = j_actualizacion["edad"].get<int>();
                if (j_actualizacion.contains("genero")) it->genero = j_actualizacion["genero"].get<string>();
                if (j_actualizacion.contains("grado")) it->grado = j_actualizacion["grado"].get<string>();

                guardar_datos_con_lock_adquirido();

                json j_respuesta = *it;
                res.set_content(j_respuesta.dump(4), "application/json; charset=utf-8");
            } else {
                res.status = 404;
                res.set_content("Alumno no encontrado", "text/plain; charset=utf-8");
            }
        } catch (json::parse_error& e) {
            res.status = 400;
            res.set_content("JSON mal formado: " + string(e.what()), "text/plain; charset=utf-8");
        } catch (json::type_error& e) {
            res.status = 400;
            res.set_content("Error en el tipo de datos del JSON o campo faltante: " + string(e.what()), "text/plain; charset=utf-8");
        } catch (const exception& e) {
            res.status = 500;
            res.set_content("Error interno del servidor: " + string(e.what()), "text/plain; charset=utf-8");
        }
    });

    svr.Options(R"(.*)", [](const Request &req, Response &res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type");
        res.status = 204;
    });

    cout << "Servidor CRUD de Alumnos iniciando en http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);

    return 0;
}
