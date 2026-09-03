// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO MENU DE CONTEXTO — testes/prova_menu_contexto.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada de dentro do menu (issue #96), sem terminal e sem tela: o estado
// arma-se á mão e afere-se o pedido que cada tecla devolve. Ella apanha o item
// trocado de logar, defeito que apagaria a faixa errada.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include "tui/menu_contexto.hpp"
#include "tui/tokens.hpp"

namespace tui = mysong::tui;
namespace nucleo = mysong::nucleo;

namespace {

// As tres listas de mentira. Os ids vão SALTEADOS de proposito: menu que
// devolvesse o indice passaria por ids seguidos e juntaria á lista errada.
std::vector<nucleo::Rol> tres_listas() {
  return {{7, "funk", 3}, {12, "estudo", 0}, {40, "domingo", 9}};
}

tui::MenuDeContexto menu_de_pe(std::vector<nucleo::Rol> listas) {
  tui::MenuDeContexto menu;
  tui::abre_o_menu(menu, 4, "Montagem Lunar Celestia", std::move(listas));
  return menu;
}

ftxui::Event clique(ftxui::Mouse::Button botao, ftxui::Mouse::Motion gesto) {
  ftxui::Mouse rato;
  rato.button = botao;
  rato.motion = gesto;
  return ftxui::Event::Mouse("", rato);
}

}  // namespace

TEST_CASE("o menu abre na faixa alvo, no primeiro item e sem submenu") {
  const tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  CHECK(menu.aberto);
  CHECK(menu.faixa == 4);
  CHECK(menu.titulo == "Montagem Lunar Celestia");
  CHECK(menu.item == 0);
  CHECK_FALSE(menu.submenu);
}

TEST_CASE("as setas andam pelos cinco itens, e dão a volta") {
  tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  for (std::size_t passo = 1; passo < tui::QUANTOS_ITENS; ++passo) {
    CHECK(tui::tecla_no_menu(menu, ftxui::Event::ArrowDown).pedido ==
          tui::PedidoDoMenu::Nada);
    CHECK(menu.item == passo);
  }
  tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);  // do ultimo ao primeiro
  CHECK(menu.item == 0);
  tui::tecla_no_menu(menu, ftxui::Event::ArrowUp);  // e do primeiro ao ultimo
  CHECK(menu.item == tui::QUANTOS_ITENS - 1);
  CHECK(menu.aberto);  // andar não fecha
}

TEST_CASE("a seta direita abre o submenu das listas e a esquerda fecha-o") {
  tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  tui::tecla_no_menu(menu, ftxui::Event::ArrowRight);
  CHECK_FALSE(menu.submenu);  // no TOCAR a direita não abre cousa alguma
  tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);
  tui::tecla_no_menu(menu, ftxui::Event::ArrowRight);
  CHECK(menu.submenu);
  CHECK(menu.lista == 0);
  tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);
  CHECK(menu.lista == 1);
  CHECK(menu.item == 1);  // dentro do submenu a seta anda nas LISTAS
  tui::tecla_no_menu(menu, ftxui::Event::ArrowLeft);
  CHECK_FALSE(menu.submenu);
  CHECK(menu.aberto);  // fechar o submenu não fecha o menu
}

TEST_CASE("o Escape fecha o menu, e o botão a descer tambem") {
  tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  tui::tecla_no_menu(menu, clique(ftxui::Mouse::Left, ftxui::Mouse::Released));
  CHECK(menu.aberto);  // o soltar do proprio clique que o abriu não o fecha
  tui::tecla_no_menu(menu, clique(ftxui::Mouse::Left, ftxui::Mouse::Pressed));
  CHECK_FALSE(menu.aberto);
  menu = menu_de_pe(tres_listas());
  tui::tecla_no_menu(menu, ftxui::Event::Escape);
  CHECK_FALSE(menu.aberto);
  // E menu fechado consome tecla nenhuma: o pedido sahe Nada, e o estado fica.
  CHECK(tui::tecla_no_menu(menu, ftxui::Event::Return).pedido ==
        tui::PedidoDoMenu::Nada);
}

TEST_CASE("sem lista alguma o submenu não abre") {
  tui::MenuDeContexto menu = menu_de_pe({});
  tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);
  tui::tecla_no_menu(menu, ftxui::Event::ArrowRight);
  CHECK_FALSE(menu.submenu);
  CHECK(tui::tecla_no_menu(menu, ftxui::Event::Return).pedido ==
        tui::PedidoDoMenu::Nada);
  CHECK(menu.aberto);  // e o Enter tambem não fecha nem promette cousa alguma
}

TEST_CASE("o Enter escolhe cada item, na ordem em que se lêem") {
  const tui::PedidoDoMenu esperados[] = {
      tui::PedidoDoMenu::Toca, tui::PedidoDoMenu::Nada,
      tui::PedidoDoMenu::NovaLista, tui::PedidoDoMenu::Renomeia,
      tui::PedidoDoMenu::Apaga};
  for (std::size_t qual = 0; qual < tui::QUANTOS_ITENS; ++qual) {
    tui::MenuDeContexto menu = menu_de_pe(tres_listas());
    for (std::size_t passo = 0; passo < qual; ++passo)
      tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);
    const tui::RespostaDoMenu escolha =
        tui::tecla_no_menu(menu, ftxui::Event::Return);
    CHECK(escolha.pedido == esperados[qual]);
    CHECK(escolha.lista == 0);
    // O JUNTAR abre o submenu em vez de escolher, e por isso fica de pé.
    CHECK(menu.aberto == (qual == 1));
  }
}

TEST_CASE("o Enter dentro do submenu junta á lista eleita, pelo id d'ella") {
  tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);
  tui::tecla_no_menu(menu, ftxui::Event::ArrowRight);
  tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);
  tui::tecla_no_menu(menu, ftxui::Event::ArrowDown);
  const tui::RespostaDoMenu escolha =
      tui::tecla_no_menu(menu, ftxui::Event::Return);
  CHECK(escolha.pedido == tui::PedidoDoMenu::Junta);
  CHECK(escolha.lista == 40);  // a terceira, e o id d'ella, e não o indice
  CHECK_FALSE(menu.aberto);
}

namespace {

// O ÉCRAN DE PAPEL, lido cella a cella. Não se lê o `ToString`, pela razão que
// a prova da tabella deu: elle mette escapes no meio dos bytes.
ftxui::Screen pintado(const tui::MenuDeContexto& menu, const ftxui::Box& linha,
                      int largura, int altura) {
  ftxui::Element quadro = tui::flutuante_do_menu(
      menu, linha, static_cast<std::size_t>(largura),
      static_cast<std::size_t>(altura));
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(largura),
                                              ftxui::Dimension::Fixed(altura));
  ftxui::Render(ecran, quadro);
  return ecran;
}

std::string linha_de(ftxui::Screen& ecran, int y) {
  std::string feita;
  for (int x = 0; x < ecran.dimx(); ++x) feita += ecran.PixelAt(x, y).character;
  return feita;
}

}  // namespace

TEST_CASE("a caixa do menu veste o chrome do RADICAL, linha a linha") {
  tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  CHECK(tui::medida_do_menu(menu).largura == 25);
  CHECK(tui::medida_do_menu(menu).altura == 8);
  // A linha da faixa é a SEIS: o menu abre na sete, logo abaixo d'ella.
  ftxui::Screen ecran = pintado(menu, {0, 59, 6, 6}, 120, 30);
  CHECK(linha_de(ecran, 7).rfind("┌─ Montagem Lunar Celes ┐", 0) == 0);
  CHECK(linha_de(ecran, 8).rfind("│ TOCAR", 0) == 0);
  CHECK(linha_de(ecran, 9).find("JUNTAR À LISTA") != std::string::npos);
  CHECK(linha_de(ecran, 9).find("▸") != std::string::npos);
  CHECK(linha_de(ecran, 10).find("NOVA LISTA COM ESTA") != std::string::npos);
  CHECK(linha_de(ecran, 11).rfind("├", 0) == 0);  // o filete, antes das duas
  CHECK(linha_de(ecran, 12).find("RENOMEAR") != std::string::npos);
  CHECK(linha_de(ecran, 13).find("APAGAR") != std::string::npos);
  CHECK(linha_de(ecran, 14).rfind("└", 0) == 0);
  // A linha ACIMA fica intacta: a camada não pinta o que não é d'ella.
  CHECK(linha_de(ecran, 6).find_first_not_of(' ') == std::string::npos);
}

namespace {
namespace tokens = mysong::tui::tokens;

ftxui::Color cor_de(std::string_view token) {
  const tokens::Triade c = tokens::rgb(token);
  return ftxui::Color::RGB(c.r, c.g, c.b);
}
}  // namespace

TEST_CASE("o item eleito sahe em bloco v600 de tinta v50") {
  tui::MenuDeContexto menu = menu_de_pe(tres_listas());
  ftxui::Screen ecran = pintado(menu, {0, 59, 6, 6}, 120, 30);
  // A linha oito é a do TOCAR, que é o item em que o menu abre.
  CHECK(ecran.PixelAt(2, 8).background_color == cor_de(tokens::v600));
  CHECK(ecran.PixelAt(2, 8).foreground_color == cor_de(tokens::v50));
  // A de baixo não: fundo panel, tinta text_primary, que é o chrome pedido.
  CHECK(ecran.PixelAt(2, 9).background_color == cor_de(tokens::panel));
  CHECK(ecran.PixelAt(2, 9).foreground_color == cor_de(tokens::text_primary));
  // A ORLA fica de FÓRA do bloco, ainda na linha eleita: ella é do chrome, e
  // bloco que a comesse faria a caixa parecer partida na linha do dedo.
  CHECK(ecran.PixelAt(0, 8).background_color == cor_de(tokens::panel));
  CHECK(ecran.PixelAt(0, 8).foreground_color == cor_de(tokens::line_base));
  // E o filete sahe mais apagado que a orla, que elle divide e não fecha.
  CHECK(ecran.PixelAt(4, 11).foreground_color == cor_de(tokens::line_dim));
}

TEST_CASE("o menu pousa abaixo da linha, e acima quando não cabe") {
  tui::MenuDeContexto menu = menu_de_pe({});
  const tui::MedidaDoMenu medida = tui::medida_do_menu(menu);
  const int alta = static_cast<int>(medida.altura);
  const tui::CantoDoMenu abaixo = tui::ancora_do_menu({4, 60, 6, 6}, medida, 120, 30);
  CHECK(abaixo.y == 7);  // logo abaixo da linha, sem tapar a faixa
  CHECK(abaixo.x == 4);  // e alinhado com o principio d'ella
  // Na ultima linha da pauta não cabe abaixo: abre ACIMA, e o pé encosta-se-lhe.
  const tui::CantoDoMenu acima =
      tui::ancora_do_menu({4, 60, 28, 28}, medida, 120, 30);
  CHECK(acima.y == 28 - alta);
  CHECK(acima.y + alta <= 30);
}

TEST_CASE("o menu nunca sahe da tela, nem ao alto nem á direita") {
  tui::MenuDeContexto menu = menu_de_pe({});
  const tui::MedidaDoMenu medida = tui::medida_do_menu(menu);
  const int alta = static_cast<int>(medida.altura);
  const int larga = static_cast<int>(medida.largura);
  // Tela baixa: não cabe abaixo da linha nem acima d'ella. Cinge-se á borda.
  const tui::CantoDoMenu preso = tui::ancora_do_menu({0, 20, 1, 1}, medida, 30, 9);
  CHECK(preso.y == 9 - alta);
  CHECK(preso.y >= 0);
  // Linha encostada á direita: o menu recua o bastante para caber, e nem uma
  // collunha a mais. Sem este recuo elle sahia pela borda, aparado em silencio.
  const tui::CantoDoMenu recuado =
      tui::ancora_do_menu({110, 119, 2, 2}, medida, 120, 30);
  CHECK(recuado.x == 120 - larga);
  CHECK(recuado.x + larga == 120);
}
