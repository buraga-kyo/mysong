// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA SALA — testes/prova_sala.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada pura da sala, e mais adiante os écrans de PAPEL d'ella, lidos
// cella a cella. Terminal algum se abre, banco algum: os retractos que a sala
// pede armam-se á mão, que é o que faz d'ella peça aferivel.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include "tui/sala.hpp"

namespace tui = mysong::tui;

TEST_CASE("a somma da colleção diz-se por extenso") {
  CHECK(tui::texto_da_duracao(0).empty());
  CHECK(tui::texto_da_duracao(-5).empty());
  CHECK(tui::texto_da_duracao(45) == "45s");
  CHECK(tui::texto_da_duracao(59) == "59s");
  CHECK(tui::texto_da_duracao(60) == "1min");
  CHECK(tui::texto_da_duracao(3599) == "59min");
  CHECK(tui::texto_da_duracao(3600) == "1h00");
  CHECK(tui::texto_da_duracao(3900) == "1h05");
  CHECK(tui::texto_da_duracao(4980) == "1h23");
}

TEST_CASE("a conta da colleção muda de substantivo com a especie") {
  CHECK(tui::texto_da_conta(4, tui::Especie::Faixas, 840) == "4 FAIXAS, 14min");
  CHECK(tui::texto_da_conta(1, tui::Especie::Faixas, 0) == "1 FAIXA");
  CHECK(tui::texto_da_conta(0, tui::Especie::Faixas, 0) == "0 FAIXAS");
  CHECK(tui::texto_da_conta(12, tui::Especie::Artistas, 0) == "12 ARTISTAS");
  CHECK(tui::texto_da_conta(1, tui::Especie::Albuns, 0) == "1 ÁLBUM");
  CHECK(tui::texto_da_conta(3, tui::Especie::Listas, 0) == "3 LISTAS");
  CHECK(tui::texto_da_conta(9, tui::Especie::Achados, 0) == "9 ACHADOS");
}
