// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA MARCA, LAVRA — src/nucleo/marca.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o que o cabecalho promette: guarda o texto da marca e o entrega.
//
// DOMÍNIO ......... o vacuo, como declarado no tractado do cabecalho.
// CONTRA-DOMÍNIO .. vista sobre o literal abaixo, que o compilador aloja em
//                   memoria de duração estática e somente-leitura.
// INVARIANTE ...... esta unidade de traducção é a ÚNICA no reino inteiro que
//                   escreve o texto da marca. Quem o quiser, chame marca();
//                   quem o repetir em literal proprio, quebra a invariante.
// Q.E.D. .......... ASCII puro, de proposito: nenhum glifo de Nerd Font, nenhum
//                   separador arrowline. Assim a marca se mostra igual em
//                   terminal pelado, e a prova de fumo não depende de fonte
//                   installada no systema de quem a roda.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/marca.hpp"

namespace mysong::nucleo {

std::string_view marca() noexcept {
  return "mysong";
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
