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


// escreve_propriedade — UMA propriedade, pelo nome. Falso quando o nome não é d'esta
// Casa, e ahi quem chama devolve o erro nomeado que a especificação pede.
bool escreve_propriedade(DBusMessageIter* pae, const std::string& interface,
                         const std::string& nome, nucleo::Tocador& tocador) {
  if (interface == kRaiz) {
    if (nome == "Identity") { escreve_variante_texto(pae, "mysong"); return true; }
    if (nome == "DesktopEntry") { escreve_variante_texto(pae, "mysong"); return true; }
    // As tres que o playerctl consulta antes de commandar. `CanQuit` falso e
    // `CanRaise` falso são a verdade: esta Casa não sahe nem se ergue por commando de
    // fóra, e mentir alli faria o cliente pedir o que não se faz.
    if (nome == "CanQuit") { escreve_variante_bool(pae, false); return true; }
    if (nome == "CanRaise") { escreve_variante_bool(pae, false); return true; }
    if (nome == "HasTrackList") { escreve_variante_bool(pae, false); return true; }
    return false;
  }
  if (interface != kTocador) return false;
  if (nome == "PlaybackStatus") {
    escreve_variante_texto(pae, std::string(estado_do_mpris(tocador.estado())));
    return true;
  }
  if (nome == "Metadata") { escreve_metadados(pae, tocador); return true; }
  if (nome == "Position") {
    escreve_variante_int64(pae, segundos_para_micros(tocador.posicao()));
    return true;
  }
  if (nome == "Volume") {
    escreve_variante_duplo(pae, porcento_para_volume(tocador.volume()));
    return true;
  }
  if (nome == "Rate" || nome == "MinimumRate" || nome == "MaximumRate") {
    escreve_variante_duplo(pae, 1.0);  // esta Casa não muda a velocidade
    return true;
  }
  // As seis capacidades. Todas verdadeiras menos a de girar a lista, que não ha.
  if (nome == "CanGoNext" || nome == "CanGoPrevious" || nome == "CanPlay" ||
      nome == "CanPause" || nome == "CanSeek" || nome == "CanControl") {
    escreve_variante_bool(pae, true);
    return true;
  }
  return false;
}


// cumpre_metodo — os oito metodos do MPRIS. Falso quando o nome não é d'esta Casa.
//
// As duas armadilhas que a issue nomeou: `Seek` é RELATIVO e `SetPosition` é
// ABSOLUTO. Trocá-los faria a tecla de avanço saltar para o segundo cinco em vez de
// avançar cinco segundos, e o defeito passaria por «funciona mal» em vez de «está
// trocado».
bool cumpre_metodo(const std::string& nome, DBusMessage* mensagem,
                   nucleo::Tocador& tocador) {
  if (nome == "Play") { tocador.retomar(); return true; }
  if (nome == "Pause") { tocador.pausar(); return true; }
  if (nome == "Stop") { tocador.pausar(); return true; }
  if (nome == "PlayPause") {
    if (tocador.estado() == nucleo::Estado::Tocando) tocador.pausar();
    else tocador.retomar();
    return true;
  }
  if (nome == "Next") { tocador.proxima(); return true; }
  if (nome == "Previous") { tocador.anterior(); return true; }

  if (nome == "Seek") {
    dbus_int64_t delta = 0;
    DBusMessageIter leitor;
    if (dbus_message_iter_init(mensagem, &leitor) &&
        dbus_message_iter_get_arg_type(&leitor) == DBUS_TYPE_INT64) {
      dbus_message_iter_get_basic(&leitor, &delta);
      // RELATIVO: soma-se á posição corrente. E o alvo apara-se aqui, que o delta
      // pode ser negativo e maior que a posição.
      double alvo = tocador.posicao() + micros_para_segundos(
                        delta < 0 ? -delta : delta) * (delta < 0 ? -1.0 : 1.0);
      if (alvo < 0.0) alvo = 0.0;
      tocador.buscar(alvo);
    }
    return true;
  }
  if (nome == "SetPosition") {
    DBusMessageIter leitor;
    if (dbus_message_iter_init(mensagem, &leitor) &&
        dbus_message_iter_get_arg_type(&leitor) == DBUS_TYPE_OBJECT_PATH) {
      // O `trackid` ignora-se de proposito: esta Casa tem UMA faixa corrente, e
      // recusar por trackid alheio faria o playerctl parecer roto sem razão.
      dbus_message_iter_next(&leitor);
      dbus_int64_t micros = 0;
      if (dbus_message_iter_get_arg_type(&leitor) == DBUS_TYPE_INT64) {
        dbus_message_iter_get_basic(&leitor, &micros);
        tocador.buscar(micros_para_segundos(micros));  // ABSOLUTO
      }
    }
    return true;
  }
  return false;
}

}  // namespace

}  // namespace mysong::api

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
