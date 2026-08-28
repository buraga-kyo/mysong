// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DOS AJUSTES — testes/prova_ajustes.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o leitor e a precedencia SEM tocar em disco e sem tocar em ambiente: o
// leitor recebe texto, e o resolvedor recebe os degraus já colhidos. É d'isto
// que a prova vale: o resultado é o mesmo na machina do auctor e na crua.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include "nucleo/ajustes.hpp"

namespace nu = mysong::nucleo;

TEST_CASE("o leitor apara as pontas do valor, e conserva o branco do meio") {
  nu::Ajustes ajustes;
  const auto pares =
      nu::ler_pares("  acervo   =   /home/braga/Minhas Musicas  \n", &ajustes);
  REQUIRE(pares.size() == 1);
  CHECK(pares[0].chave == "acervo");
  CHECK(pares[0].valor == "/home/braga/Minhas Musicas");
  CHECK(ajustes.queixas.empty());
}

TEST_CASE("commentario e linha vazia não dão par algum nem queixa alguma") {
  nu::Ajustes ajustes;
  const auto pares = nu::ler_pares(
      "# só commentario\n\n   \nvolume = 70 # o resto sahe\n", &ajustes);
  REQUIRE(pares.size() == 1);
  CHECK(pares[0].valor == "70");
  CHECK(pares[0].linha == 4);
  CHECK(ajustes.queixas.empty());
}

TEST_CASE("a chave repetida vale a ultima, e a repetição vira queixa") {
  nu::Ajustes ajustes;
  const auto pares = nu::ler_pares("volume = 10\nvolume = 20\n", &ajustes);
  REQUIRE(pares.size() == 2);
  CHECK(pares[1].valor == "20");
  CHECK(ajustes.queixas.size() == 1);
}

TEST_CASE("linha sem egual, ou sem chave, vira queixa e não par") {
  nu::Ajustes ajustes;
  CHECK(nu::ler_pares("volume 70\n= 70\n", &ajustes).empty());
  CHECK(ajustes.queixas.size() == 2);
}
