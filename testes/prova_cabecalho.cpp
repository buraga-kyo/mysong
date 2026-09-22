// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO CABEÇALHO, testes/prova_cabecalho.cpp
// ══════════════════════════════════════════════════════════════════════════
// A linha do alto (issue #102) em écran de PAPEL, lida cella a cella, e as
// taboadas puras das teclas. Terminal algum se abre: o Retracto arma-se á mão.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <string>
#include <vector>

#include "tui/cabecalho.hpp"
#include "tui/tokens.hpp"

namespace nu = mysong::nucleo;
namespace tk = mysong::tui::tokens;
namespace tui = mysong::tui;

namespace {

// linha_do, o cabeçalho pintado, lido cella a cella. O `ToString` metteria
// escape no meio dos bytes, e contar bytes seria contar a tinta.
ftxui::Screen papel(ftxui::Element quadro, int largura, int altura = 1) {
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(largura),
                                              ftxui::Dimension::Fixed(altura));
  ftxui::Render(ecran, quadro);
  return ecran;
}

// pedaco, as `quantas` cellas a partir da collunha `x`, na fileira zero. Por
// CELLA, e não por byte: `substr` n'uma cadeia UTF-8 contaria octetos, e o
// glifo de tres bytes desalinharia todo indice depois do primeiro.
std::string pedaco(const ftxui::Screen& ecran, int x, int quantas,
                   int linha = 0) {
  std::string dita;
  for (int i = x; i < x + quantas && i < ecran.dimx(); ++i) {
    const std::string& glifo = ecran.PixelAt(i, linha).character;
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
TEST_CASE("a fita sahe na ordem d'elle, egual á cadeia escripta á mão") {
  const ftxui::Screen tela = papel(
      tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong, {}, 167), 167);
  // A conta, feita á mão (issue #134): as abas pedem 39 collunhas (11, 13 e 12
  // das palavras e as 3 setas), os botões 12 (3 cada e as 3 setas), e a ponta
  // direita 61 com o HELP. Ao MEIO ficam as 55 que sobram, do 51 ao 105.
  CHECK(pedaco(tela, 0, 15) == " \U000f075a MY 0 SONG's ");
  CHECK(pedaco(tela, 15, 1) == "\ue0b0");
  CHECK(pedaco(tela, 16, 13) == " \U000f0cb8 PLAYLISTS ");
  CHECK(pedaco(tela, 30, 12) == " \U000f01da DOWNLOAD ");
  CHECK(pedaco(tela, 42, 1) == "\ue0b0");
  CHECK(pedaco(tela, 43, 3) == " \U000f03e4 ");
  CHECK(pedaco(tela, 47, 3) == " \U000f04ae ");
  CHECK(pedaco(tela, 51, 3) == " \U000f04ad ");
  CHECK(pedaco(tela, 54, 1) == "\ue0b0");
  std::string tracos;
  for (int i = 0; i < 51; ++i) tracos += "\u2501";
  CHECK(pedaco(tela, 55, 51) == tracos);
  CHECK(pedaco(tela, 106, 1) == "\ue0b2");        // a seta de entrada da direita
  CHECK(pedaco(tela, 107, 15) == " 00:19 / 03:09 ");
  CHECK(pedaco(tela, 123, 8) == " \U000f057e 100% ");
  // O volume a TRES algarismos, enchido á esquerda: sem elle, cada `-` movia
  // as peças da direita uma collunha, e a onda com ellas.
  tui::Retracto baixo = tocando();
  baixo.volume = 95;
  CHECK(pedaco(papel(tui::elemento_do_cabecalho(baixo, tui::Aba::MySong, {},
                                                167),
                     167),
               123, 8) == " \U000f057e  95% ");
  CHECK(pedaco(tela, 132, 14) == " \U000f049d EMBARALHAR ");
  CHECK(pedaco(tela, 147, 11) == " \U000f0456 REPETIR ");
  CHECK(pedaco(tela, 158, 1) == "\ue0b2");
  CHECK(pedaco(tela, 159, 8) == " \U000f02d7 HELP ");
  // E a linha FECHA a largura: nada sobra, e nada transborda.
  CHECK(pedaco(tela, 0, 167) == pedaco(tela, 0, 200));
}

TEST_CASE("a aba MY SONG mostra a quantidade real do retracto") {
  tui::Retracto retracto = tocando();
  retracto.acervo = 42;
  const ftxui::Screen tela = papel(
      tui::elemento_do_cabecalho(retracto, tui::Aba::MySong, {}, 167), 167);
  CHECK(pedaco(tela, 0, 16) == " \U000f075a MY 42 SONG's ");
}

TEST_CASE("a linha fecha a largura exacta, e o meio toma o que sobra") {
  // Em 120 as fixas pedem 51 e o HELP já não cabe (51 + 61 + 12 passa de 120):
  // ficam quatro peças á direita, 52 collunhas, e ao meio as 17 que sobram.
  const ftxui::Screen larga = papel(
      tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong, {}, 120), 120);
  std::string tracos;
  for (int i = 0; i < 13; ++i) tracos += "\u2501";
  CHECK(pedaco(larga, 55, 13) == tracos);
  CHECK(pedaco(larga, 68, 1) == "\ue0b2");
  CHECK(pedaco(larga, 69, 15) == " 00:19 / 03:09 ");
  CHECK(pedaco(larga, 0, 120).find("REPETIR") != std::string::npos);
  CHECK(pedaco(larga, 0, 120).find("HELP") == std::string::npos);
  // Nada tocando, o botão diz TOCAR e a barra fica toda por andar: o nome do
  // que sôa já não mora aqui (issue #134), mora no painel.
  tui::Retracto parado;
  const ftxui::Screen quieta = papel(
      tui::elemento_do_cabecalho(parado, tui::Aba::MySong, {}, 120), 120);
  CHECK(pedaco(quieta, 43, 3) == " \U000f040a ");
  CHECK(quieta.PixelAt(55, 0).foreground_color == cor(tk::line_dim));
  CHECK(quieta.PixelAt(67, 0).foreground_color == cor(tk::line_dim));
}


// A CONTA da fita, interrogada sem se pintar cousa alguma: as 51 collunhas das
// fixas (abas e botões) e as seis larguras da ponta direita.
TEST_CASE("a conta da fita dá o que sobra ao meio, e a direita cede do fim") {
  const std::vector<std::size_t> direita = {0, 16, 25, 40, 52, 61};
  const auto conta = [&](std::size_t larga) {
    return tui::conta_da_fita(larga, 51, direita);
  };
  CHECK(conta(167).quantas == 5);
  CHECK(conta(167).meio == 55);
  // A somma FECHA a largura: sem isto, a fita deixaria vão ou transbordaria.
  for (const std::size_t larga : {79, 88, 100, 103, 115, 124, 166, 167, 200}) {
    const tui::ContaDaFita c = conta(larga);
    CHECK(51 + c.meio + direita[c.quantas] == larga);
    CHECK(c.meio >= tui::MEIO_MINIMO);
  }
  // A ESCADA: cada peça cabe emquanto ao meio sobram as doze do minimo.
  CHECK(conta(124).quantas == 5);
  CHECK(conta(123).quantas == 4);
  CHECK(conta(115).quantas == 4);
  CHECK(conta(114).quantas == 3);
  CHECK(conta(103).quantas == 3);
  CHECK(conta(102).quantas == 2);
  CHECK(conta(88).quantas == 2);
  CHECK(conta(87).quantas == 1);
  CHECK(conta(79).quantas == 1);
  CHECK(conta(78).quantas == 0);
  // Sem peça alguma á direita, o meio toma o que as fixas deixam, pouco ou nada.
  CHECK(conta(60).meio == 9);
  CHECK(conta(51).meio == 0);
  CHECK(conta(40).meio == 0);
  CHECK(tui::conta_da_fita(167, 51, {}).quantas == 0);
}

// AS CAIXAS na ordem d'elle (issue #134): as abas principiam na primeira
// collunha, os botões colam-se a ellas, e o meio toma do 51 ao 105.
TEST_CASE("as caixas seguem a ordem da fita, e a da palavra sahe da do segmento") {
  tui::CaixasDoCabecalho caixas;
  papel(tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong, {}, 167,
                                   &caixas, tui::Focavel::Pauta, 2),
        167, 2);
  CHECK(caixas.aba_mysong.x_min == 0);
  CHECK(caixas.aba_mysong.x_max == 14);
  CHECK(caixas.aba_playlists.x_min == 16);
  CHECK(caixas.aba_download.x_max == 41);
  CHECK(caixas.botao_tocar.x_min == 43);
  CHECK(caixas.botao_seguinte.x_max == 53);
  CHECK(caixas.trilho.x_min == 55);
  CHECK(caixas.trilho.x_max == 105);
  CHECK(caixas.tempo.x_min == 107);
  CHECK(caixas.ajuda.x_max == 166);
  // A caixa do segmento tem as fileiras que a fita tem, e a da palavra tira as
  // suas d'ella: é d'ahi que a chapa em XIROD nasce.
  CHECK(caixas.aba_mysong.y_min == 0);
  CHECK(caixas.aba_mysong.y_max == 1);
  const ftxui::Box palavra = tui::caixa_da_palavra(caixas.aba_mysong);
  CHECK(palavra.y_max == 1);
  CHECK(palavra.x_min == caixas.aba_mysong.x_min + 3);
}

// A FITA ALTA (a altura continua a ser parametro): o fundo de CADA segmento
// nas duas fileiras, e o rotulo em mono na de CIMA.
TEST_CASE("a fita alta pinta o fundo nas duas linhas e o rotulo na de cima") {
  const ftxui::Screen tela =
      papel(tui::elemento_do_cabecalho(tocando(), tui::Aba::Playlists, {}, 167,
                                       nullptr, tui::Focavel::Pauta, 2),
            167, 2);
  CHECK(pedaco(tela, 16, 13, 0) == " \U000f0cb8 PLAYLISTS ");
  CHECK(pedaco(tela, 16, 13, 1) == "             ");
  // O FUNDO é o mesmo nas duas fileiras, peça por peça: aba corrente, aba
  // apagada, botão, meio e ponta direita.
  for (const int x : {18, 2, 44, 70, 145})
    CHECK(tela.PixelAt(x, 1).background_color ==
          tela.PixelAt(x, 0).background_color);
  CHECK(tela.PixelAt(18, 1).background_color == cor(tk::launcher_glow));
  CHECK(tela.PixelAt(44, 1).background_color == cor(tk::panel_hi));
  CHECK(tela.PixelAt(70, 1).background_color == cor(tk::panel));
  // E a seta da junção repete-se em baixo, senão os dous fundos encostavam-se
  // em quadrado e a emenda via-se.
  CHECK(pedaco(tela, 15, 1, 1) == "\ue0b0");
  CHECK(pedaco(tela, 54, 1, 1) == "\ue0b0");
  CHECK(pedaco(tela, 106, 1, 1) == "\ue0b2");
}

// AS TINTAS. A aba corrente é BLOCO SOLIDO, v600 com texto v50, que é o gesto do
// site d'elle onde o que está sob a mão vira bloco cheio; as outras ficam no
// raised do chrome. O modo aceso é glow_core, e o apagado é text_muted.
TEST_CASE("a aba corrente sahe em bloco solido, e as outras no repouso") {
  const ftxui::Screen tela = papel(
      tui::elemento_do_cabecalho(tocando(), tui::Aba::Playlists, {}, 167), 167);
  CHECK(tela.PixelAt(18, 0).background_color == cor(tk::launcher_glow));
  CHECK(tela.PixelAt(18, 0).foreground_color == cor(tk::vacuo));
  CHECK(tela.PixelAt(2, 0).background_color == cor(tk::raised));
  CHECK(tela.PixelAt(2, 0).foreground_color == cor(tk::text_primary));
  CHECK(tela.PixelAt(32, 0).background_color == cor(tk::raised));
  // Os botões vestem panel_hi com o glifo em glow_core: é o glow CONTIDO da
  // regra da Casa, que accende no que TOCA e nunca no fundo todo.
  CHECK(tela.PixelAt(44, 0).background_color == cor(tk::panel_hi));
  CHECK(tela.PixelAt(44, 0).foreground_color == cor(tk::launcher_glow));
  // O meio veste `panel`, que é o degrau de fundo, e não o da fita: o meio é
  // o chão, e as peças pousam n'elle. E o remate da esquerda assenta no mesmo
  // panel, e não em preto.
  CHECK(tela.PixelAt(70, 0).background_color == cor(tk::panel));
  CHECK(tela.PixelAt(54, 0).background_color == cor(tk::panel));
  CHECK(tela.PixelAt(106, 0).background_color == cor(tk::panel));
}


TEST_CASE("o modo aceso accende, e o apagado guarda o logar sem sommir") {
  tui::Retracto posto = tocando();
  const ftxui::Screen quieto =
      papel(tui::elemento_do_cabecalho(posto, tui::Aba::MySong, {}, 167), 167);
  CHECK(quieto.PixelAt(134, 0).foreground_color == cor(tk::text_muted));
  CHECK(quieto.PixelAt(149, 0).foreground_color == cor(tk::text_muted));
  posto.embaralhado = true;
  posto.repeticao = nu::Repeticao::Uma;
  const ftxui::Screen aceso =
      papel(tui::elemento_do_cabecalho(posto, tui::Aba::MySong, {}, 167), 167);
  CHECK(aceso.PixelAt(134, 0).foreground_color == cor(tk::glow_core));
  CHECK(aceso.PixelAt(149, 0).foreground_color == cor(tk::glow_core));
  // A collunha é a MESMA: o segmento apagado guarda o logar do aceso, e o nome
  // da faixa não salta de sitio quando o operador tecla `z`.
  CHECK(pedaco(aceso, 132, 14) == " \U000f049d EMBARALHAR ");
  // E a repetição de UMA troca o glifo, sem mudar a palavra nem a largura.
  CHECK(pedaco(aceso, 147, 11) == " \U000f0458 REPETIR ");
}

TEST_CASE("as tres teclas de algarismo vão á aba que dizem") {
  const tui::Aba alvos[3] = {tui::Aba::MySong, tui::Aba::Playlists,
                             tui::Aba::Download};
  for (int i = 0; i < 3; ++i) {
    const tui::OrdemDaAba d_ella = tui::ordem_da_aba(
        ftxui::Event::Character(static_cast<char>('1' + i)));
    CHECK(d_ella.gesto == tui::GestoDaAba::Vai);
    CHECK(d_ella.aba == alvos[i]);
  }
  // O Tab e o Shift+Tab cyclam, e o `o` cycla a vista. O `4` não é aba alguma.
  CHECK(tui::ordem_da_aba(ftxui::Event::Tab).gesto == tui::GestoDaAba::Cycla);
  CHECK(tui::ordem_da_aba(ftxui::Event::TabReverse).gesto ==
        tui::GestoDaAba::Cycla);
  CHECK(tui::ordem_da_aba(ftxui::Event::Character('o')).gesto ==
        tui::GestoDaAba::CyclaVista);
  // Tecla alheia é ALHEIA, e é isso que faz o atalho de sempre valer: o `q`, o
  // espaço e as setas seguem á taboada do commando e fazem o que sempre fizeram.
  const ftxui::Event alheias[6] = {
      ftxui::Event::Character('4'), ftxui::Event::Character('q'),
      ftxui::Event::Character(' '), ftxui::Event::ArrowLeft,
      ftxui::Event::Return,         ftxui::Event::Custom};
  for (const ftxui::Event& qual : alheias)
    CHECK(tui::ordem_da_aba(qual).gesto == tui::GestoDaAba::Alheio);
}

TEST_CASE("o Tab cycla as tres abas e torna ao principio") {
  CHECK(tui::aba_seguinte(tui::Aba::MySong) == tui::Aba::Playlists);
  CHECK(tui::aba_seguinte(tui::Aba::Playlists) == tui::Aba::Download);
  CHECK(tui::aba_seguinte(tui::Aba::Download) == tui::Aba::MySong);
  // Tres Tabs tornam ao ponto de partida: a fita não tem ponta que prenda.
  tui::Aba onde = tui::Aba::Playlists;
  for (int i = 0; i < 3; ++i) onde = tui::aba_seguinte(onde);
  CHECK(onde == tui::Aba::Playlists);
}

TEST_CASE("a aba diz a secção, e a secção diz a aba que accende") {
  CHECK(tui::secao_da_aba(tui::Aba::MySong) == tui::Secao::Busca);
  CHECK(tui::secao_da_aba(tui::Aba::Playlists) == tui::Secao::Rois);
  CHECK(tui::secao_da_aba(tui::Aba::Download) == tui::Secao::Rede);
  // Os degraus de DENTRO accendem a aba d'elles, e não aba alguma: dentro de
  // uma lista está-se nas PLAYLISTS, e no catalogo está-se no DOWNLOAD.
  CHECK(tui::aba_da_secao(tui::Secao::NoRol) == tui::Aba::Playlists);
  CHECK(tui::aba_da_secao(tui::Secao::Lista) == tui::Aba::Download);
  for (const tui::Secao qual : {tui::Secao::Busca, tui::Secao::Artistas,
                                tui::Secao::Albuns, tui::Secao::Faixas})
    CHECK(tui::aba_da_secao(qual) == tui::Aba::MySong);
  // E a volta fecha: a secção em que a aba abre accende a MESMA aba.
  for (const tui::Aba qual : {tui::Aba::MySong, tui::Aba::Playlists,
                              tui::Aba::Download})
    CHECK(tui::aba_da_secao(tui::secao_da_aba(qual)) == qual);
}

TEST_CASE("o `o` cycla faixas, artistas e albuns, e salta o que não tem chão") {
  CHECK(tui::vista_seguinte(tui::Secao::Busca, false) == tui::Secao::Artistas);
  // Dos ARTISTAS desce-se ao eleito, e essa é a terceira vista. Sem eleito a
  // que descer, o cyclo salta-a: vista sem chão seria tecla a não fazer nada.
  CHECK(tui::vista_seguinte(tui::Secao::Artistas, true) == tui::Secao::Albuns);
  CHECK(tui::vista_seguinte(tui::Secao::Artistas, false) == tui::Secao::Busca);
  CHECK(tui::vista_seguinte(tui::Secao::Albuns, true) == tui::Secao::Busca);
  // Dentro de um album o `o` sobe á vista plana, e não desfaz a navegação: de
  // desfazer já cuidam o Escape e o Backspace.
  CHECK(tui::vista_seguinte(tui::Secao::Faixas, true) == tui::Secao::Busca);
  // Fóra das MY SONG a tecla leva ás faixas, que é a aba a que ella pertence.
  for (const tui::Secao qual : {tui::Secao::Rois, tui::Secao::NoRol,
                                tui::Secao::Rede, tui::Secao::Lista})
    CHECK(tui::vista_seguinte(qual, true) == tui::Secao::Busca);
}

TEST_CASE("a chapa diz a vista sómente onde ella se cycla") {
  CHECK(tui::nome_da_vista(tui::Secao::Busca) == "FAIXAS");
  CHECK(tui::nome_da_vista(tui::Secao::Artistas) == "ARTISTAS");
  CHECK(tui::nome_da_vista(tui::Secao::Albuns) == "ÁLBUNS");
  CHECK(tui::nome_da_vista(tui::Secao::Faixas) == "ÁLBUNS");
  // Fóra das MY SONG a palavra sahe VAZIA: chapa que dissesse «FAIXAS» n'uma
  // lista de listas prometteria uma tecla que alli não faz cousa alguma.
  for (const tui::Secao qual : {tui::Secao::Rois, tui::Secao::NoRol,
                                tui::Secao::Rede, tui::Secao::Lista})
    CHECK(tui::nome_da_vista(qual).empty());
}

// O MEIO da fita é o trilho de sempre (issue #134): o andado em v600, o que
// falta em line_dim, e a fronteira aferida dos DOUS lados.
TEST_CASE("o meio anda em v600, e o que falta fica em line_dim") {
  tui::Retracto meio = tocando();
  meio.posicao = 50.0;
  meio.duracao = 100.0;
  tui::CaixasDoCabecalho caixas;
  const ftxui::Screen tela = papel(
      tui::elemento_do_cabecalho(meio, tui::Aba::MySong, {}, 167, &caixas), 167);
  const int andadas = static_cast<int>(tui::enchimento(50.0, 100.0, 51));
  CHECK(andadas >= 25);
  CHECK(andadas <= 26);
  CHECK(tela.PixelAt(55, 0).foreground_color == cor(tk::v600));
  CHECK(tela.PixelAt(55 + andadas - 1, 0).foreground_color == cor(tk::v600));
  CHECK(tela.PixelAt(55 + andadas, 0).foreground_color == cor(tk::line_dim));
  CHECK(tela.PixelAt(105, 0).foreground_color == cor(tk::line_dim));
  // No principio da faixa cella alguma anda, e no fim andam todas.
  tui::Retracto principio = tocando();
  principio.posicao = 0.0;
  CHECK(papel(tui::elemento_do_cabecalho(principio, tui::Aba::MySong, {}, 167),
              167)
            .PixelAt(55, 0)
            .foreground_color == cor(tk::line_dim));
  tui::Retracto fim = tocando();
  fim.posicao = fim.duracao;
  CHECK(papel(tui::elemento_do_cabecalho(fim, tui::Aba::MySong, {}, 167), 167)
            .PixelAt(105, 0)
            .foreground_color == cor(tk::v600));
  // Com o FOCO no meio, o andado accende em glow_core, e o que falta fica.
  const ftxui::Screen aceso =
      papel(tui::elemento_do_cabecalho(meio, tui::Aba::MySong, {}, 167, nullptr,
                                       tui::Focavel::Trilho),
            167);
  CHECK(aceso.PixelAt(55, 0).foreground_color == cor(tk::glow_core));
  CHECK(aceso.PixelAt(105, 0).foreground_color == cor(tk::line_dim));
}

TEST_CASE("não cabendo, as peças da direita cedem o logar INTEIRAS") {
  // As fronteiras, contadas á mão (issue #134): as fixas pedem 51 e o meio
  // guarda 12; a ponta direita cabe inteira do 124 para cima, quatro peças do
  // 115, tres do 103, duas do 88, uma do 79. Cedem INTEIRAS, e nunca aparadas:
  // aparar partiria um par de tinta e fundo ao meio, que é a emenda que o
  // aceite proscreve.
  const auto linha_em = [](std::size_t larga) {
    return pedaco(papel(tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong,
                                                   {}, larga),
                        static_cast<int>(larga)),
                  0, static_cast<int>(larga));
  };
  CHECK(linha_em(128).find("HELP") != std::string::npos);
  CHECK(linha_em(127).find("HELP") == std::string::npos);
  CHECK(linha_em(119).find("REPETIR") != std::string::npos);
  CHECK(linha_em(118).find("REPETIR") == std::string::npos);
  CHECK(linha_em(107).find("EMBARALHAR") != std::string::npos);
  CHECK(linha_em(106).find("EMBARALHAR") == std::string::npos);
  CHECK(linha_em(92).find("100%") != std::string::npos);
  CHECK(linha_em(91).find("100%") == std::string::npos);
  CHECK(linha_em(83).find("00:19") != std::string::npos);
  CHECK(linha_em(82).find("00:19") == std::string::npos);
  // As tres ABAS e os botões ficam em toda largura: ellas são a navegação, e
  // navegação que sommisse deixaria o operador sem porta para a secção seguinte.
  for (const std::size_t larga : {60, 78, 88, 123, 167}) {
    CHECK(linha_em(larga).find("MY 0 SONG's") != std::string::npos);
    CHECK(linha_em(larga).find("DOWNLOAD") != std::string::npos);
  }
}

// O SEGMENTO DO VOLUME calado (issue #106). A palavra em logar do numero, em
// glow_hot, que é a tinta da urgencia d'esta Casa; e nas MESMAS oito collunhas,
// donde a fita não anda debaixo do olho de quem só carregou no F9.
TEST_CASE("o segmento do volume diz MUDO, e nas mesmas oito collunhas") {
  tui::Retracto calado = tocando();
  calado.volume = 70;
  calado.mudo = true;
  const ftxui::Screen tela = papel(
      tui::elemento_do_cabecalho(calado, tui::Aba::MySong, {}, 167), 167);
  CHECK(pedaco(tela, 123, 8) == " \U000f075f MUDO ");
  CHECK(tela.PixelAt(125, 0).foreground_color == cor(tk::glow_hot));
  // A conta da fita NÃO anda: o EMBARALHAR fica na collunha em que ficava com o
  // numero, e o nome da faixa com elle.
  CHECK(pedaco(tela, 132, 14) == " \U000f049d EMBARALHAR ");

  // E o volume ZERO por escolha do operador segue a mostrar o numero: sómente a
  // ordem de calar diz a palavra, que são duas cousas differentes e a fita não
  // ha de as confundir.
  tui::Retracto no_zero = tocando();
  no_zero.volume = 0;
  const ftxui::Screen quieto = papel(
      tui::elemento_do_cabecalho(no_zero, tui::Aba::MySong, {}, 167), 167);
  CHECK(pedaco(quieto, 123, 8) == " \U000f075f   0% ");
  CHECK(quieto.PixelAt(125, 0).foreground_color == cor(tk::text_muted));
}

//   Da lavra do eminente Doutor BURAGA KYO., buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
