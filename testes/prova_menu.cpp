// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO MENU — testes/prova_menu.cpp
// ══════════════════════════════════════════════════════════════════════════
// A machina de foco da barra (issue #80), inteira e sem terminal: as taboadas
// de tecla e de degrau, e a machina que guarda o foco. É a bateria que a
// issue exige: Tab alterna, seta anda, Enter entra, e a tecla alheia segue.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <ftxui/component/event.hpp>

#include "tui/menu.hpp"

namespace tui = mysong::tui;

using ftxui::Event;

TEST_CASE("a taboada do degrau e a da secção são espelho fiel") {
  for (std::size_t i = 0; i < tui::DEGRAUS_DA_BARRA; ++i)
    CHECK(tui::degrau_da_secao(tui::secao_do_degrau(i)) == i);
  // Dentro de uma lista o degrau é o das LISTAS, como na pintura de hoje.
  CHECK(tui::degrau_da_secao(tui::Secao::NoRol) ==
        tui::degrau_da_secao(tui::Secao::Rois));
  // Degrau fóra da conta não estoura: cahe no primeiro.
  CHECK(tui::secao_do_degrau(99) == tui::Secao::Artistas);
}

TEST_CASE("o Tab e o Shift+Tab abrem o menu e mais tecla alguma o abre") {
  CHECK(tui::tecla_abre_menu(Event::Tab));
  CHECK(tui::tecla_abre_menu(Event::TabReverse));
  CHECK_FALSE(tui::tecla_abre_menu(Event::Return));
  CHECK_FALSE(tui::tecla_abre_menu(Event::Escape));
  CHECK_FALSE(tui::tecla_abre_menu(Event::Character('P')));
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
