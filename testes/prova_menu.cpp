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
using Gesto = tui::GestoDaBarra;

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

TEST_CASE("na barra aberta cada tecla dá o gesto da taboada") {
  for (const Event& fecha : {Event::Tab, Event::TabReverse, Event::Escape,
                             Event::Backspace, Event::ArrowLeft})
    CHECK(tui::gesto_da_barra(fecha) == Gesto::Fecha);
  CHECK(tui::gesto_da_barra(Event::ArrowDown) == Gesto::Desce);
  CHECK(tui::gesto_da_barra(Event::Character('j')) == Gesto::Desce);
  CHECK(tui::gesto_da_barra(Event::ArrowUp) == Gesto::Sobe);
  CHECK(tui::gesto_da_barra(Event::Character('k')) == Gesto::Sobe);
  CHECK(tui::gesto_da_barra(Event::Home) == Gesto::AoPrincipio);
  CHECK(tui::gesto_da_barra(Event::Character('g')) == Gesto::AoPrincipio);
  CHECK(tui::gesto_da_barra(Event::End) == Gesto::AoFim);
  CHECK(tui::gesto_da_barra(Event::Character('G')) == Gesto::AoFim);
  CHECK(tui::gesto_da_barra(Event::Return) == Gesto::Entra);
  CHECK(tui::gesto_da_barra(Event::ArrowRight) == Gesto::Entra);
}

// O Alheio é a regra aprovada no portão: a tecla que a barra não conhece
// fecha-a e SEGUE, e é ella que faz o atalho levar aonde a barra levaria.
TEST_CASE("a tecla alheia é alheia e é ella que faz o atalho valer") {
  for (const Event& alheia :
       {Event::Character('P'), Event::Character('s'), Event::Character('I'),
        Event::Character(' '), Event::Character('q'), Event::Character('n'),
        Event::Character('/'), Event::Character('z')})
    CHECK(tui::gesto_da_barra(alheia) == Gesto::Alheio);
}

TEST_CASE("o Tab alterna e a seta anda saturando nos extremos") {
  tui::Menu menu;
  CHECK_FALSE(menu.aberto());
  menu.abre(tui::Secao::NoRol);  // dentro de uma lista: acorda em LISTS
  CHECK(menu.aberto());
  CHECK(menu.alvo() == tui::Secao::Rois);
  menu.sobe();  // LISTS, NET, SEARCH, TRACKS, ALBUMS, ARTISTS, e satura
  for (int i = 0; i < 9; ++i) menu.sobe();
  CHECK(menu.alvo() == tui::Secao::Artistas);
  for (int i = 0; i < 9; ++i) menu.desce();  // até SPOTIFY, e satura
  CHECK(menu.alvo() == tui::Secao::Lista);
  menu.ao_principio();
  CHECK(menu.degrau() == 0);
  menu.ao_fim();
  CHECK(menu.degrau() + 1 == tui::DEGRAUS_DA_BARRA);
  menu.fecha();
  CHECK_FALSE(menu.aberto());
  menu.abre(tui::Secao::Busca);  // reabrir assenta na secção corrente
  CHECK(menu.alvo() == tui::Secao::Busca);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
