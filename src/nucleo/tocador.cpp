// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO TOCADOR, LAVRA — src/nucleo/tocador.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o cabecalho. Não inclue mpv algum: fala com o Motor abstracto e mais
// nada, e é d'ahi que lhe vem a inteira provabilidade.
//
// DOMÍNIO ......... o estado interno, e o Motor emprestado.
// CONTRA-DOMÍNIO .. booleanos, o estado, e o pregão aos ouvintes.
// INVARIANTE ...... nenhum evento sahe antes de o estado que elle retracta já
//                   estar assentado. Quem annuncia lê o estado, e não o
//                   adivinha.
// Q.E.D. .......... nenhuma linha d'esta unidade nomeia mpv, PipeWire ou
//                   arquivo; logo o dublê a exercita inteira, e o que a prova
//                   de som acrescenta é o contracto com a libmpv, e não esta
//                   mechanica.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/tocador.hpp"

#include <utility>

namespace mysong::nucleo {

Tocador::Tocador(Motor& motor) noexcept : motor_(motor) {}

Fila& Tocador::fila() noexcept { return fila_; }
const Fila& Tocador::fila() const noexcept { return fila_; }

// Ouvinte vazio não se guarda: guardá-lo seria adiar para a hora do pregão uma
// verificação que se faz de graça na hora do registro.
void Tocador::escuta(Ouvinte ouvinte) {
  if (ouvinte) ouvintes_.push_back(std::move(ouvinte));
}

// O pregão. Leva o retracto inteiro, e lê o estado em vez de o adivinhar.
void Tocador::annuncia(Aviso aviso, std::string razao) {
  Evento evento;
  evento.aviso = aviso;
  evento.estado = estado_;
  evento.faixa = std::string(fila_.corrente());
  evento.posicao = ultima_posicao_;
  evento.razao = std::move(razao);
  for (const Ouvinte& ouvinte : ouvintes_) ouvinte(evento);
}

// Só annuncia se de facto mudou, e SEMPRE depois de assentar.
void Tocador::assenta_estado(Estado novo) {
  if (novo == estado_) return;
  estado_ = novo;
  annuncia(Aviso::EstadoMudou);
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
