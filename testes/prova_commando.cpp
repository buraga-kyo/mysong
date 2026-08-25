// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO COMMANDO — testes/prova_commando.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada de tecla para ordem, aferida sem terminal, sem motor e sem som. O
// alvo de cada linha está escripto á mão: nenhum caso pergunta á obra qual o
// passo da busca para depois conferir que ella o usou, que isso seria consultar
// o oraculo sob prova, defeito que esta Casa já apanhou nove vezes de uma vez.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include "tui/commando.hpp"

namespace tui = mysong::tui;
namespace nu = mysong::nucleo;

namespace {

tui::Retracto tocando(double posicao = 30.0, double duracao = 100.0,
                      int volume = 50) {
  return {nu::Estado::Tocando, posicao, duracao, volume, "faixa", 0, 3};
}

}  // namespace

TEST_CASE("o espaço alterna segundo o estado, e nada faz estando parado") {
  tui::Retracto retracto = tocando();
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character(' '), retracto).verbo ==
        tui::Verbo::Pausar);
  retracto.estado = nu::Estado::Pausado;
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character(' '), retracto).verbo ==
        tui::Verbo::Retomar);
  retracto.estado = nu::Estado::Parado;
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character(' '), retracto).verbo ==
        tui::Verbo::Nada);
}

TEST_CASE("as tres teclas de verbo simples chegam por si") {
  const tui::Retracto retracto = tocando();
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character('n'), retracto).verbo ==
        tui::Verbo::Proxima);
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character('p'), retracto).verbo ==
        tui::Verbo::Anterior);
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character('q'), retracto).verbo ==
        tui::Verbo::Sahir);
}

// Tecla que não é do mockup NÃO chega ao tocador. É o caso que prova a
// negativa, e ella importa tanto como a positiva: sem elle, uma taboada que
// devolvesse Pausar para tudo passaria os casos de cima.
TEST_CASE("tecla que não é do mockup dá ordem nenhuma") {
  const tui::Retracto retracto = tocando();
  for (const char letra : {'a', 'z', 'N', 'P', 'Q', '1', '/', '*'})
    CHECK(tui::ordem_da_tecla(ftxui::Event::Character(letra), retracto).verbo ==
          tui::Verbo::Nada);
  CHECK(tui::ordem_da_tecla(ftxui::Event::ArrowUp, retracto).verbo ==
        tui::Verbo::Nada);
  CHECK(tui::ordem_da_tecla(ftxui::Event::ArrowDown, retracto).verbo ==
        tui::Verbo::Nada);
  CHECK(tui::ordem_da_tecla(ftxui::Event::Return, retracto).verbo ==
        tui::Verbo::Nada);
  CHECK(tui::ordem_da_tecla(ftxui::Event::Escape, retracto).verbo ==
        tui::Verbo::Nada);
  CHECK(tui::ordem_da_tecla(ftxui::Event::Tab, retracto).verbo ==
        tui::Verbo::Nada);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
