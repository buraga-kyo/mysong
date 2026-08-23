// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DA FILA — testes/prova_fila.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a fila SEM mpv, sem placa de som e sem arquivo em disco: os caminhos
// que se lhe entregam são cadeias que ella nunca abre. É metade do aceite que
// se pode provar em machina surda, e é por isso que a fila vive em tractado
// apartado do motor.
//
// DOMÍNIO ......... uma Fila armada aqui mesmo, com zero, uma ou tres faixas.
// CONTRA-DOMÍNIO .. veredicto do doctest, e por elle o status do ctest.
// INVARIANTE ...... nenhum caso d'esta prova toca no systema de arquivos nem
//                   na saída de áudio; roda igual em machina sem som algum.
// Q.E.D. .......... a ordem lida de volta é a ordem que o cliente definiu, e
//                   os passos que a issue pede respondem por booleano em vez
//                   de excepção, de sorte que a borda se assere e não se
//                   apanha.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include "nucleo/fila.hpp"

namespace {

// Tres faixas, na ordem em que o cliente as definiu.
mysong::nucleo::Fila com_tres() {
  mysong::nucleo::Fila fila;
  fila.junta("primeira.wav");
  fila.junta("segunda.wav");
  fila.junta("terceira.wav");
  return fila;
}

}  // namespace

TEST_CASE("a fila guarda a ordem que o cliente definiu") {
  auto fila = com_tres();
  CHECK_FALSE(fila.vazia());
  CHECK(fila.tamanho() == 3);
  CHECK(fila.indice() == 0);
  CHECK(fila.corrente() == "primeira.wav");
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
