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
  // Em ÁLBUNS a trilha tem UM degrau só, que é o artista: o `vai_para` faz
  // `trilha_.resize(1)`. Alvo de dous degraus provaria estado que a obra não
  // produz, e prova de estado irreal não guarda cousa alguma.
  CHECK(tui::onde_da_chapa(tui::Secao::Albuns, {"Boards of Canada"}, "") ==
        "MY SONG \u25b8 Boards of Canada");
  // Em FAIXAS os dous degraus existem: o artista e o album em que se entrou.
  CHECK(tui::onde_da_chapa(tui::Secao::Faixas, fundo, "") ==
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

TEST_CASE("a chapa diz onde se está, a conta e a vista, n'uma linha só") {
  tui::Chapa qual;
  qual.onde = "MY SONG";
  qual.quantas = 42;
  qual.duracao = 5340;
  qual.vista = "FAIXAS";
  CHECK(tui::texto_da_chapa(qual) == "MY SONG, 42 FAIXAS, 1h29, FAIXAS");
  const ftxui::Screen tela = papel(tui::elemento_da_chapa(qual, 80), 80, 1);
  CHECK(linha_de(tela, 0).substr(0, 33) == " MY SONG, 42 FAIXAS, 1h29, FAIXAS");
  // Tres pesos na mesma linha (issue #111): o ONDE carrega, a conta apaga-se, e
  // a VISTA sahe em chip, que é a unica das tres que se cycla por tecla.
  CHECK(tela.PixelAt(2, 0).bold);
  CHECK(!tela.PixelAt(12, 0).bold);
  const mysong::tui::tokens::Triade chip =
      mysong::tui::tokens::rgb(mysong::tui::tokens::raised);
  CHECK(tela.PixelAt(28, 0).background_color ==
        ftxui::Color::RGB(chip.r, chip.g, chip.b));
  CHECK(tela.PixelAt(12, 0).background_color !=
        ftxui::Color::RGB(chip.r, chip.g, chip.b));
  // O recado vae Á DIREITA, empurrado pelo filler, e a linha fecha a largura.
  qual.recado = "achados na rede";
  const ftxui::Screen com = papel(tui::elemento_da_chapa(qual, 80), 80, 1);
  CHECK(linha_de(com, 0).substr(64, 16) == "achados na rede ");
  // Recado comprido CINGE-SE ao que sobra, e o texto sahe INTEIRO: medido n'um
  // pty, sem o cinge os dous juntos pediam mais do que ha, e o hbox aparava por
  // egual os dous, d'onde a chapa perdia o «S» de FAIXAS.
  // As ENCOMMENDAS vão logo á direita do texto, e não no fim: alli o primeiro
  // aviso comprido comia-lhes o logar, e a issue pede-as por cima da lista.
  qual.encommendas = "3 colhidas, 1 falhada";
  const ftxui::Screen com_baixas = papel(tui::elemento_da_chapa(qual, 80), 80, 1);
  CHECK(linha_de(com_baixas, 0).substr(33, 23) == "  3 colhidas, 1 falhada");
  // E o espaço do recado desconta-as: quem o monta ha de saber o que sobra.
  CHECK(tui::espaco_do_recado(qual, 80) == 22);
  qual.encommendas.clear();
  qual.recado = std::string(90, 'R');
  CHECK(linha_de(papel(tui::elemento_da_chapa(qual, 80), 80, 1), 0).substr(0, 33) ==
        " MY SONG, 42 FAIXAS, 1h29, FAIXAS");
  // Fóra das MY SONG a vista cala-se, e a chapa diz sómente onde e quanto.
  qual.vista.clear();
  qual.recado.clear();
  qual.onde = "PLAYLISTS \u25b8 Funk";
  qual.especie = tui::Especie::Faixas;
  qual.quantas = 1;
  qual.duracao = 0;
  CHECK(tui::texto_da_chapa(qual) == "PLAYLISTS \u25b8 Funk, 1 FAIXA");
}

TEST_CASE("a chapa da pauta vazia chama pela tecla que a enche") {
  tui::Chapa qual;
  qual.onde = "MY SONG";
  qual.quantas = 0;
  qual.vista = "FAIXAS";
  // O conselho vem da tabella, e não d'aqui: uma taboa só, e não duas.
  qual.conselho = "varra o acervo (r)";
  const ftxui::Screen tela = papel(tui::elemento_da_chapa(qual, 80), 80, 1);
  CHECK(linha_de(tela, 0).substr(0, 46) ==
        " MY SONG, 0 FAIXAS, FAIXAS  varra o acervo (r)");
  // Em glow_soft, que é o que d'esta Casa CHAMA o dedo. E o espaço do recado
  // desconta-o: quem monta o recado ha de saber o que o conselho já tomou.
  const mysong::tui::tokens::Triade chama =
      mysong::tui::tokens::rgb(mysong::tui::tokens::glow_soft);
  CHECK(tela.PixelAt(28, 0).foreground_color ==
        ftxui::Color::RGB(chama.r, chama.g, chama.b));
  CHECK(tui::espaco_do_recado(qual, 80) == 32);
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
  // O corpo abre na PRIMEIRA linha da tela (issue #125), e o pé toma as
  // quatro ultimas: o trilho, as duas da fita, e o rodapé.
  CHECK(sala.chapa.y == 0);
  CHECK(sala.pauta.y == 1);
  CHECK(sala.pauta.altura == 62);
  CHECK(sala.campo.vazio());  // sem campo aberto, linha alguma se lhe reserva
  CHECK(sala.trilho.y == 63);
  CHECK(sala.cabecalho.y == 64);
  CHECK(sala.cabecalho.altura == 2);
  CHECK(sala.rodape.y == 66);
  // O tecto da capa: quarenta e cinco por cento de sessenta e tres trunca em
  // vinte e oito, e o que sobra pertence ao espectro.
  CHECK(sala.capa.altura == 28);
  CHECK(sala.espectro.y == 28);
  CHECK(sala.espectro.altura == 35);
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
  // O PRODUCTO que a issue pede: as cinco larguras por todas as alturas, e não
  // as cinco n'uma altura e as alturas n'uma largura. São mil e cento e
  // oitenta salas por corrida, e alvo algum escripto á mão: o que se afere é
  // que as peças fecham a tela, e isso vale em toda combinação.
  for (const std::size_t larga : {100, 120, 160, 167, 200})
  for (std::size_t alta = 12; alta <= 70; ++alta) {
    for (const bool campo : {false, true}) {
      const tui::Sala sala = tui::sala_da_tela(larga, alta, campo);
      CHECK(sala.cabecalho.largura == larga);
      CHECK(sala.cabecalho.altura == 1);
      CHECK(sala.trilho.altura == 1);
      // A pauta começa onde o campo e a chapa acabam, e o rodapé é a ultima.
      const std::size_t alto = 2 + (campo ? 1u : 0u);
      CHECK(sala.chapa.y == alto);
      CHECK(sala.pauta.y == alto + 1);
      CHECK(sala.rodape.y == alta - 1);
      CHECK(sala.pauta.y + sala.pauta.altura == alta - 1);
      CHECK(sala.pauta.largura + (sala.painel.vazio() ? 0 : 1) +
                sala.painel.largura ==
            larga);
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
TEST_CASE("a arte abre o painel na primeira linha, e o de baixo segue-a") {
  for (const int alta : {11, 20}) {
    const nu::CapaPintada capa = capa_de(static_cast<std::size_t>(alta), 39);
    CHECK(tui::linhas_da_arte(capa, 20) == static_cast<std::size_t>(alta));
    const ftxui::Screen tela =
        papel(tui::elemento_do_painel(tui::elemento_da_arte(capa, 39, 20),
                                      ftxui::text("BAIXO"), 39),
              39, 34);
    CHECK(linha_de(tela, 0) == std::string(39, '#'));
    CHECK(linha_de(tela, alta - 1) == std::string(39, '#'));
    CHECK(linha_de(tela, alta).substr(0, 5) == "BAIXO");
  }
}

// O MARCADOR sahe do elemento_da_capa com orla POR FÓRA, e é por isso que se
// lhe pede quatro por trinta e sete: o que se ha de ver são seis por trinta e
// nove, sem transbordar o painel.
TEST_CASE("sem capa o marcador toma seis linhas, e o de baixo vem na setima") {
  const nu::CapaPintada nenhuma;
  CHECK(tui::linhas_da_arte(nenhuma, 20) == 6);
  const ftxui::Screen tela =
      papel(tui::elemento_do_painel(tui::elemento_da_arte(nenhuma, 39, 6),
                                    ftxui::text("BAIXO"), 39),
            39, 34);
  CHECK(collunha_de(tela, 3, "\u266b") > 0);
  CHECK(collunha_de(tela, 5, "\u2500") >= 0);
  CHECK(collunha_de(tela, 6, "\u2500") == -1);
  CHECK(linha_de(tela, 6).substr(0, 5) == "BAIXO");
}

TEST_CASE("a capa mais estreita que o painel sahe centrada n'elle") {
  // O chafa guarda a proporção: a capa quadrada n'um painel de 83 collunhas
  // com tecto de 28 linhas sahe com pouco mais de metade da largura. Sem o
  // centro ella ficava encostada á esquerda, com o vão todo de um lado só.
  const nu::CapaPintada estreita = capa_de(4, 40);
  const ftxui::Screen tela =
      papel(tui::elemento_do_painel(tui::elemento_da_arte(estreita, 40, 4),
                                    ftxui::text("BAIXO"), 80),
            80, 6);
  const std::string linha = linha_de(tela, 0);
  CHECK(linha.substr(0, 20) == std::string(20, ' '));
  CHECK(linha.substr(20, 40) == std::string(40, '#'));
  // A capa que enche a largura NÃO se desloca de uma collunha.
  const ftxui::Screen cheia =
      papel(tui::elemento_do_painel(tui::elemento_da_arte(capa_de(4, 80), 80, 4),
                                    ftxui::text("BAIXO"), 80),
            80, 6);
  CHECK(linha_de(cheia, 0) == std::string(80, '#'));
}

TEST_CASE("a arte cinge-se ao tecto que se lhe pediu") {
  // Capa mais alta que o tecto: o chafa não a devolveria assim, mas a promessa
  // do `linhas_da_arte` é o MINIMO, e quem compõe conta com ella. Sem o cinge,
  // o espectro sahiria para fóra do painel.
  const nu::CapaPintada alta = capa_de(30, 39);
  CHECK(tui::linhas_da_arte(alta, 20) == 20);
  const ftxui::Screen tela =
      papel(tui::elemento_do_painel(tui::elemento_da_arte(alta, 39, 20),
                                    ftxui::text("BAIXO"), 39),
            39, 34);
  CHECK(linha_de(tela, 19) == std::string(39, '#'));
  CHECK(linha_de(tela, 20).substr(0, 5) == "BAIXO");
}

// A ALTURA EXACTA, que é o que a geometria promette a quem compõe. O elemento
// que nada pinta ha de medir ZERO: medindo um, a peça pede mais linha do que a
// conta lhe deu, e quem cinge a faixa apara a ultima em silencio.
TEST_CASE("o painel sem capa não abre fileira parasita") {
  const nu::CapaPintada nenhuma;
  const ftxui::Screen tela =
      papel(tui::elemento_do_painel(tui::elemento_da_arte(nenhuma, 39, 0),
                                    ftxui::text("BAIXO"), 39),
            39, 9);
  CHECK(linha_de(tela, 0).substr(0, 5) == "BAIXO");
  // A FONTE do defeito, aferida á parte para que a razão fique escripta: o
  // `text` vazio do FTXUI mede UMA linha, e o `emptyElement` mede zero.
  CHECK(linha_de(papel(ftxui::vbox({ftxui::emptyElement(), ftxui::text("X")}),
                       3, 2),
                 0)[0] == 'X');
  CHECK(linha_de(papel(ftxui::vbox({ftxui::text(""), ftxui::text("X")}), 3, 2),
                 1)[0] == 'X');
}

TEST_CASE("recado comprido não empurra a pauta nem o painel") {
  // O `flex_shrink_x` do FTXUI nasce ZERO: pedindo a metade esquerda mais do
  // que ha, o hbox cahe no encolhimento DURO e apara todos os irmãos por egual,
  // ainda os que pedem largura EGUAL. Medido no cabeçalho da colleção velho:
  // sem o cinge, a barra cahia de nove a oito e o painel de trinta e nove a
  // trinta e dous. A chapa cinge-se com `size`, e é isto que o guarda.
  tui::Chapa qual;
  qual.onde = "MY SONG";
  qual.quantas = 4;
  qual.duracao = 840;
  const auto sala_de = [&](const std::string& recado) {
    tui::Chapa d_ella = qual;
    d_ella.recado = recado;
    return papel(ftxui::hbox({tui::elemento_da_chapa(d_ella, 60),
                              tui::elemento_do_divisor(1),
                              tui::elemento_do_painel(ftxui::text("P"),
                                                      ftxui::emptyElement(),
                                                      39)}),
                 100, 1);
  };
  // Afere-se o DIVISOR, e não a arte: a arte vae centrada dentro do painel, e
  // é a collunha do divisor que diz onde a metade direita começa.
  CHECK(collunha_de(sala_de(""), 0, "\u2503") == 60);
  CHECK(collunha_de(sala_de(std::string(120, 'R')), 0, "\u2503") == 60);
  CHECK(linha_de(sala_de(""), 0).substr(1, 7) == "MY SONG");
  CHECK(linha_de(sala_de(std::string(120, 'R')), 0).substr(1, 7) == "MY SONG");
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

// O `l` troca o espectro pela letra, e a issue #92 promette o MESMO rectangulo:
// mesma largura, mesma altura, mesmo canto. Aqui prova-se com a letra CHEIA;
// letra curta occupa menos linhas e deixa o resto em branco, que é o que a peça
// da letra sempre fez, e não desloca cousa alguma por ser o ultimo do painel.
TEST_CASE("a letra e o espectro tomam o mesmo rectangulo do painel") {
  const nu::CapaPintada capa = capa_de(11, 39);
  std::vector<mysong::nucleo::LinhaDaLetra> versos;
  for (int i = 0; i < 12; ++i)
    versos.push_back({static_cast<double>(i), "verso " + std::to_string(i)});
  const ftxui::Screen com_letra =
      papel(tui::elemento_do_painel(tui::elemento_da_arte(capa, 39, 20),
                                    tui::elemento_da_letra(versos, 4, 8, 39),
                                    39),
            39, 26);
  const ftxui::Screen com_bandas = papel(
      tui::elemento_do_painel(
          tui::elemento_da_arte(capa, 39, 20),
          tui::elemento_do_espectro(
              tui::compor(std::vector<float>(24, 1.0f), 39, 8)),
          39),
      39, 26);
  const std::string vazia(39, ' ');
  for (const ftxui::Screen& qual :
       {std::cref(com_letra), std::cref(com_bandas)}) {
    CHECK(linha_de(qual, 10) == std::string(39, '#'));  // a capa acaba na 10
    CHECK(linha_de(qual, 11) != vazia);                 // o de baixo abre na 11
    CHECK(linha_de(qual, 18) != vazia);                 // e fecha na 18
    CHECK(linha_de(qual, 19) == vazia);                 // e não passa d'ahi
  }
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
