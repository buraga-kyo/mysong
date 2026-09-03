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

std::string lida(const ftxui::Screen& ecran) {
  std::string dita;
  for (int x = 0; x < ecran.dimx(); ++x) {
    const std::string& glifo = ecran.PixelAt(x, 0).character;
    dita += glifo.empty() ? " " : glifo;
  }
  return dita;
}

ftxui::Color cor(std::string_view token) {
  const tk::Triade c = tk::rgb(token);
  return ftxui::Color::RGB(c.r, c.g, c.b);
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

}  // namespace

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
