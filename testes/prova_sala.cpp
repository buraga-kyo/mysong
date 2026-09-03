// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA SALA — testes/prova_sala.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada pura da sala, e mais adiante os écrans de PAPEL d'ella, lidos
// cella a cella. Terminal algum se abre, banco algum: os retractos que a sala
// pede armam-se á mão, que é o que faz d'ella peça aferivel.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <string>
#include <vector>

#include "nucleo/letra.hpp"
#include "tui/espectro.hpp"
#include "tui/sala.hpp"
#include "tui/tabella.hpp"
#include "tui/tokens.hpp"

namespace nu = mysong::nucleo;
namespace tk = mysong::tui::tokens;
namespace tui = mysong::tui;

namespace {

// papel — o écran de PAPEL, lido cella a cella: o `ToString` metteria escape
// no meio dos bytes, e contar bytes seria contar a tinta.
ftxui::Screen papel(ftxui::Element quadro, int largura, int altura) {
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(largura),
                                              ftxui::Dimension::Fixed(altura));
  ftxui::Render(ecran, quadro);
  return ecran;
}

std::string linha_de(const ftxui::Screen& ecran, int y) {
  std::string dita;
  // Cella INTACTA vale espaço, e não nada: o FTXUI deixa-lhe o glifo vazio, e
  // sommar vazio faria o vão de duas collunhas somir da cadeia, com todo indice
  // seguinte a apontar para a collunha errada. Medido, e não suposto.
  for (int x = 0; x < ecran.dimx(); ++x) {
    const std::string& glifo = ecran.PixelAt(x, y).character;
    dita += glifo.empty() ? " " : glifo;
  }
  return dita;
}

// capa_de — a capa que o chafa devolveria, armada á mão. Chafa algum corre aqui.
nu::CapaPintada capa_de(std::size_t quantas, std::size_t largura) {
  nu::CapaPintada capa;
  capa.achada = quantas > 0;
  for (std::size_t l = 0; l < quantas; ++l)
    capa.linhas.push_back({nu::Corrida{std::string(largura, '#')}});
  return capa;
}

// collunha_de — a COLLUNHA em que tal glifo pousou, e menos um não o havendo.
// Por cella, e não por byte: busca em cadeia mentiria com o glifo de tres bytes.
int collunha_de(const ftxui::Screen& ecran, int y, const std::string& glifo) {
  for (int x = 0; x < ecran.dimx(); ++x)
    if (ecran.PixelAt(x, y).character == glifo) return x;
  return -1;
}

// cor — a côr do FTXUI que o token nomeia, para se comparar cella a cella.
ftxui::Color cor(std::string_view token) {
  const tk::Triade c = tk::rgb(token);
  return ftxui::Color::RGB(c.r, c.g, c.b);
}

// sala_de — as TRES collunnas montadas como o pintor as monta, para se aferir
// que peça alguma empurra as visinhas. A barra vae por um texto de nove
// collunhas: o que importa aqui é a largura d'ella, e não o conteudo.
ftxui::Screen sala_de(const std::string& nome) {
  tui::Colleccao qual;
  qual.nome = nome;
  qual.quantas = 4;
  qual.duracao = 840;
  return papel(
      ftxui::hbox(
          {ftxui::text(std::string(9, 'B')), ftxui::text("  "),
           ftxui::vbox({tui::elemento_do_cabecalho(qual, capa_de(5, 10), 105),
                        ftxui::text("T")}),
           ftxui::text(" "),
           tui::elemento_do_painel({"a", "b", "c"},
                                   tui::elemento_da_arte(capa_de(5, 39), 39, 5),
                                   ftxui::text("E"), 39)}),
      156, 2);
}

}  // namespace

TEST_CASE("a somma da colleção diz-se por extenso") {
  CHECK(tui::texto_da_duracao(0).empty());
  CHECK(tui::texto_da_duracao(-5).empty());
  CHECK(tui::texto_da_duracao(45) == "45s");
  CHECK(tui::texto_da_duracao(59) == "59s");
  CHECK(tui::texto_da_duracao(60) == "1min");
  CHECK(tui::texto_da_duracao(3599) == "59min");
  CHECK(tui::texto_da_duracao(3600) == "1h00");
  CHECK(tui::texto_da_duracao(3900) == "1h05");
  CHECK(tui::texto_da_duracao(4980) == "1h23");
}

TEST_CASE("a conta da colleção muda de substantivo com a especie") {
  CHECK(tui::texto_da_conta(4, tui::Especie::Faixas, 840) == "4 FAIXAS, 14min");
  CHECK(tui::texto_da_conta(1, tui::Especie::Faixas, 0) == "1 FAIXA");
  CHECK(tui::texto_da_conta(0, tui::Especie::Faixas, 0) == "0 FAIXAS");
  CHECK(tui::texto_da_conta(12, tui::Especie::Artistas, 0) == "12 ARTISTAS");
  CHECK(tui::texto_da_conta(1, tui::Especie::Albuns, 0) == "1 ÁLBUM");
  CHECK(tui::texto_da_conta(3, tui::Especie::Listas, 0) == "3 LISTAS");
  CHECK(tui::texto_da_conta(9, tui::Especie::Achados, 0) == "9 ACHADOS");
}

TEST_CASE("a chapa diz a aba e os degraus de dentro, apartados por seta") {
  const std::vector<std::string> alto;
  const std::vector<std::string> fundo = {"Boards of Canada", "Geogaddi"};
  CHECK(tui::onde_da_chapa(tui::Secao::Busca, alto, "") == "MY SONG");
  CHECK(tui::onde_da_chapa(tui::Secao::Artistas, alto, "") == "MY SONG");
  CHECK(tui::onde_da_chapa(tui::Secao::Albuns, fundo, "") ==
        "MY SONG \u25b8 Boards of Canada \u25b8 Geogaddi");
  CHECK(tui::onde_da_chapa(tui::Secao::Rede, alto, "") == "DOWNLOAD");
  CHECK(tui::onde_da_chapa(tui::Secao::Rois, alto, "") == "PLAYLISTS");
  CHECK(tui::onde_da_chapa(tui::Secao::NoRol, {"Funk"}, "") ==
        "PLAYLISTS \u25b8 Funk");
  CHECK(tui::onde_da_chapa(tui::Secao::Lista, alto, "") ==
        "DOWNLOAD \u25b8 SPOTIFY");
  CHECK(tui::onde_da_chapa(tui::Secao::Lista, alto, "Verão") ==
        "DOWNLOAD \u25b8 SPOTIFY \u25b8 Verão");
  // Album sem etiqueta: o degrau existe, mas o nome d'elle é vazio, e degrau
  // sem palavra sahiria como uma seta a apontar para nada.
  CHECK(tui::onde_da_chapa(tui::Secao::Faixas, {"Alan Walker", ""}, "") ==
        "MY SONG \u25b8 Alan Walker");
}

TEST_CASE("cada secção conta a sua especie") {
  CHECK(tui::especie_da_secao(tui::Secao::Artistas) == tui::Especie::Artistas);
  CHECK(tui::especie_da_secao(tui::Secao::Albuns) == tui::Especie::Albuns);
  CHECK(tui::especie_da_secao(tui::Secao::Rois) == tui::Especie::Listas);
  CHECK(tui::especie_da_secao(tui::Secao::Rede) == tui::Especie::Achados);
  CHECK(tui::especie_da_secao(tui::Secao::Busca) == tui::Especie::Faixas);
  CHECK(tui::especie_da_secao(tui::Secao::NoRol) == tui::Especie::Faixas);
}

// A GEOMETRIA nas larguras que a issue #102 nomeia, e de doze a setenta linhas.
// Alvo escripto Á MÃO, e nunca a conta da obra repetida aqui: assertiva que
// compara o valor com a formula que o produziu não pode falhar.
TEST_CASE("a sala esconde o painel abaixo de cem collunhas de tela") {
  for (const std::size_t larga : {40, 60, 80, 99}) {
    const tui::Sala sala = tui::sala_da_tela(larga, 40, false);
    CHECK(sala.painel.vazio());
    CHECK(sala.pauta.largura == larga);  // a pauta toma a tela toda
    CHECK(sala.divisor.vazio());
  }
  const tui::Sala justa = tui::sala_da_tela(100, 40, false);
  CHECK(justa.painel.largura == 50);
  CHECK(justa.pauta.largura == 49);
  CHECK(justa.divisor.x == 49);
}

TEST_CASE("as duas metades repartem a tela, com a collunha do divisor pelo meio") {
  const std::size_t larguras[4] = {120, 160, 167, 200};
  const std::size_t painel[4] = {60, 80, 83, 100};
  for (int i = 0; i < 4; ++i) {
    const tui::Sala sala = tui::sala_da_tela(larguras[i], 67, false);
    CHECK(sala.painel.largura == painel[i]);
    CHECK(sala.pauta.largura + 1 + sala.painel.largura == larguras[i]);
    CHECK(sala.painel.x == sala.pauta.largura + 1);
    CHECK(sala.cabecalho.largura == larguras[i]);
    CHECK(sala.trilho.largura == larguras[i]);
  }
}

TEST_CASE("a tela de cento e sessenta e sete por sessenta e sete") {
  const tui::Sala sala = tui::sala_da_tela(167, 67, false);
  CHECK(sala.cabecalho.y == 0);
  CHECK(sala.trilho.y == 1);
  CHECK(sala.campo.vazio());  // sem campo aberto, linha alguma se lhe reserva
  CHECK(sala.chapa.y == 2);
  CHECK(sala.pauta.y == 3);
  CHECK(sala.pauta.altura == 63);
  CHECK(sala.rodape.y == 66);
  // O tecto da capa: quarenta e cinco por cento de sessenta e quatro é vinte e
  // oito e oito decimos, e a conta inteira trunca em vinte e oito.
  CHECK(sala.capa.altura == 28);
  CHECK(sala.espectro.y == 30);
  CHECK(sala.espectro.altura == 36);
}

TEST_CASE("o campo aberto empurra o corpo uma linha para baixo") {
  const tui::Sala com = tui::sala_da_tela(167, 67, true);
  CHECK(com.campo.y == 2);
  CHECK(com.campo.largura == 167);
  CHECK(com.chapa.y == 3);
  CHECK(com.pauta.altura == 62);
  CHECK(com.rodape.y == 66);
}

// A INVARIANTE de altura, corrida de DOZE a SETENTA linhas, que é o arco que a
// issue #102 nomeia. Não se afere numero por numero: afere-se que as peças se
// fecham sem se sobreporem e sem deixarem fileira por pintar.
TEST_CASE("de doze a setenta linhas a sala fecha a tela sem vão nem sobreposição") {
  for (std::size_t alta = 12; alta <= 70; ++alta) {
    for (const bool campo : {false, true}) {
      const tui::Sala sala = tui::sala_da_tela(167, alta, campo);
      CHECK(sala.cabecalho.altura == 1);
      CHECK(sala.trilho.altura == 1);
      // A pauta começa onde o campo e a chapa acabam, e o rodapé é a ultima.
      const std::size_t alto = 2 + (campo ? 1u : 0u);
      CHECK(sala.chapa.y == alto);
      CHECK(sala.pauta.y == alto + 1);
      CHECK(sala.rodape.y == alta - 1);
      CHECK(sala.pauta.y + sala.pauta.altura == alta - 1);
      if (sala.painel.vazio()) continue;
      CHECK(sala.painel.y == alto);
      CHECK(sala.painel.altura == sala.divisor.altura);
      CHECK(sala.capa.altura + sala.espectro.altura == sala.painel.altura);
      CHECK(sala.espectro.altura >= 6);  // o espectro não desce de seis
      CHECK(sala.espectro.y == sala.capa.y + sala.capa.altura);
    }
  }
}

// A SOBRA do espectro depois de se saber quanto a capa tomou DE FACTO. O
// rectangulo da capa é TECTO, e a de 16 por 9 sahe mais baixa que elle.
TEST_CASE("o espectro toma o que a capa não gastou") {
  const tui::Sala sala = tui::sala_da_tela(167, 67, false);
  CHECK(tui::espectro_abaixo_da(sala, 28).altura == 36);
  CHECK(tui::espectro_abaixo_da(sala, 11).altura == 53);
  CHECK(tui::espectro_abaixo_da(sala, 11).y == sala.painel.y + 11);
  CHECK(tui::espectro_abaixo_da(sala, 0).altura == sala.painel.altura);
  // Capa mais alta que o tecto cinge-se n'elle: sem o cinge, a subtracção em
  // std::size_t daria numero enorme, e a peça pintaria bilhões de linhas.
  CHECK(tui::espectro_abaixo_da(sala, 99).altura == 36);
  CHECK(tui::espectro_abaixo_da(tui::sala_da_tela(80, 40, false), 3).vazio());
}

// A ARTE não tem altura reservada: a de 16 por 9 sahe mais baixa que o tecto.
TEST_CASE("a ficha vem na linha seguinte á ultima da capa") {
  const tui::Ficha ficha{"Dawn Chorus", "Boards of Canada", "Geogaddi"};
  for (int alta : {11, 20}) {
    const nu::CapaPintada capa = capa_de(static_cast<std::size_t>(alta), 39);
    CHECK(tui::linhas_da_arte(capa, 20) == static_cast<std::size_t>(alta));
    const ftxui::Screen tela = papel(
        tui::elemento_do_painel(ficha, tui::elemento_da_arte(capa, 39, 20),
                                ftxui::text(""), 39),
        39, 34);
    CHECK(linha_de(tela, 0).substr(0, 13) == "TOCANDO AGORA");
    CHECK(linha_de(tela, 1) == std::string(39, '#'));
    CHECK(linha_de(tela, alta) == std::string(39, '#'));
    CHECK(linha_de(tela, alta + 1).substr(0, 11) == "Dawn Chorus");
    CHECK(linha_de(tela, alta + 2).substr(0, 16) == "Boards of Canada");
    CHECK(linha_de(tela, alta + 3).substr(0, 8) == "Geogaddi");
  }
}

// O MARCADOR sahe do elemento_da_capa com orla POR FÓRA, e é por isso que se
// lhe pede quatro por trinta e sete: o que se ha de ver são seis por trinta e
// nove, sem transbordar o painel.
TEST_CASE("sem capa o marcador toma seis linhas e a ficha vem na setima") {
  const nu::CapaPintada nenhuma;
  CHECK(tui::linhas_da_arte(nenhuma, 20) == 6);
  const ftxui::Screen tela = papel(
      tui::elemento_do_painel({"Dawn Chorus", "Boards of Canada", "Geogaddi"},
                              tui::elemento_da_arte(nenhuma, 39, 6),
                              ftxui::text(""), 39),
      39, 34);
  CHECK(collunha_de(tela, 4, "\u266b") > 0);
  CHECK(collunha_de(tela, 6, "\u2500") >= 0);
  CHECK(collunha_de(tela, 7, "\u2500") == -1);
  CHECK(linha_de(tela, 7).substr(0, 11) == "Dawn Chorus");
}

TEST_CASE("o cabeçalho diz o nome e a conta e risca o separador") {
  tui::Colleccao qual;
  qual.nome = "GEOGADDI";
  qual.quantas = 4;
  qual.duracao = 840;
  const ftxui::Screen tela =
      papel(tui::elemento_do_cabecalho(qual, capa_de(5, 10), 105), 105, 6);
  CHECK(linha_de(tela, 0).substr(0, 10) == std::string(10, '#'));
  CHECK(linha_de(tela, 1).substr(12, 8) == "GEOGADDI");
  CHECK(linha_de(tela, 3).find("4 FAIXAS, 14min") == 12);
  CHECK(collunha_de(tela, 5, "\u2500") == 0);
  CHECK(collunha_de(tela, 4, "\u2500") == -1);
  // Meio estreito: os chips cedem o logar, e o vão da capa fica de pé. A
  // fronteira é exacta: em sessenta collunhas elles cabem, em cincoenta e nove
  // não, e ahi a linha da conta sahe sósinha.
  CHECK(collunha_de(papel(tui::elemento_do_cabecalho(qual, capa_de(5, 10), 60),
                          60, 6),
                    3, "\u21c4") == 30);
  const ftxui::Screen curta =
      papel(tui::elemento_do_cabecalho(qual, capa_de(5, 10), 59), 59, 6);
  CHECK(collunha_de(curta, 3, "\u21c4") == -1);
  CHECK(linha_de(curta, 1).substr(12, 8) == "GEOGADDI");
}

TEST_CASE("o chip do modo accende quando o modo liga") {
  tui::Colleccao qual;
  qual.nome = "GEOGADDI";
  qual.quantas = 4;
  const ftxui::Screen apagado =
      papel(tui::elemento_do_cabecalho(qual, capa_de(5, 10), 105), 105, 6);
  const int x = collunha_de(apagado, 3, "\u21c4");
  REQUIRE(x > 0);
  CHECK(apagado.PixelAt(x, 3).foreground_color == cor(tk::text_faint));
  qual.embaralhado = true;
  qual.repeticao = nu::Repeticao::Todas;
  const ftxui::Screen aceso =
      papel(tui::elemento_do_cabecalho(qual, capa_de(5, 10), 105), 105, 6);
  // A collunha é a MESMA: o chip apagado guarda o logar do aceso, e a linha da
  // conta não muda de largura quando o operador tecla `z`.
  CHECK(collunha_de(aceso, 3, "\u21c4") == x);
  CHECK(aceso.PixelAt(x, 3).background_color == cor(tk::v700));
  CHECK(aceso.PixelAt(x, 3).foreground_color == cor(tk::text_bright));
  CHECK(collunha_de(aceso, 3, "\u21bb") > x);
}

TEST_CASE("a arte cinge-se ao tecto que se lhe pediu") {
  // Capa mais alta que o tecto: o chafa não a devolveria assim, mas a promessa
  // do `linhas_da_arte` é o MINIMO, e quem compõe conta com ella. Sem o cinge,
  // a ficha sahiria para fóra do painel.
  const nu::CapaPintada alta = capa_de(30, 39);
  CHECK(tui::linhas_da_arte(alta, 20) == 20);
  const ftxui::Screen tela =
      papel(tui::elemento_do_painel({"Faded", "Alan Walker", "Faded"},
                                    tui::elemento_da_arte(alta, 39, 20),
                                    ftxui::emptyElement(), 39),
            39, 34);
  CHECK(linha_de(tela, 20) == std::string(39, '#'));
  CHECK(linha_de(tela, 21).substr(0, 5) == "Faded");
}

// A ALTURA EXACTA, que é o que a geometria promette a quem compõe. O elemento
// que nada pinta ha de medir ZERO: medindo um, a peça pede mais linha do que a
// conta lhe deu, e quem cinge a faixa apara a ultima em silencio.
TEST_CASE("o painel sem capa não abre fileira parasita") {
  const nu::CapaPintada nenhuma;
  const ftxui::Screen tela =
      papel(tui::elemento_do_painel({"Faded", "Alan Walker", "Faded"},
                                    tui::elemento_da_arte(nenhuma, 39, 0),
                                    ftxui::text("BAIXO"), 39),
            39, 9);
  CHECK(linha_de(tela, 0).substr(0, 13) == "TOCANDO AGORA");
  CHECK(linha_de(tela, 1).substr(0, 5) == "Faded");
  CHECK(linha_de(tela, 2).substr(0, 11) == "Alan Walker");
  CHECK(linha_de(tela, 4).substr(0, 5) == "BAIXO");
  // A FONTE do defeito, aferida á parte para que a razão fique escripta: o
  // `text` vazio do FTXUI mede UMA linha, e o `emptyElement` mede zero.
  CHECK(linha_de(papel(ftxui::vbox({ftxui::emptyElement(), ftxui::text("X")}),
                       3, 2),
                 0)[0] == 'X');
  CHECK(linha_de(papel(ftxui::vbox({ftxui::text(""), ftxui::text("X")}), 3, 2),
                 1)[0] == 'X');
}

TEST_CASE("nome comprido não empurra a barra nem o painel") {
  // O `flex_shrink_x` do FTXUI nasce ZERO: pedindo o meio mais do que ha, o
  // hbox cahe no encolhimento DURO e apara todos os irmãos por egual, ainda os
  // que pedem largura EGUAL. Medido: sem o cinge, a barra cahia de nove a oito
  // e o painel de trinta e nove a trinta e dous.
  const int curto = collunha_de(sala_de("GEOGADDI"), 0, "T");
  const int comprido = collunha_de(sala_de(std::string(120, 'N')), 0, "T");
  CHECK(curto == 117);
  CHECK(comprido == curto);
  CHECK(linha_de(sala_de(std::string(120, 'N')), 0).substr(0, 11) ==
        "BBBBBBBBB  ");
  // E o nome comprido escreve-se até onde cabe, sem invadir o painel.
  CHECK(linha_de(sala_de(std::string(120, 'N')), 1).substr(23, 4) == "NNNN");
}

TEST_CASE("sómente tres secções trazem caminho de arquivo por chave") {
  CHECK(tui::chave_e_caminho(tui::Secao::Faixas));
  CHECK(tui::chave_e_caminho(tui::Secao::Busca));
  CHECK(tui::chave_e_caminho(tui::Secao::NoRol));
  CHECK_FALSE(tui::chave_e_caminho(tui::Secao::Artistas));
  CHECK_FALSE(tui::chave_e_caminho(tui::Secao::Albuns));
  CHECK_FALSE(tui::chave_e_caminho(tui::Secao::Rede));
  CHECK_FALSE(tui::chave_e_caminho(tui::Secao::Rois));
  CHECK_FALSE(tui::chave_e_caminho(tui::Secao::Lista));
}

TEST_CASE("a ficha sem etiqueta cahe no nome do arquivo") {
  const tui::Ficha crua =
      tui::ficha_da_faixa("/acervo/Alan Walker/Faded.mp3", "", "", "");
  CHECK(crua.titulo == "Faded");
  CHECK(crua.artista.empty());
  const tui::Ficha posta = tui::ficha_da_faixa("/acervo/x.mp3", "Faded",
                                               "Alan Walker", "Different");
  CHECK(posta.titulo == "Faded");
  CHECK(posta.album == "Different");
  // Caminho vazio dá ficha vazia, que é o que diz «nada toca».
  CHECK(tui::ficha_da_faixa("", "Faded", "Alan Walker", "").titulo.empty());
}

TEST_CASE("a ficha vazia mede tres linhas e sahe apagada") {
  const ftxui::Screen tela = papel(tui::elemento_da_ficha({}, 39), 39, 5);
  CHECK(linha_de(tela, 0).substr(0, 11) == "(nada toca)");
  CHECK(tela.PixelAt(0, 0).foreground_color == cor(tk::text_faint));
  // Tres linhas, e não uma: sem ellas o espectro subiria e desceria a cada
  // troca de faixa. A quarta fica intacta.
  const ftxui::Screen painel = papel(
      tui::elemento_do_painel({}, ftxui::emptyElement(), ftxui::text("BAIXO"),
                              39),
      39, 6);
  CHECK(linha_de(painel, 4).substr(0, 5) == "BAIXO");
}

// O `l` troca o espectro pela letra, e a issue #92 promette o MESMO
// rectangulo: mesma largura, mesma altura, mesmo canto. Aqui prova-se com a
// letra CHEIA; letra curta occupa menos linhas e deixa o resto em branco, que
// é o que a peça da letra sempre fez, e não desloca cousa alguma por ser o
// ultimo filho do painel.
TEST_CASE("a letra e o espectro tomam o mesmo rectangulo do painel") {
  const tui::Ficha ficha{"Faded", "Alan Walker", "Faded"};
  const nu::CapaPintada capa = capa_de(11, 39);
  std::vector<mysong::nucleo::LinhaDaLetra> versos;
  for (int i = 0; i < 12; ++i)
    versos.push_back({static_cast<double>(i), "verso " + std::to_string(i)});
  const ftxui::Screen com_letra =
      papel(tui::elemento_do_painel(ficha, tui::elemento_da_arte(capa, 39, 20),
                                    tui::elemento_da_letra(versos, 4, 8, 39),
                                    39),
            39, 26);
  const ftxui::Screen com_bandas = papel(
      tui::elemento_do_painel(
          ficha, tui::elemento_da_arte(capa, 39, 20),
          tui::elemento_do_espectro(tui::compor(std::vector<float>(24, 1.0f),
                                                39, 8)),
          39),
      39, 26);
  const std::string vazia(39, ' ');
  for (const ftxui::Screen& qual : {std::cref(com_letra), std::cref(com_bandas)}) {
    CHECK(linha_de(qual, 14).substr(0, 5) == "Faded");  // a ficha acaba na 14
    CHECK(linha_de(qual, 15) != vazia);                 // o de baixo abre na 15
    CHECK(linha_de(qual, 22) != vazia);                 // e fecha na 22
    CHECK(linha_de(qual, 23) == vazia);                 // e não passa d'ahi
  }
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
