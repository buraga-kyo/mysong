// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO CABEÇALHO — testes/prova_cabecalho.cpp
// ══════════════════════════════════════════════════════════════════════════
// A linha do alto (issue #102) em écran de PAPEL, lida cella a cella, e as
// taboadas puras das teclas. Terminal algum se abre: o Retracto arma-se á mão.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <string>

#include "tui/cabecalho.hpp"
#include "tui/tokens.hpp"

namespace nu = mysong::nucleo;
namespace tk = mysong::tui::tokens;
namespace tui = mysong::tui;

namespace {

// linha_do — o cabeçalho pintado, lido cella a cella. O `ToString` metteria
// escape no meio dos bytes, e contar bytes seria contar a tinta.
ftxui::Screen papel(ftxui::Element quadro, int largura) {
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(largura),
                                              ftxui::Dimension::Fixed(1));
  ftxui::Render(ecran, quadro);
  return ecran;
}

// pedaco — as `quantas` cellas a partir da collunha `x`, na fileira zero. Por
// CELLA, e não por byte: `substr` n'uma cadeia UTF-8 contaria octetos, e o
// glifo de tres bytes desalinharia todo indice depois do primeiro.
std::string pedaco(const ftxui::Screen& ecran, int x, int quantas) {
  std::string dita;
  for (int i = x; i < x + quantas && i < ecran.dimx(); ++i) {
    const std::string& glifo = ecran.PixelAt(i, 0).character;
    dita += glifo.empty() ? " " : glifo;
  }
  return dita;
}

// O que sôa: uma faixa de verdade do acervo d'elle, a tocar aos dezanove
// segundos de tres minutos e nove, com o volume cheio e os dous modos parados.
tui::Retracto tocando() {
  tui::Retracto d_ella;
  d_ella.estado = nu::Estado::Tocando;
  d_ella.posicao = 19.0;
  d_ella.duracao = 189.0;
  d_ella.volume = 100;
  return d_ella;
}

ftxui::Color cor(std::string_view token) {
  const tk::Triade c = tk::rgb(token);
  return ftxui::Color::RGB(c.r, c.g, c.b);
}

}  // namespace

// A LINHA INTEIRA contra alvo escripto Á MÃO. Contar collunhas do écran de
// papel não provaria cousa alguma: elle enche sempre a largura que se lhe
// pediu. O que prova é a cadeia, que diz ordem, guarnição e conta de uma vez.
TEST_CASE("a linha do alto sahe egual á cadeia escripta á mão") {
  const ftxui::Screen tela = papel(
      tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong,
                                 "Montagem Lunar Celestia 1.0 (SLOWED)", 167),
      167);

  // A conta, feita á mão: a fita da esquerda pede 51 collunhas (11 da primeira
  // aba, 13 da segunda, 12 da terceira, 3 por botão, e as 6 setas), a da
  // direita pede 52 (a seta de entrada, 15 do tempo, 8 do volume, 14 do
  // embaralhar, 11 do repetir, e as 3 setas do meio), e o nome toma as 64 que
  // sobram: 51 mais 64 mais 52 dão 167 em ponto.
  CHECK(pedaco(tela, 0, 11) == " \U000f075a MY SONG ");
  CHECK(pedaco(tela, 11, 1) == "\ue0b0");
  CHECK(pedaco(tela, 12, 13) == " \U000f0cb8 PLAYLISTS ");
  CHECK(pedaco(tela, 26, 12) == " \U000f01da DOWNLOAD ");
  CHECK(pedaco(tela, 39, 3) == " \U000f03e4 ");   // toca: o botão diz PAUSAR
  CHECK(pedaco(tela, 43, 3) == " \U000f04ae ");   // anterior
  CHECK(pedaco(tela, 47, 3) == " \U000f04ad ");   // seguinte
  CHECK(pedaco(tela, 51, 36) == "Montagem Lunar Celestia 1.0 (SLOWED)");
  CHECK(pedaco(tela, 115, 1) == "\ue0b2");        // a seta de entrada da direita
  CHECK(pedaco(tela, 116, 15) == " 00:19 / 03:09 ");
  CHECK(pedaco(tela, 132, 8) == " \U000f057e 100% ");
  CHECK(pedaco(tela, 141, 14) == " \U000f049d EMBARALHAR ");
  CHECK(pedaco(tela, 156, 11) == " \U000f0456 REPETIR ");
  // E a linha FECHA a largura: nada sobra, e nada transborda.
  CHECK(pedaco(tela, 0, 167) == pedaco(tela, 0, 200));
}

TEST_CASE("a linha fecha a largura exacta, e o nome toma o que sobra") {
  // Em 120 sobram 17 collunhas ao nome: 120 menos as 51 da esquerda e as 52 da
  // direita. O nome que não cabe corta-se com «…»; o que cabe enche-se.
  const ftxui::Screen larga = papel(
      tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong,
                                 "Montagem Lunar Celestia 1.0 (SLOWED)", 120),
      120);
  CHECK(pedaco(larga, 51, 17) == "Montagem Lunar C…");
  CHECK(pedaco(larga, 68, 1) == "\ue0b2");
  // Nome curto: o fundo do segmento veste a collunha inteira, e o que sobra
  // enche-se de espaço. Buraco escuro no meio da fita lê-se como emenda.
  const ftxui::Screen curto =
      papel(tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong, "NO FEAR!",
                                       120),
            120);
  CHECK(pedaco(curto, 51, 17) == "NO FEAR!         ");
  // E nada tocando, o meio DIZ que nada toca, em vez de ficar em branco.
  tui::Retracto parado;
  CHECK(pedaco(papel(tui::elemento_do_cabecalho(parado, tui::Aba::MySong, "",
                                                120),
                     120),
               51, 11) == "(nada toca)");
}


// AS TINTAS. A aba corrente é BLOCO SOLIDO, v600 com texto v50, que é o gesto do
// site d'elle onde o que está sob a mão vira bloco cheio; as outras ficam no
// raised do chrome. O modo aceso é glow_core, e o apagado é text_muted.
TEST_CASE("a aba corrente sahe em bloco solido, e as outras no repouso") {
  const ftxui::Screen tela = papel(
      tui::elemento_do_cabecalho(tocando(), tui::Aba::Playlists, "x", 167),
      167);
  CHECK(tela.PixelAt(14, 0).background_color == cor(tk::v600));
  CHECK(tela.PixelAt(14, 0).foreground_color == cor(tk::v50));
  CHECK(tela.PixelAt(4, 0).background_color == cor(tk::raised));
  CHECK(tela.PixelAt(4, 0).foreground_color == cor(tk::text_primary));
  CHECK(tela.PixelAt(30, 0).background_color == cor(tk::raised));
  // Os botões vestem panel_hi com o glifo em glow_core: é o glow CONTIDO da
  // regra da Casa, que accende no que TOCA e nunca no fundo todo.
  CHECK(tela.PixelAt(40, 0).background_color == cor(tk::panel_hi));
  CHECK(tela.PixelAt(40, 0).foreground_color == cor(tk::glow_core));
  // O nome veste `panel`, que é o degrau de fundo, e não o da fita.
  CHECK(tela.PixelAt(60, 0).background_color == cor(tk::panel));
}


//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
