// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO CORREIO — testes/prova_correio.cpp
// ══════════════════════════════════════════════════════════════════════════
// Sem fios e sem tela: o que se afere é a conta das duas gerações, que é o que faz
// a colheita consumir e a substituição funccionar.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "tui/correio.hpp"

namespace tui = mysong::tui;

TEST_CASE("correio vazio não tem o que colher") {
  tui::Correio correio;
  std::vector<tui::Linha> achados;
  std::string recado;
  CHECK_FALSE(correio.colhe(&achados, &recado));
  CHECK(correio.geracao() == 0);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
