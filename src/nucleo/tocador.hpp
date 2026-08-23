// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO TOCADOR — src/nucleo/tocador.hpp
// ══════════════════════════════════════════════════════════════════════════
// Casa a potencia com a ordem: toma um Motor emprestado e uma Fila propria, e
// traduz «proxima» em «tocar a faixa que vem depois». É aqui que o estado se
// guarda e que o pregão se faz; o motor não sabe de fila, e a fila não sabe de
// som.
//
// DOMÍNIO ......... um Motor, qualquer que seja a sua carne, e ordens de
//                   operador: tocar, proxima, anterior, pausar, retomar,
//                   buscar, volume.
// CONTRA-DOMÍNIO .. booleano a cada ordem, o estado, e o pregão aos ouvintes.
// INVARIANTE ...... o tocador guarda REFERENCIA ao motor, e não a sua
//                   propriedade: quem o construiu ha de manter o motor vivo
//                   mais tempo que o tocador. Todo evento sahe DEPOIS de o
//                   estado interno já ter mudado, de sorte que o ouvinte nunca
//                   vê um retracto que já não vale.
// Q.E.D. .......... sendo o Motor abstracto, esta classe inteira se prova com
//                   um dublê em machina surda; e sendo o ouvinte um functor
//                   que se registra, o tocador emitte sem conhecer quem ouve.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include "nucleo/fila.hpp"
#include "nucleo/motor.hpp"

namespace mysong::nucleo {

class Tocador {
 public:
  explicit Tocador(Motor& motor) noexcept;

  Tocador(const Tocador&) = delete;
  Tocador& operator=(const Tocador&) = delete;

 private:
  Motor& motor_;
  Fila fila_;
};

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
