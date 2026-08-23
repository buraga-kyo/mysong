// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DE FUMO — testes/fumo_marca.cpp
// ══════════════════════════════════════════════════════════════════════════
// A primeira prova da Casa. Não se propõe a examinar o núcleo inteiro: propõe
// a demonstrar que a BATERIA RODA de verdade, o que é coisa differente de
// existir. Bateria que não sabe falhar não prova nada, e "zero provas
// falharam" com "zero provas existem" são indistinguiveis a olho nu.
//
// DOMÍNIO ......... nucleo::marca(), invocada sem argumento algum.
// CONTRA-DOMÍNIO .. veredicto do doctest, e por elle o status do ctest.
// INVARIANTE ...... a marca não é vazia, é exactamente "mysong", e todo byte
//                   seu é ASCII imprimivel (nenhum glifo de Nerd Font, nenhuma
//                   sequencia de escape que o terminal pelado não saiba pintar).
// Q.E.D. .......... invertida qualquer destas asserções, o ctest passa a
//                   accusar falha nomeando o caso; logo a bateria é viva, e o
//                   verde que ella mostra é verde ganho e não verde vazio.
// ══════════════════════════════════════════════════════════════════════════
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include "nucleo/marca.hpp"

TEST_CASE("a marca do programa se annuncia") {
  const auto assignada = mysong::nucleo::marca();
  CHECK_FALSE(assignada.empty());
  CHECK(assignada == "mysong");
}

TEST_CASE("a marca se escreve em terminal pelado") {
  for (const char letra : mysong::nucleo::marca()) {
    CHECK(letra >= 0x20);
    CHECK(letra < 0x7F);
  }
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
