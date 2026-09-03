// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA AJUDA — testes/prova_ajuda.cpp
// ══════════════════════════════════════════════════════════════════════════
// O HELP (issue #133) afere-se sem terminal: a taboada contra as taboadas das
// teclas, a medida contra a tela, a janella em écran de papel cella a cella, e
// a máquina de teclas e de rato como valores.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>

#include "tui/ajuda.hpp"
#include "tui/cabecalho.hpp"
#include "tui/commando.hpp"
#include "tui/espectro.hpp"
#include "tui/foco.hpp"
#include "tui/tokens.hpp"
#include "tui/transporte.hpp"

namespace tk = mysong::tui::tokens;
namespace tui = mysong::tui;
namespace nucleo = mysong::nucleo;

namespace {

ftxui::Screen papel(ftxui::Element quadro, int largura, int altura) {
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(largura),
                                              ftxui::Dimension::Fixed(altura));
  ftxui::Render(ecran, quadro);
  return ecran;
}

std::string pedaco(const ftxui::Screen& ecran, int x, int quantas, int y) {
  std::string texto;
  for (int c = x; c < x + quantas; ++c) texto += ecran.PixelAt(c, y).character;
  return texto;
}

ftxui::Color cor(std::string_view token) {
  const tk::Triade t = tk::rgb(token);
  return ftxui::Color::RGB(t.r, t.g, t.b);
}

ftxui::Color cor(tk::Triade t) { return ftxui::Color::RGB(t.r, t.g, t.b); }

tui::Retracto tocando() {
  tui::Retracto r;
  r.estado = nucleo::Estado::Tocando;
  r.posicao = 19.0;
  r.duracao = 189.0;
  return r;
}

// tem_officio — se alguma das tres taboadas conhece a tecla: a do commando, a
// das abas e a das setas. É o conjunto que o HELP promette nomear inteiro.
bool tem_officio(const ftxui::Event& tecla) {
  if (tui::ordem_da_tecla(tecla, tocando()).verbo != tui::Verbo::Nada)
    return true;
  if (tui::ordem_da_aba(tecla).gesto != tui::GestoDaAba::Alheio) return true;
  return tui::rumo_da_tecla(tecla) != tui::Direcao::Nenhuma;
}

// dita — se o rotulo apparece como PALAVRA inteira n'alguma linha da taboada.
bool dita(const std::string& rotulo) {
  for (const tui::GrupoDaAjuda& grupo : tui::taboada_da_ajuda())
    for (const tui::LinhaDaAjuda& linha : grupo.linhas) {
      std::istringstream palavras(linha.tecla);
      std::string palavra;
      while (palavras >> palavra)
        if (palavra == rotulo) return true;
    }
  return false;
}

// ha_cella_com — se alguma cella da tela leva a tinta pedida.
bool ha_cella_com(const ftxui::Screen& ecran, ftxui::Color tinta) {
  for (int y = 0; y < ecran.dimy(); ++y)
    for (int x = 0; x < ecran.dimx(); ++x)
      if (ecran.PixelAt(x, y).foreground_color == tinta) return true;
  return false;
}

}  // namespace

TEST_CASE("toda tecla com officio está dita na ajuda, pelo nome que o HELP usa") {
  std::vector<ftxui::Event> teclas;
  for (int c = 33; c < 127; ++c)
    teclas.push_back(ftxui::Event::Character(std::string(1, static_cast<char>(c))));
  teclas.push_back(ftxui::Event::Character(' '));
  for (const ftxui::Event& e :
       {ftxui::Event::Return, ftxui::Event::Escape, ftxui::Event::Backspace,
        ftxui::Event::Tab, ftxui::Event::TabReverse, ftxui::Event::Delete,
        ftxui::Event::ArrowUp, ftxui::Event::ArrowDown, ftxui::Event::ArrowLeft,
        ftxui::Event::ArrowRight, ftxui::Event::Home, ftxui::Event::End,
        ftxui::Event::PageUp, ftxui::Event::PageDown, ftxui::Event::F1,
        ftxui::Event::F2, ftxui::Event::F3, ftxui::Event::F4, ftxui::Event::F5,
        ftxui::Event::F6, ftxui::Event::F7, ftxui::Event::F8, ftxui::Event::F9,
        ftxui::Event::F10, ftxui::Event::F11, ftxui::Event::F12})
    teclas.push_back(e);
  std::size_t com_officio = 0;
  for (const ftxui::Event& tecla : teclas) {
    if (!tem_officio(tecla)) continue;
    ++com_officio;
    const std::string rotulo = tui::rotulo_da_tecla(tecla);
    CAPTURE(rotulo);
    CHECK_FALSE(rotulo.empty());
    CHECK(dita(rotulo));
  }
  // Ha mais de trinta teclas com officio: a varredura não ficou vazia.
  CHECK(com_officio > 30);
  // E os seis grupos, na ordem em que se lêem.
  const std::vector<tui::GrupoDaAjuda> grupos = tui::taboada_da_ajuda();
  REQUIRE(grupos.size() == 6);
  CHECK(grupos[0].nome == "TOCADOR");
  CHECK(grupos[1].nome == "NAVEGAÇÃO");
  CHECK(grupos[2].nome == "FAIXA");
  CHECK(grupos[3].nome == "PLAYLISTS");
  CHECK(grupos[4].nome == "DOWNLOAD");
  CHECK(grupos[5].nome == "RATO");
}

TEST_CASE("o rotulo da tecla diz o nome de gente de cada uma") {
  CHECK(tui::rotulo_da_tecla(ftxui::Event::Character(' ')) == "espaço");
  CHECK(tui::rotulo_da_tecla(ftxui::Event::Character('z')) == "z");
  CHECK(tui::rotulo_da_tecla(ftxui::Event::Return) == "Enter");
  CHECK(tui::rotulo_da_tecla(ftxui::Event::Escape) == "Esc");
  CHECK(tui::rotulo_da_tecla(ftxui::Event::TabReverse) == "Shift+Tab");
  CHECK(tui::rotulo_da_tecla(ftxui::Event::F11) == "F11");
  CHECK(tui::rotulo_da_tecla(ftxui::Event::ArrowLeft) == "←");
  CHECK(tui::rotulo_da_tecla(ftxui::Event::PageDown) == "PgDn");
  CHECK(tui::rotulo_da_tecla(ftxui::Event::Custom).empty());
}

TEST_CASE("a legenda do espectro tira as côres e os hertz do proprio espectro") {
  const std::vector<tui::AmostraDaAjuda> legenda = tui::legenda_do_espectro();
  REQUIRE(legenda.size() == 6);  // a barra, os quatro registros e o mudo
  CHECK(legenda[0].rotulo == "A BARRA");
  CHECK(legenda[1].rotulo == std::string(tui::nome_do_registro(tui::Registro::Graves)));
  CHECK(legenda[1].faixa == "40 Hz a 250 Hz");
  CHECK(legenda[2].faixa == "250 Hz a 1 kHz");
  CHECK(legenda[3].faixa == "1 kHz a 4 kHz");
  CHECK(legenda[4].faixa == "4 kHz a 16 kHz");
  CHECK(cor(legenda[2].tinta) ==
        cor(tui::tinta_do_registro(tui::Registro::MediosGraves)));
  CHECK(cor(legenda[4].tinta) == cor(tui::tinta_do_registro(tui::Registro::Agudos)));
  CHECK(legenda[5].rotulo == "MUDO");
  CHECK(cor(legenda[5].tinta) == cor(tk::text_faint));
}

TEST_CASE("a medida reparte as collunhas pela largura, e cede na tela baixa") {
  const tui::MedidaDaAjuda larga = tui::medida_da_ajuda(167, 67);
  CHECK(larga.collunhas == 3);
  CHECK(larga.largura == 3 * tui::LARGURA_DA_COLLUNHA + 2 * tui::VAO_ENTRE_COLLUNHAS + 4);
  CHECK(larga.altura == larga.conteudo + 3);
  CHECK(larga.uteis == larga.conteudo);
  CHECK(tui::rolagem_maxima(larga) == 0);
  CHECK(larga.x == static_cast<int>((167 - larga.largura) / 2));
  CHECK(larga.y == static_cast<int>((67 - larga.altura) / 2));
  CHECK(tui::medida_da_ajuda(120, 45).collunhas == 2);
  const tui::MedidaDaAjuda estreita = tui::medida_da_ajuda(80, 20);
  CHECK(estreita.collunhas == 1);
  CHECK(estreita.largura == tui::LARGURA_DA_COLLUNHA + 4);
  // Tela baixa: a janella toma a altura toda, e o que não cabe rola.
  CHECK(estreita.altura == 20);
  CHECK(estreita.uteis == 17);
  CHECK(tui::rolagem_maxima(estreita) == estreita.conteudo - 17);
  CHECK(tui::rolagem_maxima(estreita) > 0);
  // Tela mais estreita que uma collunha: a janella cinge-se á tela.
  CHECK(tui::medida_da_ajuda(30, 20).largura == 30);
  // Tela sem logar para a moldura: medida vazia, e janella alguma.
  CHECK(tui::medida_da_ajuda(4, 10).largura == 0);
  CHECK(tui::medida_da_ajuda(100, 4).largura == 0);
}

TEST_CASE("a máquina de teclas abre, engole, rola e fecha") {
  tui::Ajuda ajuda;
  CHECK_FALSE(tui::tecla_na_ajuda(ajuda, ftxui::Event::Escape, 10));
  tui::alterna_a_ajuda(ajuda);
  CHECK(ajuda.aberta);
  // Aberta, tecla alheia morre aqui e a ajuda fica.
  CHECK(tui::tecla_na_ajuda(ajuda, ftxui::Event::Character('z'), 10));
  CHECK(ajuda.aberta);
  // A rolagem não passa do fim nem do principio.
  CHECK(tui::tecla_na_ajuda(ajuda, ftxui::Event::ArrowDown, 10));
  CHECK(ajuda.rolagem == 1);
  tui::tecla_na_ajuda(ajuda, ftxui::Event::Character('j'), 10);
  CHECK(ajuda.rolagem == 2);
  tui::tecla_na_ajuda(ajuda, ftxui::Event::End, 10);
  CHECK(ajuda.rolagem == 10);
  tui::tecla_na_ajuda(ajuda, ftxui::Event::ArrowDown, 10);
  CHECK(ajuda.rolagem == 10);
  tui::tecla_na_ajuda(ajuda, ftxui::Event::Home, 10);
  CHECK(ajuda.rolagem == 0);
  tui::tecla_na_ajuda(ajuda, ftxui::Event::ArrowUp, 10);
  CHECK(ajuda.rolagem == 0);
  tui::tecla_na_ajuda(ajuda, ftxui::Event::PageDown, 10);
  CHECK(ajuda.rolagem == 10);  // o passo é dez, e o teto tambem
  tui::tecla_na_ajuda(ajuda, ftxui::Event::Character('k'), 10);
  CHECK(ajuda.rolagem == 9);
  tui::tecla_na_ajuda(ajuda, ftxui::Event::PageUp, 10);
  CHECK(ajuda.rolagem == 0);
  // Sem nada que rolar, a seta fica em zero.
  tui::tecla_na_ajuda(ajuda, ftxui::Event::ArrowDown, 0);
  CHECK(ajuda.rolagem == 0);
  // Cada uma das seis fecha, e fechar torna ao alto.
  for (const ftxui::Event& fecha :
       {ftxui::Event::Escape, ftxui::Event::Character('?'), ftxui::Event::F1,
        ftxui::Event::Return, ftxui::Event::Backspace,
        ftxui::Event::Character('q')}) {
    ajuda.aberta = true;
    ajuda.rolagem = 4;
    CHECK(tui::tecla_na_ajuda(ajuda, fecha, 10));
    CHECK_FALSE(ajuda.aberta);
    CHECK(ajuda.rolagem == 0);
  }
  // Alternar duas vezes torna ao que era.
  tui::alterna_a_ajuda(ajuda);
  tui::alterna_a_ajuda(ajuda);
  CHECK_FALSE(ajuda.aberta);
}

TEST_CASE("o rato rola pela roda, fecha pelo clique de fóra e fica pelo de dentro") {
  tui::Ajuda ajuda;
  const ftxui::Box caixa{10, 50, 5, 20};
  ftxui::Mouse rato;
  rato.button = ftxui::Mouse::Left;
  rato.motion = ftxui::Mouse::Pressed;
  rato.x = 20;
  rato.y = 10;
  CHECK_FALSE(tui::rato_na_ajuda(ajuda, caixa, rato, 54));
  ajuda.aberta = true;
  CHECK(tui::rato_na_ajuda(ajuda, caixa, rato, 54));
  CHECK(ajuda.aberta);  // dentro: fica
  rato.button = ftxui::Mouse::WheelDown;
  tui::rato_na_ajuda(ajuda, caixa, rato, 54);
  CHECK(ajuda.rolagem == 3);
  rato.button = ftxui::Mouse::WheelUp;
  tui::rato_na_ajuda(ajuda, caixa, rato, 54);
  CHECK(ajuda.rolagem == 0);
  // Soltar o botão não é gesto.
  rato.button = ftxui::Mouse::Left;
  rato.motion = ftxui::Mouse::Released;
  rato.x = 5;
  tui::rato_na_ajuda(ajuda, caixa, rato, 54);
  CHECK(ajuda.aberta);
  rato.motion = ftxui::Mouse::Pressed;
  tui::rato_na_ajuda(ajuda, caixa, rato, 54);
  CHECK_FALSE(ajuda.aberta);  // fóra: fecha
}

TEST_CASE("a janella pinta-se ao centro, com o titulo, a orla e a legenda") {
  tui::Ajuda ajuda;
  ajuda.aberta = true;
  ftxui::Box caixa;
  const ftxui::Screen tela =
      papel(tui::flutuante_da_ajuda(ajuda, 167, 50, &caixa), 167, 50);
  const tui::MedidaDaAjuda medida = tui::medida_da_ajuda(167, 50);
  CHECK(caixa.x_min == medida.x);
  CHECK(caixa.y_min == medida.y);
  CHECK(caixa.x_max - caixa.x_min + 1 == static_cast<int>(medida.largura));
  CHECK(caixa.y_max - caixa.y_min + 1 == static_cast<int>(medida.altura));
  // O titulo na orla de cima, e a orla em line_base sobre o fundo panel.
  CHECK(pedaco(tela, caixa.x_min + 1, 6, caixa.y_min) == " HELP ");
  CHECK(tela.PixelAt(caixa.x_min, caixa.y_min).foreground_color == cor(tk::line_base));
  CHECK(tela.PixelAt(caixa.x_min + 2, caixa.y_min + 1).background_color == cor(tk::panel));
  // O primeiro grupo no canto de cima á esquerda do conteudo, em text_heading.
  CHECK(pedaco(tela, caixa.x_min + 2, 7, caixa.y_min + 1) == "TOCADOR");
  CHECK(tela.PixelAt(caixa.x_min + 2, caixa.y_min + 1).foreground_color == cor(tk::text_heading));
  // A tecla em glow_soft, e o que faz em text_body.
  CHECK(pedaco(tela, caixa.x_min + 2, 6, caixa.y_min + 2) == "espaço");
  CHECK(tela.PixelAt(caixa.x_min + 2, caixa.y_min + 2).foreground_color == cor(tk::glow_soft));
  CHECK(tela.PixelAt(caixa.x_min + 2 + 17, caixa.y_min + 2).foreground_color == cor(tk::text_body));
  // As amostras da legenda levam as côres do espectro, e o rodapé diz como fechar.
  CHECK(ha_cella_com(tela, cor(tui::tinta_do_registro(tui::Registro::MediosGraves))));
  CHECK(ha_cella_com(tela, cor(tui::tinta_do_registro(tui::Registro::Agudos))));
  CHECK(pedaco(tela, 0, 167, caixa.y_max - 1).find("Esc ou ? fecha") != std::string::npos);
  // Fóra da janella a tela ficou por pintar: o dbox põe-na por cima do corpo.
  CHECK(tela.PixelAt(0, caixa.y_min).background_color != cor(tk::panel));
  CHECK(tela.PixelAt(0, caixa.y_min).character != "\u256d");
}

TEST_CASE("na tela baixa a janella rola, e no fim mostra a ultima linha") {
  tui::Ajuda ajuda;
  ajuda.aberta = true;
  const tui::MedidaDaAjuda medida = tui::medida_da_ajuda(100, 20);
  REQUIRE(tui::rolagem_maxima(medida) > 0);
  ajuda.rolagem = tui::rolagem_maxima(medida) + 50;  // acima do teto: cinge-se
  ftxui::Box caixa;
  const ftxui::Screen tela =
      papel(tui::flutuante_da_ajuda(ajuda, 100, 20, &caixa), 100, 20);
  CHECK(caixa.y_min == 0);
  CHECK(caixa.y_max == 19);
  const int ultima_util = caixa.y_min + 1 + static_cast<int>(medida.uteis) - 1;
  CHECK(pedaco(tela, 0, 100, ultima_util).find("apagado") != std::string::npos);
  CHECK(pedaco(tela, 0, 100, caixa.y_max - 1).find("rola") != std::string::npos);
}

TEST_CASE("fechada, a ajuda não pinta cella alguma e a caixa fica por pintar") {
  const tui::Ajuda fechada;
  ftxui::Box caixa{3, 9, 3, 9};
  const ftxui::Screen tela =
      papel(tui::flutuante_da_ajuda(fechada, 40, 6, &caixa), 40, 6);
  CHECK(caixa.IsEmpty());
  for (int y = 0; y < 6; ++y)
    for (int x = 0; x < 40; ++x) {
      CHECK(tela.PixelAt(x, y).background_color != cor(tk::panel));
      CHECK(tela.PixelAt(x, y).character.size() <= 1);  // vazio ou espaço
    }
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
