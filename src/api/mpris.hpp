// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MPRIS, src/api/mpris.hpp
// ══════════════════════════════════════════════════════════════════════════
// Publica `org.mpris.MediaPlayer2.mysong` no barramento de sessão. Com elle, o
// `playerctl` commanda o mysong; e com o `playerctl`, as teclas de midia que o
// RADICAL-OS já tem ligadas em `mappings/global_keys.lua`. Linha nova de Lua
// alguma naquelle repo: é o que esta issue compra.
//
// DOMÍNIO ......... um Tocador emprestado, e as mensagens do barramento.
// CONTRA-DOMÍNIO .. respostas no barramento, e `PropertiesChanged` quando muda.
// INVARIANTE ...... barramento AUSENTE não é falha. Quem corre o mysong n'uma
//                   sessão sem D-Bus continua a ter tocador; o que perde é o
//                   commando de fóra, e diz-se-lhe isso uma vez.
// Q.E.D. .......... a traducção de unidades vive em `unidades.cpp`, onde o `grep
//                   "#include.*dbus"` sahe vazio; donde o que mais erra prova-se em
//                   machina surda, e aqui fica sómente o encanamento.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <memory>
#include <string>

#include "nucleo/tocador.hpp"

namespace mysong::api {

// A CASA DO MPRIS. Ergue-se, e depois pulsa-se: como o resto d'esta obra, quem chama
// é dono do seu relogio, e nada aqui abre fio proprio.
class CasaDoMpris {
 public:
  explicit CasaDoMpris(nucleo::Tocador& tocador);
  ~CasaDoMpris();

  CasaDoMpris(const CasaDoMpris&) = delete;
  CasaDoMpris& operator=(const CasaDoMpris&) = delete;

  bool viva() const noexcept;
  const std::string& razao() const noexcept;

  // Uma batida: lê o que chegou e responde. NÃO bloqueia.
  void pulsa();

 // O punho é PUBLICO de proposito. As funcções que respondem ás mensagens vivem no
  // namespace anonymo do `.cpp`, que é onde devem viver, e por isso não podem ser
  // amigas d'esta classe. Publicá-lo é mais honesto que declarar meia duzia de
  // `friend`: o typo é incompleto aqui, donde de fóra d'este reino ninguem lhe toca.
  struct Punho;
  std::unique_ptr<Punho> punho_;
};

}  // namespace mysong::api

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
