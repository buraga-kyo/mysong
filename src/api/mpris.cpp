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

}  // namespace

}  // namespace mysong::api

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
