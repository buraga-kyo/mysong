// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LOUSA — src/nucleo/lousa.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro; o filho e o cano no fim.
//
// DOMÍNIO ......... o rectangulo em célullas, e o caminho de uma imagem.
// CONTRA-DOMÍNIO .. linhas de JSON, e um filho que morre com o tocador.
// INVARIANTE ...... funcção alguma d'aqui lança nem espera pelo filho.
// Q.E.D. .......... a composição do JSON não conhece o filho, d'onde a prova do
//                   protocolo corre sem X11 vivo.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/lousa.hpp"

namespace mysong::nucleo {

std::vector<std::string> argumentos_da_lousa() {
  return {"ueberzugpp", "layer", "--silent", "-o", "x11"};
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
