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
  // O `a` e o `P` sahiram d'esta lista na issue #10, que lhes deu officio: o `a`
  // junta á lista, e o `P` abre as listas. O `z` sahiu na issue #62, que lhe deu
  // o embaralhar. Ficam as que ainda não têm officio nenhum.
  for (const char letra : {'w', 'y', 'N', 'Q', '1', '*'})
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
      ftxui::Event::Character('s'), ftxui::Event::Character('P'),
      ftxui::Event::Character('c'), ftxui::Event::Character('R'),
      ftxui::Event::Character('D'), ftxui::Event::Character('a'),
      ftxui::Event::Character('t'), ftxui::Event::Character('K'),
      ftxui::Event::Character('J'), ftxui::Event::Character('v'),
      ftxui::Event::Character('I'), ftxui::Event::Character('T'),
      ftxui::Event::Character('f'),
      ftxui::Event::ArrowUp,
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
  // As oito das listas. As tres que estragam cousa gravada são MAIUSCULAS: tecla
  // que muda o que está no disco não ha de ficar debaixo do dedo de quem anda na
  // lista com as minusculas do vi.
  CHECK(verbo(ftxui::Event::Character('P')) == tui::Verbo::AbreRois);
  CHECK(verbo(ftxui::Event::Character('c')) == tui::Verbo::CriaRol);
  CHECK(verbo(ftxui::Event::Character('R')) == tui::Verbo::RenomeiaRol);
  CHECK(verbo(ftxui::Event::Character('D')) == tui::Verbo::ApagaRol);
  CHECK(verbo(ftxui::Event::Character('a')) == tui::Verbo::JuntaAoRol);
  CHECK(verbo(ftxui::Event::Character('t')) == tui::Verbo::RetiraDoRol);
  CHECK(verbo(ftxui::Event::Character('K')) == tui::Verbo::SobeNoRol);
  CHECK(verbo(ftxui::Event::Character('J')) == tui::Verbo::DesceNoRol);
  // O `v` do video é MINUSCULA porque não estraga cousa gravada: abre janella, e
  // fechá-la não perde nada. As maiusculas ficam para o que muta o disco.
  CHECK(verbo(ftxui::Event::Character('v')) == tui::Verbo::AbreVideo);
  // As duas do catalogo. MAIUSCULAS: a primeira abre porta de rede, e a segunda
  // encommenda cincoenta baixas de uma vez.
  CHECK(verbo(ftxui::Event::Character('I')) == tui::Verbo::AbreCatalogo);
  CHECK(verbo(ftxui::Event::Character('T')) == tui::Verbo::BaixaTudo);
  // E a minuscula d'ellas continua a ser a do vi: `k` e `j` andam, e não movem.
  CHECK(verbo(ftxui::Event::Character('k')) == tui::Verbo::Sobe);
  CHECK(verbo(ftxui::Event::Character('j')) == tui::Verbo::Desce);
  // O `f` da fonte (issue #56). MINUSCULA: ciclar a fonte não estraga cousa
  // gravada, e a guarda da secção vive na janella, como a do BaixaTudo.
  CHECK(verbo(ftxui::Event::Character('f')) == tui::Verbo::TrocaFonte);
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

// ── A JANELLA DO VÍDEO (issue #17) ─────────────────────────────────────────
// Estando a janella de pé, o motor de audio está CALADO, e as teclas de transporte
// governam a janella. Sem isto, o espaço lia o estado do motor: motor parado dava
// Ordem::Nada, e a tecla não pausava nada. Foi defeito medido n'um pty, e não
// suposto, e estes casos são o que o guarda.

TEST_CASE("com janella de pé, o espaço governa a janella e não o motor") {
  tui::Retracto retracto;              // motor PARADO, que é o que succede
  retracto.estado = mysong::nucleo::Estado::Parado;
  retracto.video = true;
  retracto.video_pausada = false;
  // Sem o campo do video, isto dava Ordem::Nada: parado não tem o que pausar.
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character(' '), retracto).verbo ==
        tui::Verbo::Pausar);
  retracto.video_pausada = true;
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character(' '), retracto).verbo ==
        tui::Verbo::Retomar);
  // E a janella GANHA do motor: com os dous a dizer cousas differentes, vale ella,
  // que é a que está a tocar.
  retracto.estado = mysong::nucleo::Estado::Tocando;
  retracto.video_pausada = true;
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character(' '), retracto).verbo ==
        tui::Verbo::Retomar);
  // Sem janella, volta a valer o motor.
  retracto.video = false;
  CHECK(tui::ordem_da_tecla(ftxui::Event::Character(' '), retracto).verbo ==
        tui::Verbo::Pausar);
}

TEST_CASE("com janella de pé, a busca sahe RELATIVA e não absoluta") {
  tui::Retracto retracto;
  retracto.video = true;
  retracto.posicao = 30.0;  // do MOTOR, e o motor não é quem toca
  retracto.duracao = 100.0;
  const tui::Ordem deante =
      tui::ordem_da_tecla(ftxui::Event::Character('.'), retracto);
  CHECK(deante.verbo == tui::Verbo::Buscar);
  CHECK(deante.relativo);
  // CINCO, e não trinta e cinco: o alvo é deslocamento, e a posição do motor não
  // diz nada da janella. Se sahisse absoluto, teclar `.` saltava a janella para o
  // segundo trinta e cinco, que não é onde ella estava.
  CHECK(deante.alvo == doctest::Approx(5.0));

  const tui::Ordem atras =
      tui::ordem_da_tecla(ftxui::Event::Character(','), retracto);
  CHECK(atras.relativo);
  CHECK(atras.alvo == doctest::Approx(-5.0));

  // Sem janella, ABSOLUTA como sempre, e o relativo fica falso.
  retracto.video = false;
  const tui::Ordem no_motor =
      tui::ordem_da_tecla(ftxui::Event::Character('.'), retracto);
  CHECK_FALSE(no_motor.relativo);
  CHECK(no_motor.alvo == doctest::Approx(35.0));
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
