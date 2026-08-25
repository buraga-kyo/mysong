// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO COMMANDO — src/tui/commando.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada. Uma tecla, uma linha; e tecla que não está na taboada cahe em
// Ordem::Nada, que não chama cousa alguma.
//
// DOMÍNIO ......... a tecla e o Retracto.
// CONTRA-DOMÍNIO .. a Ordem, com o alvo já aparado.
// INVARIANTE ...... funcção pura: não toca tocador, não lê ambiente, não lança.
// Q.E.D. .......... a aparadura entra aqui e não no chamador, donde a tela e o
//                   motor recebem SEMPRE o mesmo numero.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/commando.hpp"

#include <cmath>

namespace mysong::tui {

namespace {

// aparar_busca — o alvo da busca dentro do arco da faixa. Duração que não presta
// dá zero: buscar n'uma faixa cuja duração o mpv ainda não sabe é pedir o
// principio, e não é pedir um numero de sorte.
double aparar_busca(double pedido, double duracao) {
  if (!std::isfinite(duracao) || duracao <= 0.0) return 0.0;
  if (!std::isfinite(pedido) || pedido < 0.0) return 0.0;
  return pedido > duracao ? duracao : pedido;
}

int aparar_volume(int pedido) {
  if (pedido < 0) return 0;
  return pedido > 100 ? 100 : pedido;
}

}  // namespace

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
