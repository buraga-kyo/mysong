// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO RATO — testes/prova_rato.cpp
// ══════════════════════════════════════════════════════════════════════════
// As duas taboadas do rato (issue #95), sem terminal e sem tela: as caixas
// armam-se á mão, com as coordenadas escriptas, e o que se afere é o alvo que
// o ponto acha e o gesto que o alvo pede. É ella que apanha a collunha trocada.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <ftxui/component/mouse.hpp>
#include <ftxui/screen/box.hpp>

#include "tui/rato.hpp"

namespace tui = mysong::tui;

TEST_CASE("a caixa por pintar não casa com ponto algum") {
  const ftxui::Box vazia = tui::caixa_por_pintar();
  CHECK(vazia.IsEmpty());
  CHECK_FALSE(vazia.Contain(0, 0));
  // E a de omissão do FTXUI CASA com o canto: é d'esta medida que a regra
  // nasce, e é ella que faria o primeiro clique acertar a tela toda.
  CHECK(ftxui::Box{}.Contain(0, 0));
  const tui::CaixasDaTela nascida;
  CHECK(tui::alvo_do_ponto(nascida, 0, 0).peca == tui::Peca::Nada);
}
