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

#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include "nucleo/analisador.hpp"
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

// Os glifos ESCRIPTOS Á MÃO, por ponto de codigo. Escrevem-se por \u e não pelo
// glifo cru para que a prova não dependa da codificação com que o editor gravou
// este arquivo, e para que o assento de cada degrau se leia como numero.
constexpr const char* kCheio = "\u2588";  // oito oitavos, o bloco cheio
constexpr const char* kCinco = "\u2585";  // cinco oitavos
constexpr const char* kUm = "\u2581";     // um oitavo, o piso do silencio
constexpr const char* kVazio = " ";

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
TEST_CASE("a rampa vae de v700 na base a v400 no topo") {
  const std::vector<float> bandas = bandas_uniformes(0.899f);
  const es::Quadro quadro = es::compor(bandas, mysong::nucleo::QUANTAS_BANDAS, 5);

  REQUIRE(quadro.altura == 5);
  for (std::size_t c = 0; c < quadro.largura; ++c) {
    REQUIRE(quadro.em(4, c).pinta);
    REQUIRE(quadro.em(0, c).pinta);

    // A BASE (linha 4) veste v700 EXACTO, conferido contra o TOKEN e não contra
    // outra chamada da obra: é asserção de fóra, e não pergunta ao oraculo.
    CHECK(es::mesma_tinta(quadro.em(4, c).tinta, tk::rgb(tk::v700)));
    // O TOPO (linha 0) veste v400 EXACTO, pelo mesmo modo.
    CHECK(es::mesma_tinta(quadro.em(0, c).tinta, tk::rgb(tk::v400)));

    // O MEIO (linha 2) fica a meia rampa. O peso recalcula-se AQUI: a linha 2 é
    // a terceira desde a base d'um painel de cinco, d'onde o alto vale 2 e o
    // peso 2/4, que é 0,5. Compõe-se por tokens::mistura, que é a mesma
    // interpolação que a issue #2 já prova, e não pela funcção sob exame.
    CHECK(es::mesma_tinta(quadro.em(2, c).tinta,
                          tk::mistura(tk::v400, tk::v700, 0.5)));
  }
}

TEST_CASE("painel de uma célulla veste a base da rampa") {
  const es::Quadro quadro = es::compor(bandas_uniformes(0.5f), 8, 1);
  REQUIRE(quadro.altura == 1);
  for (std::size_t c = 0; c < quadro.largura; ++c) {
    REQUIRE(quadro.em(0, c).pinta);
    CHECK(es::mesma_tinta(quadro.em(0, c).tinta, tk::rgb(tk::v700)));
  }
}
