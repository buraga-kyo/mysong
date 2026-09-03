// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DO DESENHO — testes/prova_espectro_tui.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a fita de barras verticaes da §7.4.9. Caso algum abre terminal: o
// desenho resolve-se em QUADRO, e é pelo quadro que se prova.
//
// Chama-se _tui porque testes/prova_espectro.cpp já existe e é a prova do
// NÚCLEO, da issue #5: aquella prova que as bandas se COLHEM, esta que ellas se
// DESENHAM, e dous arquivos de egual nome no mesmo directorio não cabem.
//
// A LIÇÃO D'ESTA OBRA, que aqui se obedece: n'esta bateria já se apanharam NOVE
// casos que perguntavam ao oraculo sob exame o que devião estar affirmando de
// fóra, e um caso que NUNCA CORRIA. D'onde duas regras, e nenhuma se dispensa:
//   (a) o alvo esperado vem escripto Á MÃO, ou recalculado dentro do caso por
//       conta INDEPENDENTE. Jamais se compara a sahida de compor() com outra
//       sahida de compor(), salvo quando a egualdade das duas É o que se afirma.
//   (b) nome de caso sem ponto e vírgula, JAMAIS: o ponto e vírgula parte a
//       lista do CMake, e zero casos corridos dá status de successo.
//
// DOMÍNIO ......... bandas armadas aqui mesmo, e larguras e alturas escolhidas
//                   no proprio caso. Nada se colhe e nada se sonda.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... o que se afirma é a REGRA do systema de desenho, e não a
//                   apparencia: os degraus, a orientação, a ancoragem do
//                   gradiente, a precedencia da côr e o ladrilho.
// Q.E.D. .......... provada a orientação por INDICE de linha, a fita de cabeça
//                   para baixo deixa de passar calada. O que o olho ainda deve
//                   julgar vae por extenso na SPEC, e caso algum o allega.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cctype>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/screen/screen.hpp>

#include "nucleo/analisador.hpp"
#include "nucleo/espectro.hpp"
#include "tui/espectro.hpp"
#include "tui/tokens.hpp"

namespace es = mysong::tui;
namespace tk = mysong::tui::tokens;

// ── C1 · o mapeamento de magnitude para degrau ──────────────────────────────
// A taboada vae escripta Á MÃO, e não gerada por laço que repetisse a conta sob
// exame. Os k/8 escrevem-se como fracções, e não como decimaes arredondados:
// oito é potencia de dous, d'onde k/8 é exacto em binario e o floor não tem
// arredondamento a perdoar. Escripto 0,375 em vez de 3.0f/8.0f, o caso passaria
// a affirmar a redacção do decimal em vez do degrau.
TEST_CASE("o degrau vae de zero a cheio, e o lixo não passa") {
  // Painel de tres célullas: o teto é 3 vezes 8, que são 24 degraus.
  CHECK(es::oitavos(0.0f, 3) == 0);
  CHECK(es::oitavos(1.0f, 3) == 24);
  CHECK(es::oitavos(0.5f, 3) == 12);

  // Acima do teto CINGE-SE, e não estoura nem transborda o quadro.
  CHECK(es::oitavos(1.5f, 3) == 24);
  CHECK(es::oitavos(1000.0f, 3) == 24);

  // Abaixo do piso cinge-se a zero, que é o caso da banda negativa por erro.
  CHECK(es::oitavos(-0.2f, 3) == 0);
  CHECK(es::oitavos(-1000.0f, 3) == 0);

  // O NÃO FINITO vale zero. É o caso que a ordem das guardas de cingido()
  // sustenta: fosse a finitude testada DEPOIS do cingir, o NaN chegaria ao floor.
  const float nan = std::numeric_limits<float>::quiet_NaN();
  const float inf = std::numeric_limits<float>::infinity();
  CHECK(es::oitavos(nan, 3) == 0);
  CHECK(es::oitavos(inf, 3) == 0);
  CHECK(es::oitavos(-inf, 3) == 0);

  // Painel de UMA célulla: os oito degraus todos se alcançam, um por um.
  CHECK(es::oitavos(0.0f / 8.0f, 1) == 0);
  CHECK(es::oitavos(1.0f / 8.0f, 1) == 1);
  CHECK(es::oitavos(2.0f / 8.0f, 1) == 2);
  CHECK(es::oitavos(3.0f / 8.0f, 1) == 3);
  CHECK(es::oitavos(4.0f / 8.0f, 1) == 4);
  CHECK(es::oitavos(5.0f / 8.0f, 1) == 5);
  CHECK(es::oitavos(6.0f / 8.0f, 1) == 6);
  CHECK(es::oitavos(7.0f / 8.0f, 1) == 7);
  CHECK(es::oitavos(8.0f / 8.0f, 1) == 8);

  // Painel sem altura não tem degrau algum, e não divide por cousa nenhuma.
  CHECK(es::oitavos(1.0f, 0) == 0);
}

namespace {

// bandas_uniformes — as QUANTAS_BANDAS todas no mesmo valor.
std::vector<float> bandas_uniformes(float valor) {
  return std::vector<float>(mysong::nucleo::QUANTAS_BANDAS, valor);
}

// centros_em — as QUANTAS_BANDAS todas no mesmo hertz, para PRENDER o registro
// da fita a um só. Os casos da rampa afirmam a RAMPA, e não a fronteira, que tem
// caso proprio: sem isto, cada columna cahiria n'uma familia differente.
std::vector<float> centros_em(float hertz) {
  return std::vector<float>(mysong::nucleo::QUANTAS_BANDAS, hertz);
}

// Os glifos ESCRIPTOS Á MÃO, por ponto de codigo. Escrevem-se por \u e não pelo
// glifo cru para que a prova não dependa da codificação com que o editor gravou
// este arquivo, e para que o assento de cada degrau se leia como numero.
constexpr const char* kCheio = "\u2588";  // oito oitavos, o bloco cheio
constexpr const char* kCinco = "\u2585";  // cinco oitavos
constexpr const char* kUm = "\u2581";     // um oitavo, o piso do silencio
constexpr const char* kVazio = " ";

// cor — a tríade em côr do FTXUI, que é como a tela a guarda. Recebe TRÍADE e
// não token porque os degraus do meio da rampa nascem de tokens::mistura, e
// nome na paleta não têm.
ftxui::Color cor(tk::Triade c) { return ftxui::Color::RGB(c.r, c.g, c.b); }

}  // namespace

// ── C2 · os blocos empilhados, e o parcial NO TOPO ──────────────────────────
TEST_CASE("a columna empilha o cheio e põe o degrau parcial acima") {
  // Painel de QUATRO célullas, d'onde o teto são 32 degraus. Escolhe-se 21/32,
  // que dá 21 degraus: DOUS cheios (16) e resto CINCO. A conta faz-se aqui, á
  // mão, e não se pergunta a oitavos() qual seria.
  const std::vector<float> bandas = bandas_uniformes(21.0f / 32.0f);
  const es::Quadro quadro = es::compor(bandas, mysong::nucleo::QUANTAS_BANDAS, 4);

  REQUIRE(quadro.altura == 4);
  for (std::size_t c = 0; c < quadro.largura; ++c) {
    // A BASE é a linha 3, e é lá que mora o primeiro cheio.
    CHECK(quadro.em(3, c).glifo == kCheio);
    CHECK(quadro.em(2, c).glifo == kCheio);
    // O PARCIAL de cinco oitavos vem ACIMA dos cheios, na linha 1.
    CHECK(quadro.em(1, c).glifo == kCinco);
    // E acima d'elle nada se pinta.
    CHECK(quadro.em(0, c).pinta == false);
    CHECK(quadro.em(0, c).glifo == kVazio);
  }
}

// ── C3 · o gradiente ancorado no PAINEL ─────────────────────────────────────
// A magnitude escolhida é 0,899: abaixo do limiar quente (0,90), de sorte que o
// gradiente vale, e alta bastante para pintar as CINCO linhas do painel. A conta
// á mão: teto 5 vezes 8 = 40 degraus, 0,899 vezes 40 = 35,96, floor 35, que dá
// quatro blocos cheios (32) e resto tres. Cinco célullas desenhadas, e é o que
// permitte aferir a rampa INTEIRA, da base ao topo, n'uma composição só.
TEST_CASE("a rampa vae da base composta ao topo do registro") {
  const std::vector<float> bandas = bandas_uniformes(0.899f);
  // Cem hertz prende a fita inteira nos GRAVES, e o caso afere a RAMPA.
  const es::Quadro quadro = es::compor(bandas, mysong::nucleo::QUANTAS_BANDAS, 5,
                                       false, centros_em(100.0f));

  REQUIRE(quadro.altura == 5);
  for (std::size_t c = 0; c < quadro.largura; ++c) {
    REQUIRE(quadro.em(4, c).pinta);
    REQUIRE(quadro.em(0, c).pinta);

    // A BASE (linha 4) veste o v500 composto sobre o painel com peso 0,55,
    // conferido contra os TOKENS e não contra outra chamada da obra.
    CHECK(es::mesma_tinta(quadro.em(4, c).tinta,
                          tk::mistura(tk::v500, tk::panel_hi, 0.55)));
    // O TOPO (linha 0) veste v500 EXACTO, pelo mesmo modo.
    CHECK(es::mesma_tinta(quadro.em(0, c).tinta, tk::rgb(tk::v500)));

    // O MEIO (linha 2) fica a meia rampa. O peso recalcula-se AQUI: a linha 2 é
    // a terceira desde a base d'um painel de cinco, d'onde o alto vale 2 e a
    // fracção 2/4, que é 0,5; e o peso vae de 0,55 a 1, d'onde 0,775. Compõe-se
    // por tokens::mistura, e não pela funcção sob exame.
    CHECK(es::mesma_tinta(quadro.em(2, c).tinta,
                          tk::mistura(tk::v500, tk::panel_hi, 0.775)));
  }
}

TEST_CASE("painel de uma célulla veste a base da rampa") {
  const es::Quadro quadro =
      es::compor(bandas_uniformes(0.5f), 8, 1, false, centros_em(100.0f));
  REQUIRE(quadro.altura == 1);
  for (std::size_t c = 0; c < quadro.largura; ++c) {
    REQUIRE(quadro.em(0, c).pinta);
    CHECK(es::mesma_tinta(quadro.em(0, c).tinta,
                          tk::mistura(tk::v500, tk::panel_hi, 0.55)));
  }
}

// A NEGATIVA, que é o coração do C3 e o que mais escapa a teste: mudando-se a
// magnitude e conservando-se a LINHA, a tinta NÃO muda. É isto que estabelece
// que o gradiente é do continente e não do conteudo. Ancorado na barra, a linha
// 2 de uma barra de tres célullas seria o TOPO d'ella e sahiria em v400, ao
// passo que a mesma linha 2 de uma barra de cinco sahiria a meia rampa, e as
// duas asserções abaixo divergirião.
//
// As magnitudes: 0,899 pinta cinco célullas (a conta está no caso acima), e 0,5
// pinta tres, a saber teto 40, 0,5 vezes 40 = 20 degraus, dous blocos cheios e
// resto quatro. Ambas alcançam a linha 2, e é o que as torna comparaveis.
TEST_CASE("a tinta da linha não muda quando a magnitude muda") {
  const std::size_t largura = mysong::nucleo::QUANTAS_BANDAS;
  const std::vector<float> centros = centros_em(100.0f);
  const es::Quadro alta =
      es::compor(bandas_uniformes(0.899f), largura, 5, false, centros);
  const es::Quadro baixa =
      es::compor(bandas_uniformes(0.5f), largura, 5, false, centros);

  for (std::size_t c = 0; c < largura; ++c) {
    // As duas pintam a linha 2 e a linha 4, e é premissa do caso.
    REQUIRE(alta.em(2, c).pinta);
    REQUIRE(baixa.em(2, c).pinta);
    REQUIRE(baixa.em(4, c).pinta);

    // A linha 2 veste a MESMA tinta nas duas, e a linha 4 tambem. Confere-se
    // contra o alvo escripto de fóra, e ainda uma contra a outra.
    CHECK(es::mesma_tinta(baixa.em(2, c).tinta,
                          tk::mistura(tk::v500, tk::panel_hi, 0.775)));
    CHECK(es::mesma_tinta(alta.em(2, c).tinta, baixa.em(2, c).tinta));
    CHECK(es::mesma_tinta(baixa.em(4, c).tinta,
                          tk::mistura(tk::v500, tk::panel_hi, 0.55)));
    CHECK(es::mesma_tinta(alta.em(4, c).tinta, baixa.em(4, c).tinta));

    // E a barra BAIXA de facto pára antes do topo: não é que ella seja egual
    // por ser egualmente alta. Sem esta linha o caso passaria por coincidencia.
    CHECK(baixa.em(1, c).pinta == false);
    CHECK(alta.em(1, c).pinta == true);
  }
}

// ── C4 · o quente, por COLUMNA e não por fita ───────────────────────────────
// A vizinha fria entra na MESMA composição, de propósito: é o que distingue «o
// quente é d'esta columna» de «o quente é da fita toda». Sem ella, uma obra que
// pintasse tudo de glow_hot ao ver um pico passaria o caso.
TEST_CASE("a columna quente veste glow_hot inteira, e a vizinha fria não") {
  std::vector<float> bandas(mysong::nucleo::QUANTAS_BANDAS, 0.0f);
  bandas[3] = 0.95f;  // acima do limiar: quente
  bandas[4] = 0.40f;  // abaixo: fria, e no gradiente
  const es::Quadro quadro = es::compor(bandas, mysong::nucleo::QUANTAS_BANDAS, 5,
                                       false, centros_em(100.0f));

  // A columna 3: teto 40, 0,95 vezes 40 = 38 degraus, quatro cheios e resto 6,
  // d'onde cinco célullas, todas em glow_hot, da base ao topo.
  for (std::size_t l = 0; l < 5; ++l) {
    REQUIRE(quadro.em(l, 3).pinta);
    CHECK(es::mesma_tinta(quadro.em(l, 3).tinta, tk::rgb(tk::glow_hot)));
  }

  // A columna 4: 0,40 vezes 40 = 16 degraus, dous cheios e resto zero, d'onde
  // duas célullas, e no GRADIENTE. A base na côr do registro composta sobre o
  // painel, e não em glow_hot.
  REQUIRE(quadro.em(4, 4).pinta);
  CHECK(es::mesma_tinta(quadro.em(4, 4).tinta,
                        tk::mistura(tk::v500, tk::panel_hi, 0.55)));
  CHECK_FALSE(es::mesma_tinta(quadro.em(4, 4).tinta, tk::rgb(tk::glow_hot)));
  CHECK(quadro.em(2, 4).pinta == false);
}

// O limiar pertence ao quente: afere-se nos DOUS lados d'elle, que é onde o
// maior-ou-igual se distingue do maior.
TEST_CASE("o limiar de noventa por cento pertence ao quente") {
  const std::size_t largura = mysong::nucleo::QUANTAS_BANDAS;
  const std::vector<float> centros = centros_em(100.0f);
  const es::Quadro no_limiar =
      es::compor(bandas_uniformes(0.90f), largura, 4, false, centros);
  const es::Quadro sob_limiar =
      es::compor(bandas_uniformes(0.899f), largura, 4, false, centros);

  REQUIRE(no_limiar.em(3, 0).pinta);
  REQUIRE(sob_limiar.em(3, 0).pinta);
  // Em cima do limiar: quente.
  CHECK(es::mesma_tinta(no_limiar.em(3, 0).tinta, tk::rgb(tk::glow_hot)));
  // Um milesimo abaixo: frio, e de volta á base da rampa.
  CHECK(es::mesma_tinta(sob_limiar.em(3, 0).tinta,
                        tk::mistura(tk::v500, tk::panel_hi, 0.55)));
}

// ── C5 · o mudo, e o piso do silencio ───────────────────────────────────────
// O mudo põe-se com banda ACIMA do limiar quente, de propósito: é o que afere a
// precedencia. Ordem do operador não se deixa sobrepujar por leitura de sinal, e
// um caso com banda fria não distinguiria as duas regras.
TEST_CASE("o mudo veste text_faint e vence o quente") {
  const std::size_t largura = mysong::nucleo::QUANTAS_BANDAS;
  const es::Quadro quadro = es::compor(bandas_uniformes(0.95f), largura, 5, true);

  std::size_t pintadas = 0;
  for (std::size_t l = 0; l < quadro.altura; ++l)
    for (std::size_t c = 0; c < quadro.largura; ++c)
      if (quadro.em(l, c).pinta) {
        ++pintadas;
        CHECK(es::mesma_tinta(quadro.em(l, c).tinta, tk::rgb(tk::text_faint)));
        CHECK_FALSE(es::mesma_tinta(quadro.em(l, c).tinta, tk::rgb(tk::glow_hot)));
      }
  // As ALTURAS conservam-se: o mudo esmaece, e não derruba a fita. Cinco linhas
  // por columna, pela conta do caso do quente (0,95 pinta as cinco).
  CHECK(pintadas == 5 * largura);
}

// O piso: as barras cahem a zero e FICAM VISIVEIS, que é o que faz o aceite
// dizer algo. Barra de zero célullas não teria côr, e a promessa sahiria invacua.
TEST_CASE("o silencio deixa um piso de um oitavo em text_faint") {
  const std::size_t largura = 12;
  const es::Quadro quadro = es::compor(bandas_uniformes(0.0f), largura, 6);

  for (std::size_t c = 0; c < largura; ++c) {
    // UMA célulla, na BASE (linha 5), de um oitavo, em text_faint.
    REQUIRE(quadro.em(5, c).pinta);
    CHECK(quadro.em(5, c).glifo == kUm);
    CHECK(es::mesma_tinta(quadro.em(5, c).tinta, tk::rgb(tk::text_faint)));
    // E nada acima d'ella, em linha alguma.
    for (std::size_t l = 0; l < 5; ++l) CHECK(quadro.em(l, c).pinta == false);
  }
}

// ── C11 · os quatro registros, e a fronteira em HERTZ ───────────────────────
// A taboada vae escripta Á MÃO, e nos DOUS lados de cada fronteira: é ahi que o
// maior-ou-egual se distingue do maior, e é o unico logar onde um erro de um
// hertz se apanha, que no meio da faixa toda obra acerta.
TEST_CASE("a fronteira do registro decide-se pelo centro em hertz") {
  CHECK(es::registro_da_banda(40.0f) == es::Registro::Graves);
  CHECK(es::registro_da_banda(249.0f) == es::Registro::Graves);
  CHECK(es::registro_da_banda(250.0f) == es::Registro::Graves);
  CHECK(es::registro_da_banda(251.0f) == es::Registro::MediosGraves);
  CHECK(es::registro_da_banda(999.0f) == es::Registro::MediosGraves);
  CHECK(es::registro_da_banda(1000.0f) == es::Registro::MediosGraves);
  CHECK(es::registro_da_banda(1001.0f) == es::Registro::MediosAgudos);
  CHECK(es::registro_da_banda(3999.0f) == es::Registro::MediosAgudos);
  CHECK(es::registro_da_banda(4000.0f) == es::Registro::MediosAgudos);
  CHECK(es::registro_da_banda(4001.0f) == es::Registro::Agudos);
  CHECK(es::registro_da_banda(16000.0f) == es::Registro::Agudos);

  // O lixo, e é a mesma lição do cingido: toda comparação com NaN é falsa,
  // d'onde elle atravessaria os tres ramos e sahiria AGUDOS, que é a familia
  // que ninguem pediu. Sahe GRAVES, que é o principio da escala.
  CHECK(es::registro_da_banda(std::numeric_limits<float>::quiet_NaN()) ==
        es::Registro::Graves);
  CHECK(es::registro_da_banda(-1.0f) == es::Registro::Graves);
}

// ── C12 · as quatro côres da BATIDA, e a rampa que não as usa ───────────────
// Um caso por familia, com o centro posto no MEIO da faixa d'ella e não junto da
// fronteira: aqui afere-se a CÔR, e a fronteira tem caso proprio.
TEST_CASE("cada registro tem a sua côr de batida, e a rampa é a mesma") {
  struct Caso {
    float hertz;
    std::string_view batida;
  };
  // Os hertz e os tokens escriptos Á MÃO, que é a taboada da issue #132: rosa
  // glow_hot nos graves, cyan data5 nos medios-graves, laranja data3 nos
  // medios-agudos e amarello data2 nos agudos.
  const Caso casos[] = {{100.0f, tk::glow_hot},
                        {500.0f, tk::data5},
                        {2000.0f, tk::data3},
                        {8000.0f, tk::data2}};
  for (const Caso& caso : casos) {
    CHECK(es::tinta_do_registro(es::registro_da_banda(caso.hertz)) ==
          caso.batida);
    // 0,899 pinta as CINCO célullas d'um painel de cinco, pela conta do caso da
    // rampa: teto 40, floor de 35,96 dá 35, quatro cheios e resto tres. E a
    // banda FRIA veste a rampa, que é v500 em toda familia: a côr da batida não
    // lhe toca, e é justamente o que a issue #132 restituiu.
    const es::Quadro quadro =
        es::compor(bandas_uniformes(0.899f), 6, 5, false, centros_em(caso.hertz));
    for (std::size_t c = 0; c < quadro.largura; ++c) {
      REQUIRE(quadro.em(0, c).pinta);
      CHECK(es::mesma_tinta(quadro.em(0, c).tinta, tk::rgb(tk::v500)));
      CHECK(es::mesma_tinta(quadro.em(4, c).tinta,
                            tk::mistura(tk::v500, tk::panel_hi, 0.55)));
    }
  }
}

// A FRONTEIRA dentro do QUADRO, e não sómente no punho puro: duas bandas, uma de
// centro em 249 hertz e a outra em 251, e a fita mostra as duas familias lado a
// lado. É o caso que apanha um quadro que resolvesse o registro pela POSIÇÃO da
// columna em vez do centro em hertz da banda que ella cobre.
TEST_CASE("banda de 249 hertz sahe grave e a de 251 sahe media-grave") {
  const std::vector<float> bandas = {0.5f, 0.5f};
  const std::vector<float> centros = {249.0f, 251.0f};
  const es::Quadro quadro = es::compor(bandas, 2, 4, false, centros);

  REQUIRE(quadro.registros.size() == 2);
  CHECK(quadro.registros[0] == es::Registro::Graves);
  CHECK(quadro.registros[1] == es::Registro::MediosGraves);

  // E a RAMPA não segue o registro: teto 32, 0,5 vezes 32 dá 16 degraus, dous
  // blocos cheios e resto zero, d'onde duas célullas, e a base é a linha 3. As
  // duas familias vestem a MESMA base composta, que é o que a issue #132 quer;
  // o que as aparta é a côr da BATIDA, e essa só apparece na batida.
  REQUIRE(quadro.em(3, 0).pinta);
  REQUIRE(quadro.em(3, 1).pinta);
  CHECK(es::mesma_tinta(quadro.em(3, 0).tinta,
                        tk::mistura(tk::v500, tk::panel_hi, 0.55)));
  CHECK(es::mesma_tinta(quadro.em(3, 1).tinta,
                        tk::mistura(tk::v500, tk::panel_hi, 0.55)));
  CHECK(es::tinta_do_registro(quadro.registros[0]) == tk::glow_hot);
  CHECK(es::tinta_do_registro(quadro.registros[1]) == tk::data5);
}

// A PRECEDENCIA da côr não se mexe com o registro. Arma-se nos AGUDOS de
// propósito: a batida d'elles é amarella, e uma obra que esquecesse o ramo do
// quente e cahisse na rampa denuncia-se pelo violeta, ao passo que armada nos
// graves a mesma falha daria côr parecida de mais com o rosa da batida.
TEST_CASE("o pico veste a côr do registro, e o mudo text_faint por cima") {
  const std::vector<float> centros = centros_em(8000.0f);  // agudos, o amarello
  const es::Quadro quente =
      es::compor(bandas_uniformes(0.95f), 6, 4, false, centros);
  const es::Quadro calado =
      es::compor(bandas_uniformes(0.95f), 6, 4, true, centros);
  const es::Quadro silencio =
      es::compor(bandas_uniformes(0.0f), 6, 4, false, centros);

  for (std::size_t c = 0; c < 6; ++c) {
    // A base (linha 3) em todos os tres, que é a célulla que toda barra tem.
    REQUIRE(quente.em(3, c).pinta);
    CHECK(es::mesma_tinta(quente.em(3, c).tinta, tk::rgb(tk::data2)));
    CHECK_FALSE(es::mesma_tinta(quente.em(3, c).tinta, tk::rgb(tk::glow_hot)));
    CHECK(es::mesma_tinta(calado.em(3, c).tinta, tk::rgb(tk::text_faint)));
    CHECK(es::mesma_tinta(silencio.em(3, c).tinta, tk::rgb(tk::text_faint)));
  }
}

// ── C13 · as bordas REAES do nucleo ─────────────────────────────────────────
// A obra viva não alcança as bordas do nucleo::Espectro, que o punho do
// analisador as não abre, e vae pela escala NOMINAL do contracto. Este caso
// afere que as duas põem TODA banda no mesmo registro, nas taxas que esta Casa
// encontra, e é o que fecha a differença entre o que a tela pinta e o que o
// nucleo colheu. Em 96 kHz ellas divergem em tres bandas do grave, que a
// quantização em raias empurra as bordas para cima; taxa que o mundo não dá a
// um tocador de mesa, e por isso se nomeia aqui em vez de se affirmar.
TEST_CASE("as bordas reaes do nucleo põem toda banda no mesmo registro") {
  const std::vector<float> nominais =
      es::centros_da_escala(mysong::nucleo::QUANTAS_BANDAS);
  for (const float taxa : {44100.0f, 48000.0f}) {
    mysong::nucleo::Espectro espectro(taxa, 2);
    const std::vector<float> reaes = es::centros_das_bandas(
        espectro.bordas(),
        taxa / static_cast<float>(mysong::nucleo::JANELA_DA_FFT));
    REQUIRE(reaes.size() == mysong::nucleo::QUANTAS_BANDAS);
    for (std::size_t b = 0; b < reaes.size(); ++b)
      CHECK(es::registro_da_banda(reaes[b]) ==
            es::registro_da_banda(nominais[b]));
  }
}

// A FITA com as bordas REAES, que é o aceite da issue por extenso: vinte e
// quatro columnas, uma por banda, e as quatro familias em BLOCOS na ordem, da
// esquerda para a direita. Os limites vão escriptos Á MÃO, colhidos das bordas
// que o nucleo assenta em 48 kHz: a banda 6 tem centro em 210 hertz e a 7 em
// 267, a 12 em 907 e a 13 em 1163, a 17 em 3163 e a 18 em 4059.
TEST_CASE("com as bordas reaes a fita reparte-se nas quatro familias") {
  mysong::nucleo::Espectro espectro(48000.0f, 2);
  const std::vector<float> centros = es::centros_das_bandas(
      espectro.bordas(),
      48000.0f / static_cast<float>(mysong::nucleo::JANELA_DA_FFT));
  const es::Quadro quadro = es::compor(
      bandas_uniformes(0.5f), mysong::nucleo::QUANTAS_BANDAS, 4, false, centros);

  struct Faixa {
    std::size_t ultima;
    es::Registro registro;
    std::string_view batida;
  };
  const Faixa faixas[] = {{6, es::Registro::Graves, tk::glow_hot},
                          {12, es::Registro::MediosGraves, tk::data5},
                          {17, es::Registro::MediosAgudos, tk::data3},
                          {23, es::Registro::Agudos, tk::data2}};
  REQUIRE(quadro.registros.size() == mysong::nucleo::QUANTAS_BANDAS);
  std::size_t b = 0;
  for (const Faixa& faixa : faixas)
    for (; b <= faixa.ultima; ++b) {
      CHECK(quadro.registros[b] == faixa.registro);
      // A base veste a RAMPA, egual em toda familia; o que muda de familia para
      // familia é a côr da batida, que aqui não se accende (0,5 é frio).
      CHECK(es::mesma_tinta(quadro.em(3, b).tinta,
                            tk::mistura(tk::v500, tk::panel_hi, 0.55)));
      CHECK(es::tinta_do_registro(faixa.registro) == faixa.batida);
    }
  CHECK(b == mysong::nucleo::QUANTAS_BANDAS);  // a taboada cobre a fita inteira
}

// ── C14 · o pico recente de cada banda ─────────────────────────────────────
// A conta do decaimento vae escripta Á MÃO, e não pela mesma fórmula da obra:
// escripta por std::pow com o mesmo expoente, o caso affirmaria a redacção em
// vez do decaimento. Metade n'uma meia-vida, um quarto em duas.
TEST_CASE("o pico sobe de immediato ao valor da banda") {
  std::vector<float> picos;
  es::avanca_picos(picos, {0.2f, 0.8f}, 0.0);
  REQUIRE(picos.size() == 2);
  CHECK(picos[0] == doctest::Approx(0.2f));
  CHECK(picos[1] == doctest::Approx(0.8f));

  // E a banda que CAE não leva o pico com ella: elle fica, e é contra elle que
  // a batida seguinte se ha de medir. Sem isto não haveria pico algum, sómente
  // o valor corrente por outro nome.
  es::avanca_picos(picos, {0.0f, 0.0f}, 0.0);
  CHECK(picos[1] == doctest::Approx(0.8f));
}

TEST_CASE("o pico cae á metade na meia-vida") {
  std::vector<float> picos;
  es::avanca_picos(picos, {1.0f}, 0.0);
  REQUIRE(picos.size() == 1);

  es::avanca_picos(picos, {0.0f}, es::MEIA_VIDA_DO_PICO_S);
  CHECK(picos[0] == doctest::Approx(0.5f));
  // Outra meia-vida dá um QUARTO, e não zero: o decaimento é continuo, e não
  // um degrau que se desse por inteiro ao fim do prazo.
  es::avanca_picos(picos, {0.0f}, es::MEIA_VIDA_DO_PICO_S);
  CHECK(picos[0] == doctest::Approx(0.25f));
  // E meia meia-vida não dá tres oitavos: dá um quarto vezes a raiz de meio,
  // que é o que aparta o decaimento continuo do linear.
  es::avanca_picos(picos, {0.0f}, es::MEIA_VIDA_DO_PICO_S / 2.0);
  CHECK(picos[0] == doctest::Approx(0.25f * std::sqrt(0.5f)));
}

TEST_CASE("o pico redimensiona-se e ZERA quando as bandas mudão de numero") {
  std::vector<float> picos = {0.9f, 0.9f, 0.9f};
  es::avanca_picos(picos, {0.1f, 0.1f}, 0.0);
  REQUIRE(picos.size() == 2);
  // Zera, e não conserva os 0,9: aquelles erão de outra colheita, e pico de
  // banda que já não existe accenderia a batida na columna errada.
  CHECK(picos[0] == doctest::Approx(0.1f));
  CHECK(picos[1] == doctest::Approx(0.1f));
}

// O tempo LIXO, e é a mesma lição do cingido: relogio que recua, quadro que se
// repete e conta indefinida valem todos a passagem NULLA. Derrubassem o pico,
// a batida apagava-se por causa do relogio e não por causa da musica.
TEST_CASE("tempo negativo ou não finito não derruba pico algum") {
  const double nan = std::numeric_limits<double>::quiet_NaN();
  const double inf = std::numeric_limits<double>::infinity();
  for (const double segundos : {-1.0, -1000.0, nan, inf, -inf}) {
    std::vector<float> picos;
    es::avanca_picos(picos, {1.0f}, 0.0);
    es::avanca_picos(picos, {0.0f}, segundos);
    REQUIRE(picos.size() == 1);
    CHECK(picos[0] == doctest::Approx(1.0f));
  }
}

// ── C15 · a batida RELATIVA ao pico da columna ─────────────────────────────
// Os tres pontos que a issue #132 nomeia, e cada um aparta uma regra. Arma-se
// nos AGUDOS porque é lá que o tecto absoluto falhava: elle raro chega a 0,9 da
// fita, e a batida d'elle passaria despercebida por não ser grande, sómente
// grande PARA ELLE. Painel de tres: teto 24, e 0,6 dá 14 degraus, um cheio e
// resto seis, d'onde duas célullas e a base na linha 2.
TEST_CASE("a batida mede-se contra o pico da columna, com piso de meio") {
  const std::vector<float> centros = centros_em(8000.0f);  // agudos, o amarello
  const tk::Triade rampa = tk::mistura(tk::v500, tk::panel_hi, 0.55);

  // Seis decimos com o pico em seis decimos: a banda está no proprio topo e
  // passa do piso. ACCENDE, e o tecto absoluto nunca lhe daria isto.
  const es::Quadro bate = es::compor(bandas_uniformes(0.6f), 4, 3, false,
                                     centros, bandas_uniformes(0.6f));
  REQUIRE(bate.em(2, 0).pinta);
  CHECK(es::mesma_tinta(bate.em(2, 0).tinta, tk::rgb(tk::data2)));

  // Os mesmos seis decimos com o pico em um: 0,6 não chega a 0,9 vezes 1, e a
  // columna fica FRIA. É a banda que já deu mais e ainda não tornou a dar.
  const es::Quadro cede = es::compor(bandas_uniformes(0.6f), 4, 3, false,
                                     centros, bandas_uniformes(1.0f));
  CHECK(es::mesma_tinta(cede.em(2, 0).tinta, rampa));

  // Quatro decimos com o pico em quatro decimos: no topo de si, mas debaixo do
  // PISO. Sem elle, a passagem baixa e o silencio piscarião sem parar.
  const es::Quadro baixa = es::compor(bandas_uniformes(0.4f), 4, 3, false,
                                      centros, bandas_uniformes(0.4f));
  CHECK(es::mesma_tinta(baixa.em(2, 0).tinta, rampa));
}

// Sem picos vale o TECTO, e é o que conserva verdadeiro quanto se affirmou
// antes da issue #132: chamador que não guarda estado ha de ver o que sempre viu.
TEST_CASE("sem picos a batida continua a medir-se pelo tecto absoluto") {
  const std::vector<float> centros = centros_em(8000.0f);
  // Seis decimos sem picos: FRIO, ainda que a banda esteja no proprio topo.
  const es::Quadro sem = es::compor(bandas_uniformes(0.6f), 4, 3, false, centros);
  CHECK(es::mesma_tinta(sem.em(2, 0).tinta,
                        tk::mistura(tk::v500, tk::panel_hi, 0.55)));
  // E o tecto continua a accender, sem pico algum a governá-lo.
  const es::Quadro tecto =
      es::compor(bandas_uniformes(0.95f), 4, 3, false, centros);
  CHECK(es::mesma_tinta(tecto.em(2, 0).tinta, tk::rgb(tk::data2)));
}

// O MUDO vence a batida, como vencia o quente: ordem do operador não se deixa
// sobrepujar por leitura de sinal, e agora ha uma côr de familia a vencer.
TEST_CASE("o mudo vence a côr da batida do registro") {
  const std::vector<float> centros = centros_em(2000.0f);  // medios-agudos
  const es::Quadro quadro = es::compor(bandas_uniformes(0.6f), 4, 3, true,
                                       centros, bandas_uniformes(0.6f));
  REQUIRE(quadro.em(2, 0).pinta);
  CHECK(es::mesma_tinta(quadro.em(2, 0).tinta, tk::rgb(tk::text_faint)));
  CHECK_FALSE(es::mesma_tinta(quadro.em(2, 0).tinta, tk::rgb(tk::data3)));
}

// ── C16 · as quatro côres da batida na TELA ────────────────────────────────
// Oito columnas, uma banda por columna, e as familias aos pares: a de indice
// par bate, a impar ao lado d'ella fica fria. Assim afere-se n'uma composição só
// que a côr é da COLUMNA e da familia d'ella, e não da fita. Lê-se por PixelAt,
// que é o que o terminal receberia: a côr prova-se depois de atravessar o
// elemento, e não sómente no quadro.
//
// Painel de duas: teto 16, e 0,95 dá 15 degraus, um cheio e resto sete, d'onde
// as DUAS célullas; 0,30 dá 4 degraus, que é uma célulla, na base.
TEST_CASE("as quatro batidas sahem nas quatro côres, e a vizinha fria não") {
  const std::vector<float> bandas = {0.95f, 0.30f, 0.95f, 0.30f,
                                     0.95f, 0.30f, 0.95f, 0.30f};
  const std::vector<float> centros = {100.0f,  100.0f,  500.0f,  500.0f,
                                      2000.0f, 2000.0f, 8000.0f, 8000.0f};
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(8),
                                              ftxui::Dimension::Fixed(2));
  ftxui::Render(ecran, es::elemento_do_espectro(
                           es::compor(bandas, 8, 2, false, centros)));

  // Os tokens escriptos Á MÃO, na ordem em que o ouvido sobe.
  const std::string_view batidas[] = {tk::glow_hot, tk::data5, tk::data3,
                                      tk::data2};
  for (int f = 0; f < 4; ++f) {
    // A columna QUENTE veste a batida da familia d'ella, da base ao topo.
    CHECK(ecran.PixelAt(2 * f, 1).foreground_color == cor(tk::rgb(batidas[f])));
    CHECK(ecran.PixelAt(2 * f, 0).foreground_color == cor(tk::rgb(batidas[f])));
    // E a vizinha FRIA veste a rampa, que é a mesma nas quatro familias.
    CHECK(ecran.PixelAt(2 * f + 1, 1).foreground_color ==
          cor(tk::mistura(tk::v500, tk::panel_hi, 0.55)));
  }
}

// ── C6 · o ladrilho exacto, e a cobertura de toda banda ─────────────────────
TEST_CASE("o quadro fecha a largura exacta, de uma a duzentas collunhas") {
  const std::vector<float> bandas = bandas_uniformes(0.5f);
  for (std::size_t largura = 1; largura <= 200; ++largura) {
    const es::Quadro quadro = es::compor(bandas, largura, 3);
    // Nem estoura, nem deixa buraco: a conta do tamanho faz-se aqui, á mão.
    CHECK(quadro.largura == largura);
    CHECK(quadro.altura == 3);
    CHECK(quadro.celulas.size() == largura * 3);
  }
}

// A COBERTURA prova-se por SONDA, e não repetindo a conta da repartição. Para
// cada banda arma-se um pico solitario e afere-se que ELLE APPARECE na fita: é
// asserção de comportamento, e não a mesma fórmula escripta duas vezes. Fosse a
// prova a recalcular os intervallos, ella affirmaria a arithmetica que a obra já
// affirma, e uma repartição que perdesse banda passaria nas duas.
TEST_CASE("banda alguma se perde, em largura alguma") {
  const std::size_t larguras[] = {1, 2, 3, 5, 7, 11, 13, 23, 24, 25, 47, 80, 200};
  for (const std::size_t largura : larguras) {
    for (std::size_t b = 0; b < mysong::nucleo::QUANTAS_BANDAS; ++b) {
      std::vector<float> bandas(mysong::nucleo::QUANTAS_BANDAS, 0.0f);
      bandas[b] = 1.0f;  // pico solitario: enche a columna que o contiver
      const es::Quadro quadro = es::compor(bandas, largura, 4);

      // Alguma columna ha de subir ao TOPO (linha 0) cheia. Se a repartição
      // saltasse esta banda, columna alguma subiria, e a fita mostraria o pico
      // de outra banda ou pico nenhum. O pico solitario é QUENTE, d'onde nas
      // larguras em que a banda toma varias collunhas o topo sae na PONTA
      // (issue #141) e não no bloco; aceitam-se os tres glifos, que o que se
      // afere aqui é a repartição e não o remate.
      bool alguma_cheia = false;
      for (std::size_t c = 0; c < largura; ++c)
        if (quadro.em(0, c).pinta &&
            (quadro.em(0, c).glifo == kCheio ||
             quadro.em(0, c).glifo == std::string(es::kFlancoQueSobe) ||
             quadro.em(0, c).glifo == std::string(es::kFlancoQueDesce)))
          alguma_cheia = true;
      CHECK(alguma_cheia);
    }
  }
}

// ── C7 · painel estreito funde por MÁXIMO ───────────────────────────────────
TEST_CASE("em largura de uma collunha a fita mostra o pico do quadro") {
  std::vector<float> bandas(mysong::nucleo::QUANTAS_BANDAS, 0.10f);
  bandas[17] = 0.80f;  // o pico, e mora longe das bordas

  // O alvo recalcula-se com conta PROPRIA, e não se pergunta á obra: o máximo do
  // vector afere-se por laço escripto aqui, e d'elle se derivam os degraus.
  float pico = 0.0f;
  for (const float valor : bandas)
    if (valor > pico) pico = valor;
  REQUIRE(pico == 0.80f);

  const es::Quadro quadro = es::compor(bandas, 1, 5);
  REQUIRE(quadro.largura == 1);
  // Teto 40, 0,80 vezes 40 = 32 degraus, QUATRO blocos cheios e resto zero,
  // d'onde quatro célullas: linhas 4, 3, 2 e 1 pintadas, e a linha 0 vazia.
  CHECK(quadro.em(4, 0).glifo == kCheio);
  CHECK(quadro.em(1, 0).glifo == kCheio);
  CHECK(quadro.em(0, 0).pinta == false);
}

// Fundir por máximo e não amostrar: a banda vizinha ao pico é BAIXA, e ainda
// assim a columna que as cobre ambas sahe pelo pico. Amostrando, sahiria pela
// que o sorteio apanhasse, e metade das vezes o pico desappareceria.
TEST_CASE("a columna que cobre pico e vale sahe pelo pico") {
  std::vector<float> bandas(mysong::nucleo::QUANTAS_BANDAS, 0.0f);
  bandas[0] = 1.0f;   // pico na primeira banda
  bandas[1] = 0.0f;   // vale imediatamente ao lado
  // Largura 12 sobre 24 bandas: cada columna cobre DUAS bandas, d'onde a
  // columna 0 cobre as bandas 0 e 1, que são justamente o pico e o vale.
  const es::Quadro quadro = es::compor(bandas, 12, 4);
  CHECK(quadro.em(0, 0).pinta);
  // Largura 12 sobre 24 bandas: cada banda cae DENTRO de uma collunha só, e
  // ahi não ha ponta (issue #141), que meia diagonal sósinha é degrau e não
  // seta. O topo fica bloco.
  CHECK(quadro.em(0, 0).glifo == kCheio);
  // E a columna seguinte, que cobre as bandas 2 e 3, ambas em zero, fica no piso.
  CHECK(quadro.em(3, 1).glifo == kUm);
  CHECK(quadro.em(0, 1).pinta == false);
}

// ── C8 · redimensionar recompõe sem quebrar ─────────────────────────────────
// Aqui a comparação de uma sahida de compor() com outra É o que se affirma, e
// por isso é legitima: o caso não pergunta á obra qual é o desenho, pergunta se
// o desenho VOLTA. É a unica excepção á regra (a) do tractado, e vae dita.
TEST_CASE("compor em duas larguras e voltar dá o mesmo quadro") {
  std::vector<float> bandas(mysong::nucleo::QUANTAS_BANDAS, 0.0f);
  for (std::size_t b = 0; b < bandas.size(); ++b)
    bandas[b] = static_cast<float>(b) / static_cast<float>(bandas.size());

  const es::Quadro primeiro = es::compor(bandas, 40, 6);
  const es::Quadro pelo_meio = es::compor(bandas, 13, 6);  // estreita
  const es::Quadro de_volta = es::compor(bandas, 40, 6);   // e volta

  REQUIRE(primeiro.celulas.size() == de_volta.celulas.size());
  CHECK(pelo_meio.largura == 13);  // o do meio de facto mudou de fórma

  // Campo a campo: glifo, tinta e o pintar. Estado escondido apparecería aqui.
  bool identico = true;
  for (std::size_t i = 0; i < primeiro.celulas.size(); ++i) {
    if (primeiro.celulas[i].glifo != de_volta.celulas[i].glifo) identico = false;
    if (primeiro.celulas[i].pinta != de_volta.celulas[i].pinta) identico = false;
    if (!es::mesma_tinta(primeiro.celulas[i].tinta, de_volta.celulas[i].tinta))
      identico = false;
  }
  CHECK(identico);
}

TEST_CASE("painel sem largura ou sem altura dá quadro vazio") {
  const std::vector<float> bandas = bandas_uniformes(0.5f);
  CHECK(es::compor(bandas, 0, 5).celulas.empty());
  CHECK(es::compor(bandas, 10, 0).celulas.empty());
  CHECK(es::compor(bandas, 0, 0).celulas.empty());
  // E vector de bandas VAZIO não presume vinte e quatro: compõe o piso, visto
  // que columna alguma tem valor, e sobretudo não lê fóra de limite.
  const es::Quadro sem_bandas = es::compor({}, 6, 3);
  CHECK(sem_bandas.celulas.size() == 18);
  CHECK(sem_bandas.em(2, 0).glifo == kUm);
}

namespace {

// sem_escape — a linha despida do escape e do retorno de carro, que é o que
// ella MOSTRA. Compara-se a linha INTEIRA contra a esperada, e a egualdade diz
// posição e contagem n'uma asserção só, sem se contar codepoint algum.
std::string sem_escape(const std::string& linha) {
  std::string limpa;
  for (std::size_t i = 0; i < linha.size(); ++i) {
    const unsigned char oct = static_cast<unsigned char>(linha[i]);
    if (oct == 0x1b) {  // o ++i do laço salta a letra que fecha a sequencia
      while (i < linha.size() &&
             !std::isalpha(static_cast<unsigned char>(linha[i])))
        ++i;
      continue;
    }
    if (oct == '\r') continue;
    limpa += linha[i];
  }
  return limpa;
}

}  // namespace

// ── C10 · o elemento pintado em écran de PAPEL ──────────────────────────────
TEST_CASE("o elemento mostra a linha que o quadro manda") {
  // Teto 32: 0,899 x 32 = 28,768, floor 28, tres cheios e resto 4, QUATRO
  // célullas, que enchem o painel de alto a baixo.
  const es::Quadro quadro = es::compor(bandas_uniformes(0.899f), 10, 4);
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(10),
                                              ftxui::Dimension::Fixed(4));
  ftxui::Render(ecran, es::elemento_do_espectro(quadro));

  std::vector<std::string> linhas;
  std::string corrente;
  for (const char oct : ecran.ToString()) {
    if (oct != '\n') { corrente += oct; continue; }
    linhas.push_back(sem_escape(corrente));
    corrente.clear();
  }
  if (!corrente.empty()) linhas.push_back(sem_escape(corrente));
  REQUIRE(linhas.size() == 4);
  for (std::size_t l = 0; l < 4; ++l) {
    // Monta-se do QUADRO, glifo a glifo: nem estoura, nem deixa buraco.
    std::string esperada;
    for (std::size_t c = 0; c < 10; ++c) esperada += quadro.em(l, c).glifo;
    CHECK(linhas[l] == esperada);
  }
}

// ── C9 · os bytes emittidos ─────────────────────────────────────────────────
// A sequencia esperada vae escripta Á MÃO, octeto a octeto, e não composta por
// tokens::sgr: escripta por sgr, o caso affirmaria que a obra chama sgr, que é o
// que já se vê no codigo. Escripta á mão, affirma a SEQUENCIA. Os numeros sahem
// da base dos graves, que é v500 = #8b5cf6 composto sobre panel_hi = #1b1030 com
// peso 0,55: 139 por 0,55 mais 27 por 0,45 dá 89, e assim 58 e 157.
TEST_CASE("a tinta sahe immediatamente antes do glifo, sem repouso pelo meio") {
  const es::Quadro quadro =
      es::compor(bandas_uniformes(0.899f), 4, 5, false, centros_em(100.0f));
  // A base veste a côr dos graves composta, e traz bloco cheio: dous cheios ao
  // menos, pela conta do caso da rampa (35 degraus, quatro cheios e resto tres).
  const es::Celula& base = quadro.em(4, 0);
  REQUIRE(base.pinta);
  REQUIRE(base.glifo == kCheio);
  CHECK(es::sequencia_da_celula(base) == "\x1b[38;2;89;58;157m█");

  // A célulla que NÃO pinta sahe em ordem de repouso, e traz o espaço.
  const es::Celula vazia;
  REQUIRE(vazia.pinta == false);
  CHECK(es::sequencia_da_celula(vazia) == "\x1b[0m ");

  // E a NEGATIVA que fecha a emenda: na célulla pintada não ha repouso algum
  // entre a tinta e o glifo. Havendo-o, o fundo do painel apparecería entre uma
  // barra e a seguinte, e a fita sahiria costurada em vez de continua.
  CHECK(es::sequencia_da_celula(base).find("\x1b[0m") == std::string::npos);
}

// O piso do silencio, em bytes: text_faint = #463566, que é 70, 53 e 102.
TEST_CASE("o piso do silencio sahe em text_faint com o bloco de um oitavo") {
  const es::Quadro quadro = es::compor(bandas_uniformes(0.0f), 3, 4);
  const es::Celula& piso = quadro.em(3, 0);
  REQUIRE(piso.pinta);
  CHECK(es::sequencia_da_celula(piso) == "\x1b[38;2;70;53;102m▁");
}

// A PONTA DA BATIDA (issue #139, refeita pela #141). A columna que accende na
// côr do registro acaba n'uma seta ENCORPADA, feita das meias diagonaes
// powerline: o flanco que sobe na primeira collunha da banda, o que desce na
// ultima, e o bloco cheio nas do meio. As frias, a muda, a de uma cella e a
// banda de uma collunha ficam como estavam.
TEST_CASE("a banda quente remata em seta, flanco a flanco") {
  std::vector<float> bandas(mysong::nucleo::QUANTAS_BANDAS, 0.0f);
  bandas[3] = 1.0f;   // teto cheio: quente
  bandas[4] = 0.40f;  // fria
  // Tres collunhas por banda, que é o que a tela d'elle dá: 24 bandas em 72.
  const es::Quadro quadro =
      es::compor(bandas, 3 * mysong::nucleo::QUANTAS_BANDAS, 5, false,
                 centros_em(100.0f));
  // As tres collunhas da banda 3 são a 9, a 10 e a 11: a ponta lê-se n'ellas,
  // da esquerda para a direita, e é UMA seta.
  CHECK(quadro.em(0, 9).glifo == std::string(es::kFlancoQueSobe));
  CHECK(quadro.em(0, 10).glifo == std::string(es::kBlocoCheio));
  CHECK(quadro.em(0, 11).glifo == std::string(es::kFlancoQueDesce));
  // Por baixo da ponta, bloco cheio: a seta é o REMATE, e não a barra.
  for (std::size_t l = 1; l < 5; ++l)
    for (std::size_t c = 9; c <= 11; ++c)
      CHECK(quadro.em(l, c).glifo == es::glifo_do_degrau(8));
  // A ponta veste a MESMA côr da columna: ella é o topo da barra, e não peça
  // á parte.
  CHECK(es::mesma_tinta(quadro.em(0, 9).tinta, quadro.em(1, 9).tinta));
  CHECK(es::mesma_tinta(quadro.em(0, 10).tinta,
                        tk::rgb(es::tinta_do_registro(es::Registro::Graves))));
  // A FRIA acaba em bloco, e a altura d'ella é a de sempre: duas cellas.
  CHECK(quadro.em(3, 12).glifo == es::glifo_do_degrau(8));
  CHECK(quadro.em(4, 12).glifo == es::glifo_do_degrau(8));
  CHECK_FALSE(quadro.em(2, 12).pinta);
}

TEST_CASE("a taboada da ponta dá flanco, bloco, e nada na banda de uma collunha") {
  CHECK(es::glifo_da_ponta(true, false) == es::kFlancoQueSobe);
  CHECK(es::glifo_da_ponta(false, true) == es::kFlancoQueDesce);
  CHECK(es::glifo_da_ponta(false, false) == es::kBlocoCheio);
  CHECK(es::glifo_da_ponta(true, true).empty());

  std::vector<float> bandas(mysong::nucleo::QUANTAS_BANDAS, 0.0f);
  bandas[3] = 0.95f;
  // Banda de UMA collunha: a fita tem tantas collunhas quantas bandas, e ahi a
  // barra fica com o topo de BLOCO, que meia diagonal sósinha é degrau.
  const es::Quadro justo = es::compor(bandas, mysong::nucleo::QUANTAS_BANDAS, 5,
                                      false, centros_em(100.0f));
  CHECK(justo.em(0, 3).glifo == es::glifo_do_degrau(6));

  // Painel de UMA linha: a columna tem uma cella só, e ponta sem corpo não é
  // barra. Fica o bloco, mesmo com tres collunhas por banda.
  const es::Quadro raso =
      es::compor(bandas, 3 * mysong::nucleo::QUANTAS_BANDAS, 1, false,
                 centros_em(100.0f));
  CHECK(raso.em(0, 9).glifo != std::string(es::kFlancoQueSobe));
  CHECK(raso.em(0, 10).glifo != std::string(es::kBlocoCheio));

  // E o MUDO vence a ponta, como vence a côr: fita calada não tem batida.
  const es::Quadro calado =
      es::compor(bandas, 3 * mysong::nucleo::QUANTAS_BANDAS, 5, true,
                 centros_em(100.0f));
  CHECK(calado.em(0, 9).glifo == es::glifo_do_degrau(6));
  CHECK(es::mesma_tinta(calado.em(0, 9).tinta, tk::rgb(tk::text_faint)));
}
