// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MPRIS — src/api/mpris.cpp
// ══════════════════════════════════════════════════════════════════════════
// O encanamento da `libdbus`. A traducção de unidades NÃO vive aqui: vive em
// `unidades.cpp`, e é lá que ella se prova.
//
// DOMÍNIO ......... mensagens do barramento de sessão.
// CONTRA-DOMÍNIO .. respostas, e o pregão `PropertiesChanged`.
// INVARIANTE ...... mensagem desconhecida recebe ERRO NOMEADO, e nunca silencio:
//                   cliente que espera resposta e não a recebe fica pendurado no
//                   seu proprio prazo, e o operador vê o playerctl a travar sem
//                   razão dita.
// Q.E.D. .......... barramento ausente devolve uma Casa morta com razão, e o
//                   tocador segue: é o mesmo padrão do analisador da issue #5.
// ══════════════════════════════════════════════════════════════════════════
#include "api/mpris.hpp"

#include <dbus/dbus.h>

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include "api/unidades.hpp"

namespace mysong::api {

namespace {

constexpr const char* kNome = "org.mpris.MediaPlayer2.mysong";
constexpr const char* kCaminho = "/org/mpris/MediaPlayer2";
constexpr const char* kRaiz = "org.mpris.MediaPlayer2";
constexpr const char* kTocador = "org.mpris.MediaPlayer2.Player";
constexpr const char* kPropriedades = "org.freedesktop.DBus.Properties";


// ── OS ESCRIPTORES de valores. Um por typo, para que a fórma se escreva UMA vez.

void escreve_texto(DBusMessageIter* pae, const std::string& valor) {
  const char* cru = valor.c_str();
  dbus_message_iter_append_basic(pae, DBUS_TYPE_STRING, &cru);
}

// escreve_variante — o valor embrulhado n'uma variante, que é o que o `Get` devolve.
// A assignatura entra como cadeia porque a `libdbus` a pede assim.
void escreve_variante_texto(DBusMessageIter* pae, const std::string& valor) {
  DBusMessageIter dentro;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "s", &dentro);
  escreve_texto(&dentro, valor);
  dbus_message_iter_close_container(pae, &dentro);
}

void escreve_variante_caminho(DBusMessageIter* pae, const std::string& valor) {
  DBusMessageIter dentro;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "o", &dentro);
  const char* cru = valor.c_str();
  dbus_message_iter_append_basic(&dentro, DBUS_TYPE_OBJECT_PATH, &cru);
  dbus_message_iter_close_container(pae, &dentro);
}

void escreve_variante_int64(DBusMessageIter* pae, std::int64_t valor) {
  DBusMessageIter dentro;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "x", &dentro);
  dbus_int64_t cru = valor;
  dbus_message_iter_append_basic(&dentro, DBUS_TYPE_INT64, &cru);
  dbus_message_iter_close_container(pae, &dentro);
}

void escreve_variante_duplo(DBusMessageIter* pae, double valor) {
  DBusMessageIter dentro;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "d", &dentro);
  dbus_message_iter_append_basic(&dentro, DBUS_TYPE_DOUBLE, &valor);
  dbus_message_iter_close_container(pae, &dentro);
}

void escreve_variante_bool(DBusMessageIter* pae, bool valor) {
  DBusMessageIter dentro;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "b", &dentro);
  dbus_bool_t cru = valor ? TRUE : FALSE;
  dbus_message_iter_append_basic(&dentro, DBUS_TYPE_BOOLEAN, &cru);
  dbus_message_iter_close_container(pae, &dentro);
}


// escreve_metadados — o `a{sv}` do `Metadata`. Quatro chaves, e as quatro que o
// `playerctl metadata` mostra por defeito.
void escreve_metadados(DBusMessageIter* pae, const nucleo::Tocador& tocador) {
  DBusMessageIter variante, mapa;
  dbus_message_iter_open_container(pae, DBUS_TYPE_VARIANT, "a{sv}", &variante);
  dbus_message_iter_open_container(&variante, DBUS_TYPE_ARRAY, "{sv}", &mapa);

  const nucleo::Fila& fila = tocador.fila();
  const bool ha = !fila.vazia();
  const std::string caminho(ha ? fila.corrente() : std::string_view());

  const auto par = [&mapa](const char* chave, auto escriptor) {
    DBusMessageIter entrada;
    dbus_message_iter_open_container(&mapa, DBUS_TYPE_DICT_ENTRY, nullptr,
                                     &entrada);
    dbus_message_iter_append_basic(&entrada, DBUS_TYPE_STRING, &chave);
    escriptor(&entrada);
    dbus_message_iter_close_container(&mapa, &entrada);
  };

  par("mpris:trackid", [&](DBusMessageIter* onde) {
    escreve_variante_caminho(onde, caminho_da_faixa(ha ? fila.indice() : 0, ha));
  });
  if (ha) {
    par("xesam:url", [&](DBusMessageIter* onde) {
      escreve_variante_texto(onde, url_do_arquivo(caminho));
    });
    par("xesam:title", [&](DBusMessageIter* onde) {
      // O titulo é o NOME do arquivo, e não o caminho: é o que o operador lê no
      // painel do systema, e o caminho inteiro alli não caberia.
      escreve_variante_texto(onde,
                             std::filesystem::path(caminho).filename().string());
    });
    par("mpris:length", [&](DBusMessageIter* onde) {
      escreve_variante_int64(onde, segundos_para_micros(tocador.duracao()));
    });
  }
  dbus_message_iter_close_container(&variante, &mapa);
  dbus_message_iter_close_container(pae, &variante);
}

}  // namespace

}  // namespace mysong::api

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
