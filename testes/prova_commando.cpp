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

// Tecla que não tem officio NÃO produz ordem. A lista encurtou com a issue #9,
// que deu sentido ás setas, ao Return e ao Escape; o que sobra são as letras sem
// officio, e ellas ficam aqui de propósito, para que uma taboada que devolvesse
// verbo para tudo não passasse calada.
TEST_CASE("tecla sem officio dá ordem nenhuma") {
  const tui::Retracto retracto = tocando();
  for (const char letra : {'a', 'z', 'N', 'P', 'Q', '1', '*'})
    CHECK(tui::ordem_da_tecla(ftxui::Event::Character(letra), retracto).verbo ==
          tui::Verbo::Nada);
  CHECK(tui::ordem_da_tecla(ftxui::Event::Tab, retracto).verbo ==
        tui::Verbo::Nada);
}

// A GUARDA DO MODO: estando-se a digitar na barra de busca, tecla alguma da
// taboada vale. É o defeito classico das TUI que esquecem o modo, e este caso
// percorre TODAS as teclas que teriam officio para provar que nenhuma escapa.
TEST_CASE("digitando, tecla alguma da taboada vale") {
  const tui::Retracto retracto = tocando();
  const ftxui::Event teclas[] = {
      ftxui::Event::Character(' '), ftxui::Event::Character('n'),
      ftxui::Event::Character('p'), ftxui::Event::Character('q'),
      ftxui::Event::Character('j'), ftxui::Event::Character('k'),
      ftxui::Event::Character('g'), ftxui::Event::Character('G'),
      ftxui::Event::Character('/'), ftxui::Event::Character('r'),
      ftxui::Event::Character('+'), ftxui::Event::Character('-'),
      ftxui::Event::Character(','), ftxui::Event::Character('.'),
      ftxui::Event::Character('b'), ftxui::Event::Character('l'),
      ftxui::Event::Character('s'), ftxui::Event::ArrowUp,
      ftxui::Event::ArrowDown,
      ftxui::Event::ArrowLeft,      ftxui::Event::ArrowRight,
      ftxui::Event::Return,         ftxui::Event::Escape,
      ftxui::Event::Home,           ftxui::Event::End,
  };
  for (const ftxui::Event& tecla : teclas) {
    // Sem o modo, cada uma d'estas tem officio; com o modo, nenhuma.
    REQUIRE(tui::ordem_da_tecla(tecla, retracto, false).verbo !=
            tui::Verbo::Nada);
    CHECK(tui::ordem_da_tecla(tecla, retracto, true).verbo == tui::Verbo::Nada);
  }
}

// As teclas da navegação, e as duas fórmas de cada uma: seta e letra do vi.
TEST_CASE("as teclas da navegação valem por seta e por letra") {
  const tui::Retracto retracto = tocando();
  const auto verbo = [&retracto](const ftxui::Event& t) {
    return tui::ordem_da_tecla(t, retracto).verbo;
  };
  CHECK(verbo(ftxui::Event::ArrowDown) == tui::Verbo::Desce);
  CHECK(verbo(ftxui::Event::Character('j')) == tui::Verbo::Desce);
  CHECK(verbo(ftxui::Event::ArrowUp) == tui::Verbo::Sobe);
  CHECK(verbo(ftxui::Event::Character('k')) == tui::Verbo::Sobe);
  CHECK(verbo(ftxui::Event::Home) == tui::Verbo::AoPrincipio);
  CHECK(verbo(ftxui::Event::Character('g')) == tui::Verbo::AoPrincipio);
  CHECK(verbo(ftxui::Event::End) == tui::Verbo::AoFim);
  CHECK(verbo(ftxui::Event::Character('G')) == tui::Verbo::AoFim);
  CHECK(verbo(ftxui::Event::Return) == tui::Verbo::Entra);
  CHECK(verbo(ftxui::Event::ArrowRight) == tui::Verbo::Entra);
  CHECK(verbo(ftxui::Event::Escape) == tui::Verbo::Volta);
  CHECK(verbo(ftxui::Event::Backspace) == tui::Verbo::Volta);
  CHECK(verbo(ftxui::Event::ArrowLeft) == tui::Verbo::Volta);
  CHECK(verbo(ftxui::Event::Character('/')) == tui::Verbo::AbreBusca);
  CHECK(verbo(ftxui::Event::Character('r')) == tui::Verbo::Varre);
  CHECK(verbo(ftxui::Event::Character('b')) == tui::Verbo::AbreBaixa);
  CHECK(verbo(ftxui::Event::Character('l')) == tui::Verbo::TrocaLetra);
  // O `s` pergunta á REDE, e não filtra o que ha: verbo proprio, e não o `/` com
  // bandeira. Os dous a darem o mesmo verbo seria o defeito que este caso guarda.
  CHECK(verbo(ftxui::Event::Character('s')) == tui::Verbo::AbreProcura);
  CHECK(verbo(ftxui::Event::Character('/')) != tui::Verbo::AbreProcura);
}

// Os alvos aqui vão escriptos á mão em SEGUNDOS, e não em passos: dizer «posicao mais
// PASSO_DA_BUSCA» seria perguntar á obra qual o passo para depois conferir que ella o
// usou, e a assertiva não poderia falhar.
//
// A busca é por `,` e `.` desde a issue #48. Era por seta, e a seta passou a NAVEGAR: o
// operador tentou voltar nos menus com a seta esquerda e o que ella fazia era buscar no
// som.
TEST_CASE("a virgula e o ponto buscam pelo passo, e aparam-se nas bordas") {
  const tui::Ordem deante =
      tui::ordem_da_tecla(ftxui::Event::Character('.'), tocando(30.0, 100.0));
  CHECK(deante.verbo == tui::Verbo::Buscar);
  CHECK(deante.alvo == doctest::Approx(35.0));

  const tui::Ordem atras =
      tui::ordem_da_tecla(ftxui::Event::Character(','), tocando(30.0, 100.0));
  CHECK(atras.verbo == tui::Verbo::Buscar);
  CHECK(atras.alvo == doctest::Approx(25.0));

  // Antes do zero pede zero, e não numero negativo.
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character(','), tocando(2.0, 100.0)).alvo ==
        doctest::Approx(0.0));
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character(','), tocando(0.0, 100.0)).alvo ==
        doctest::Approx(0.0));
  // Depois do fim pede a duração, e não numero maior que ella.
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character('.'), tocando(98.0, 100.0)).alvo ==
        doctest::Approx(100.0));
  // Duração que não presta pede o principio.
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character('.'), tocando(30.0, 0.0)).alvo ==
        doctest::Approx(0.0));

  // E as setas JÁ NÃO buscam: navegam. É a negativa que fecha a mudança, e sem ella uma
  // taboada que respondesse ás duas cousas passaria calada.
  CHECK(tui::ordem_da_tecla(ftxui::Event::ArrowRight, tocando()).verbo ==
        tui::Verbo::Entra);
  CHECK(tui::ordem_da_tecla(ftxui::Event::ArrowLeft, tocando()).verbo ==
        tui::Verbo::Volta);
}

TEST_CASE("o volume anda por degrau, e apara-se em zero e cem") {
  const tui::Ordem mais =
      tui::ordem_da_tecla(ftxui::Event::Character('+'), tocando(0.0, 100.0, 50));
  CHECK(mais.verbo == tui::Verbo::Volume);
  CHECK(mais.alvo == doctest::Approx(55.0));
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character('-'),
                            tocando(0.0, 100.0, 50)).alvo == doctest::Approx(45.0));
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character('+'),
                            tocando(0.0, 100.0, 98)).alvo == doctest::Approx(100.0));
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character('-'),
                            tocando(0.0, 100.0, 2)).alvo == doctest::Approx(0.0));
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character('+'),
                            tocando(0.0, 100.0, 100)).alvo == doctest::Approx(100.0));
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
