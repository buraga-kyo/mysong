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
#include <vector>

#include "tui/cabecalho.hpp"
#include "tui/tokens.hpp"

namespace nu = mysong::nucleo;
namespace tk = mysong::tui::tokens;
namespace tui = mysong::tui;

namespace {

// linha_do — o cabeçalho pintado, lido cella a cella. O `ToString` metteria
// escape no meio dos bytes, e contar bytes seria contar a tinta.
ftxui::Screen papel(ftxui::Element quadro, int largura, int altura = 1) {
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(largura),
                                              ftxui::Dimension::Fixed(altura));
  ftxui::Render(ecran, quadro);
  return ecran;
}

// pedaco — as `quantas` cellas a partir da collunha `x`, na fileira zero. Por
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
TEST_CASE("a linha do alto sahe egual á cadeia escripta á mão") {
  const ftxui::Screen tela = papel(
      tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong,
                                 "Montagem Lunar Celestia 1.0 (SLOWED)", 167),
      167);

  // A conta, feita á mão (issue #125): os botões pedem 12 collunhas (3 cada e
  // as 3 setas), o grupo das abas pede 39 (11, 13 e 12 das palavras e as 3
  // setas) e a ponta direita pede 61 com o HELP (issue #133). O grupo principia
  // no CENTRO EXACTO, que é 167 menos 39 a dividir por dous: a collunha 64. Ao
  // nome ficam as 52 que vão dos botões ao grupo, e ao vão da outra banda as 3
  // que sobram.
  CHECK(pedaco(tela, 0, 3) == " \U000f03e4 ");    // toca: o botão diz PAUSAR
  CHECK(pedaco(tela, 4, 3) == " \U000f04ae ");    // anterior
  CHECK(pedaco(tela, 8, 3) == " \U000f04ad ");    // seguinte
  CHECK(pedaco(tela, 11, 1) == "\ue0b0");
  // O nome CENTRADO nas 52 collunhas d'elle: 36 de titulo, 8 de cada banda.
  CHECK(pedaco(tela, 12, 52) ==
        "        Montagem Lunar Celestia 1.0 (SLOWED)        ");
  CHECK(pedaco(tela, 64, 11) == " \U000f075a MY SONG ");
  CHECK(pedaco(tela, 75, 1) == "\ue0b0");
  CHECK(pedaco(tela, 76, 13) == " \U000f0cb8 PLAYLISTS ");
  CHECK(pedaco(tela, 90, 12) == " \U000f01da DOWNLOAD ");
  CHECK(pedaco(tela, 103, 3) == "   ");
  CHECK(pedaco(tela, 106, 1) == "\ue0b2");        // a seta de entrada da direita
  CHECK(pedaco(tela, 107, 15) == " 00:19 / 03:09 ");
  CHECK(pedaco(tela, 123, 8) == " \U000f057e 100% ");
  // O volume a TRES algarismos, enchido á esquerda: sem elle, cada `-` movia
  // as quatro peças da direita uma collunha, e o nome da faixa com ellas.
  tui::Retracto baixo = tocando();
  baixo.volume = 95;
  CHECK(pedaco(papel(tui::elemento_do_cabecalho(baixo, tui::Aba::MySong, "x",
                                                167),
                     167),
               123, 8) == " \U000f057e  95% ");
  CHECK(pedaco(tela, 132, 14) == " \U000f049d EMBARALHAR ");
  CHECK(pedaco(tela, 147, 11) == " \U000f0456 REPETIR ");
  // O HELP (issue #133) é a ponta: oito collunhas, com o glifo da fonte d'elle.
  CHECK(pedaco(tela, 158, 1) == "\ue0b2");
  CHECK(pedaco(tela, 159, 8) == " \U000f02d7 HELP ");
  // E a linha FECHA a largura: nada sobra, e nada transborda.
  CHECK(pedaco(tela, 0, 167) == pedaco(tela, 0, 200));
}

TEST_CASE("a linha fecha a largura exacta, e o nome toma o que sobra") {
  // Em 120 o centro cae na collunha 40, d'onde ao nome ficam as 28 que vão dos
  // 12 dos botões até elle. O REPETIR cede, que a ponta direita já lá não
  // cabia; o nome que não cabe corta-se com «…», e o que cabe centra-se.
  const ftxui::Screen larga = papel(
      tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong,
                                 "Montagem Lunar Celestia 1.0 (SLOWED)", 120),
      120);
  CHECK(pedaco(larga, 12, 28) == "Montagem Lunar Celestia 1.0…");
  CHECK(pedaco(larga, 40, 11) == " \U000f075a MY SONG ");
  CHECK(pedaco(larga, 80, 1) == "\ue0b2");
  // Nome curto: o fundo do segmento veste a collunha inteira, e o que sobra
  // enche-se de espaço. Buraco escuro no meio da fita lê-se como emenda.
  const ftxui::Screen curto =
      papel(tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong, "NO FEAR!",
                                       120),
            120);
  CHECK(pedaco(curto, 12, 28) == "          NO FEAR!          ");
  // E o grupo fica na MESMA collunha com o nome comprido e com o curto: é o
  // que o centro CONTADO dá, e o que enchimento elastico algum daria.
  CHECK(pedaco(curto, 40, 11) == " \U000f075a MY SONG ");
  // E nada tocando, o meio DIZ que nada toca, em vez de ficar em branco.
  tui::Retracto parado;
  CHECK(pedaco(papel(tui::elemento_do_cabecalho(parado, tui::Aba::MySong, "",
                                                120),
                     120),
               20, 11) == "(nada toca)");
}


// A CONTA da fita, interrogada sem se pintar cousa alguma: os doze collunhas
// dos botões, os trinta e nove do grupo, e as cinco larguras da ponta direita.
TEST_CASE("a conta da fita cede as pontas antes de o grupo deixar o centro") {
  const std::vector<std::size_t> direita = {0, 16, 25, 40, 52};
  const auto conta = [&](std::size_t larga) {
    return tui::conta_da_fita(larga, 12, 39, direita);
  };
  CHECK(conta(167).ao_centro);
  CHECK(conta(167).comeca == 64);
  CHECK(conta(167).quantas == 4);
  CHECK(conta(167).nome == 52);
  CHECK(conta(167).depois == 12);
  // A somma FECHA a largura: sem isto, a fita deixaria vão ou transbordaria.
  for (const std::size_t larga : {70, 88, 100, 118, 142, 166, 167, 200}) {
    const tui::ContaDaFita c = conta(larga);
    CHECK(12 + c.nome + 39 + c.depois + direita[c.quantas] == larga);
  }
  // A ESCADA: a ponta direita cede do fim para o principio, e o grupo sómente
  // deixa o centro quando ao nome já não sobram as sete collunhas do minimo.
  CHECK(conta(141).quantas == 3);
  CHECK(conta(117).quantas == 2);
  CHECK(conta(87).quantas == 1);
  CHECK(conta(69).quantas == 0);
  CHECK(conta(77).ao_centro);
  CHECK_FALSE(conta(76).ao_centro);
}

// O CENTRO EXACTO (issue #125): a collunha do grupo é a largura menos a d'elle,
// a dividir por dous. Pede-se em largura PAR e IMPAR, com nome curto, comprido
// e nenhum: é n'isto que o centro CONTADO se aparta do centro por enchimento
// elastico, que aquelle fica quieto e este segue o nome da faixa.
TEST_CASE("o grupo das abas fica na mesma collunha, mude ou não o nome") {
  const auto onde = [](std::size_t larga, const char* nome) {
    tui::CaixasDoCabecalho caixas;
    papel(tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong, nome, larga,
                                     &caixas, tui::Focavel::Pauta, 2),
          static_cast<int>(larga), 2);
    return caixas.aba_mysong;
  };
  // O grupo pede 39 collunhas: em 166 principia na 63, em 167 na 64.
  for (const std::size_t larga : {166, 167}) {
    const int comeca = static_cast<int>((larga - 39) / 2);
    CHECK(onde(larga, "NO FEAR!").x_min == comeca);
    CHECK(onde(larga, "Montagem Lunar Celestia 1.0 (SLOWED)").x_min == comeca);
    CHECK(onde(larga, "").x_min == comeca);
  }
  // E a caixa do segmento tem DUAS fileiras, d'onde a caixa da palavra tira as
  // suas: é d'ahi que a chapa em XIROD alta da issue irmã ha de nascer.
  const ftxui::Box segmento = onde(167, "NO FEAR!");
  CHECK(segmento.y_min == 0);
  CHECK(segmento.y_max == 1);
  const ftxui::Box palavra = tui::caixa_da_palavra(segmento);
  CHECK(palavra.y_min == 0);
  CHECK(palavra.y_max == 1);
  CHECK(palavra.x_min == segmento.x_min + 3);
}

// A FITA ALTA do pé (issue #125): duas linhas, o fundo de CADA segmento nas
// duas, e o rotulo em mono na de CIMA. Lê-se em écran de papel de duas
// fileiras, cella a cella, que é o unico modo de o affirmar sem terminal.
TEST_CASE("a fita alta pinta o fundo nas duas linhas e o rotulo na de cima") {
  const ftxui::Screen tela =
      papel(tui::elemento_do_cabecalho(tocando(), tui::Aba::Playlists,
                                       "NO FEAR!", 167, nullptr,
                                       tui::Focavel::Pauta, 2),
            167, 2);
  CHECK(pedaco(tela, 76, 13, 0) == " \U000f0cb8 PLAYLISTS ");
  CHECK(pedaco(tela, 76, 13, 1) == "             ");
  // O FUNDO é o mesmo nas duas fileiras, peça por peça: aba corrente, aba
  // apagada, botão, nome, vão do outro lado e ponta direita.
  for (const int x : {78, 66, 1, 30, 104, 145})
    CHECK(tela.PixelAt(x, 1).background_color ==
          tela.PixelAt(x, 0).background_color);
  CHECK(tela.PixelAt(78, 1).background_color == cor(tk::v600));
  CHECK(tela.PixelAt(1, 1).background_color == cor(tk::panel_hi));
  // E a seta da junção repete-se em baixo, senão os dous fundos encostavam-se
  // em quadrado e a emenda via-se.
  CHECK(pedaco(tela, 75, 1, 1) == "\ue0b0");
  CHECK(pedaco(tela, 11, 1, 1) == "\ue0b0");
  CHECK(pedaco(tela, 106, 1, 1) == "\ue0b2");
}

// AS TINTAS. A aba corrente é BLOCO SOLIDO, v600 com texto v50, que é o gesto do
// site d'elle onde o que está sob a mão vira bloco cheio; as outras ficam no
// raised do chrome. O modo aceso é glow_core, e o apagado é text_muted.
TEST_CASE("a aba corrente sahe em bloco solido, e as outras no repouso") {
  const ftxui::Screen tela = papel(
      tui::elemento_do_cabecalho(tocando(), tui::Aba::Playlists, "x", 167),
      167);
  CHECK(tela.PixelAt(78, 0).background_color == cor(tk::v600));
  CHECK(tela.PixelAt(78, 0).foreground_color == cor(tk::v50));
  CHECK(tela.PixelAt(66, 0).background_color == cor(tk::raised));
  CHECK(tela.PixelAt(66, 0).foreground_color == cor(tk::text_primary));
  CHECK(tela.PixelAt(92, 0).background_color == cor(tk::raised));
  // Os botões vestem panel_hi com o glifo em glow_core: é o glow CONTIDO da
  // regra da Casa, que accende no que TOCA e nunca no fundo todo.
  CHECK(tela.PixelAt(1, 0).background_color == cor(tk::panel_hi));
  CHECK(tela.PixelAt(1, 0).foreground_color == cor(tk::glow_core));
  // O nome veste `panel`, que é o degrau de fundo, e não o da fita; e o vão
  // da outra banda do grupo veste o mesmo, que a fita ha de ser continua.
  CHECK(tela.PixelAt(30, 0).background_color == cor(tk::panel));
  CHECK(tela.PixelAt(104, 0).background_color == cor(tk::panel));
}


TEST_CASE("o modo aceso accende, e o apagado guarda o logar sem sommir") {
  tui::Retracto posto = tocando();
  const ftxui::Screen quieto =
      papel(tui::elemento_do_cabecalho(posto, tui::Aba::MySong, "x", 167), 167);
  CHECK(quieto.PixelAt(134, 0).foreground_color == cor(tk::text_muted));
  CHECK(quieto.PixelAt(149, 0).foreground_color == cor(tk::text_muted));
  posto.embaralhado = true;
  posto.repeticao = nu::Repeticao::Uma;
  const ftxui::Screen aceso =
      papel(tui::elemento_do_cabecalho(posto, tui::Aba::MySong, "x", 167), 167);
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

TEST_CASE("o trilho anda em v600, e o que falta fica em line_dim") {
  tui::Retracto meio = tocando();
  meio.posicao = 50.0;
  meio.duracao = 100.0;
  const ftxui::Screen tela =
      papel(tui::elemento_do_trilho(meio, 40), 40);
  // Cincoenta por cento de quarenta: vinte cellas andadas, vinte por andar. A
  // fronteira afere-se dos DOUS lados, que é o que apanha o erro de uma cella.
  CHECK(tela.PixelAt(0, 0).foreground_color == cor(tk::v600));
  CHECK(tela.PixelAt(19, 0).foreground_color == cor(tk::v600));
  CHECK(tela.PixelAt(20, 0).foreground_color == cor(tk::line_dim));
  CHECK(tela.PixelAt(39, 0).foreground_color == cor(tk::line_dim));
  // O glifo é o traço PESADO em toda a largura, andado ou não: traço leve some
  // no fundo violaceo, e trilho que se não vê não diz onde a faixa vae.
  std::string traco;
  for (int i = 0; i < 40; ++i) traco += "\u2501";
  CHECK(pedaco(tela, 0, 40) == traco);
  // No principio da faixa cella alguma anda, e no fim andam todas: o trilho é
  // UM elemento, e não duas metades de que uma teria largura zero.
  tui::Retracto principio = tocando();
  principio.posicao = 0.0;
  CHECK(papel(tui::elemento_do_trilho(principio, 40), 40)
            .PixelAt(0, 0)
            .foreground_color == cor(tk::line_dim));
  tui::Retracto fim = tocando();
  fim.posicao = fim.duracao;
  CHECK(papel(tui::elemento_do_trilho(fim, 40), 40)
            .PixelAt(39, 0)
            .foreground_color == cor(tk::v600));
}

TEST_CASE("não cabendo, as peças da direita cedem o logar INTEIRAS") {
  // As fronteiras, contadas á mão sobre o centro (issue #125): a ponta direita
  // cabe inteira do 142 para cima, tres peças do 118, duas do 88, uma do 70.
  // São mais largas que as da fita antiga, e é o preço do centro: o grupo toma
  // o meio, e as pontas dividem o que sobra em duas metades eguaes.
  // Cedem INTEIRAS, e nunca aparadas: aparar partiria um par de tinta e fundo
  // ao meio, que é a emenda que o aceite proscreve.
  const auto linha_em = [](std::size_t larga) {
    return pedaco(papel(tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong,
                                                   "NO FEAR!", larga),
                        static_cast<int>(larga)),
                  0, static_cast<int>(larga));
  };
  // O HELP (issue #133) é o primeiro a ceder: cabe do 160 para cima.
  CHECK(linha_em(160).find("HELP") != std::string::npos);
  CHECK(linha_em(159).find("HELP") == std::string::npos);
  CHECK(linha_em(142).find("REPETIR") != std::string::npos);
  CHECK(linha_em(141).find("REPETIR") == std::string::npos);
  CHECK(linha_em(118).find("EMBARALHAR") != std::string::npos);
  CHECK(linha_em(117).find("EMBARALHAR") == std::string::npos);
  CHECK(linha_em(88).find("100%") != std::string::npos);
  CHECK(linha_em(87).find("100%") == std::string::npos);
  CHECK(linha_em(70).find("00:19") != std::string::npos);
  CHECK(linha_em(69).find("00:19") == std::string::npos);
  // As tres ABAS ficam em toda largura: ellas são a navegação, e navegação que
  // sommisse deixaria o operador sem porta para a secção seguinte.
  for (const std::size_t larga : {60, 69, 88, 141, 167})
    CHECK(linha_em(larga).find("MY SONG") != std::string::npos);
}

// O SEGMENTO DO VOLUME calado (issue #106). A palavra em logar do numero, em
// glow_hot, que é a tinta da urgencia d'esta Casa; e nas MESMAS oito collunhas,
// donde a fita não anda debaixo do olho de quem só carregou no F9.
TEST_CASE("o segmento do volume diz MUDO, e nas mesmas oito collunhas") {
  tui::Retracto calado = tocando();
  calado.volume = 70;
  calado.mudo = true;
  const ftxui::Screen tela = papel(
      tui::elemento_do_cabecalho(calado, tui::Aba::MySong, "x", 167), 167);
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
      tui::elemento_do_cabecalho(no_zero, tui::Aba::MySong, "x", 167), 167);
  CHECK(pedaco(quieto, 123, 8) == " \U000f075f   0% ");
  CHECK(quieto.PixelAt(125, 0).foreground_color == cor(tk::text_muted));
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
