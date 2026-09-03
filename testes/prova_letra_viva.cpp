// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DO RIO — testes/prova_letra_viva.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o rio da letra da issue #109. Caso algum abre terminal, som ou relogio:
// o rio resolve-se em QUADRO por POSIÇÃO, e é pelo quadro que se prova. Sendo a
// funcção pura, a posição escreve-se á mão e o instante da prova é o instante
// que se quer, sem esperar segundo algum.
//
// A LIÇÃO D'ESTA CASA, que aqui se obedece: o alvo esperado vem escripto Á MÃO,
// ou recalculado por conta INDEPENDENTE. Jamais se compara a sahida de
// quadro_da_letra() com outra sahida d'ella, salvo quando a egualdade das duas É
// o que se afirma, que é o caso da pureza. E nome de caso sem ponto e vírgula,
// que o ponto e vírgula parte a lista do CMake e zero casos corridos dá status
// de successo.
//
// DOMÍNIO ......... versos armados aqui mesmo, com os instantes escolhidos no
//                   proprio caso, e um quadro de espectro de barras cheias.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... o que se afirma é a REGRA do rio, e não a apparencia: a
//                   linha da tela por INDICE, a fracção resolvida, a tinta por
//                   token, e a composição por célulla.
// Q.E.D. .......... provada a linha da tela por indice, o rio de cabeça para
//                   baixo deixa de passar calado, que a contagem de célullas não
//                   muda com o sentido.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
#include <string>
#include <vector>

#include <ftxui/screen/screen.hpp>

#include "nucleo/letra.hpp"
#include "tui/espectro.hpp"
#include "tui/letra_viva.hpp"
#include "tui/tokens.hpp"

namespace tui = mysong::tui;
namespace nu = mysong::nucleo;
namespace tk = mysong::tui::tokens;

namespace {

// A LETRA de prova: dous versos, um aos dez segundos e outro aos catorze. Cinco
// glyphos cada, que é o que faz as contas da fracção resolvida sahirem redondas.
const std::vector<nu::LinhaDaLetra> kVersos = {{10.0, "abcde"}, {14.0, "fghij"}};

// O painel: vinte por doze. D'ahi a linha de leitura em quatro (doze a dividir
// por tres) e a base em onze, e o vão da subida de sete. Escrevem-se Á MÃO, que
// recalculá-los aqui seria perguntar ao oraculo sob exame.
constexpr std::size_t kLargura = 20;
constexpr std::size_t kAltura = 12;
constexpr std::size_t kLeitura = 4;
constexpr std::size_t kBase = 11;

// d_ella — a linha viva de tal verso, ou nada. Busca-se pelo `qual`, e não pela
// ordem no vector: quem morre no alto sae do quadro, e o indice deslocava-se.
const tui::LinhaViva* d_ella(const tui::QuadroDaLetra& quadro,
                             std::size_t qual) {
  for (const tui::LinhaViva& viva : quadro.linhas)
    if (viva.qual == qual) return &viva;
  return nullptr;
}

}  // namespace

TEST_CASE("a linha de leitura é o terço do alto") {
  CHECK(tui::linha_de_leitura(kAltura) == kLeitura);
  CHECK(tui::linha_de_leitura(30) == 10);
  CHECK(tui::linha_de_leitura(3) == 1);
  // Painel de uma linha e painel de nenhuma não estouram: a leitura é o zero, e
  // com ella o vão da subida fica vazio, que é o que um painel d'esses comporta.
  CHECK(tui::linha_de_leitura(1) == 0);
  CHECK(tui::linha_de_leitura(0) == 0);
}

TEST_CASE("o nascimento é de quatro segundos ou do intervallo menor") {
  CHECK(tui::nascimento_da_linha(kVersos, 0) == doctest::Approx(4.0));
  CHECK(tui::nascimento_da_linha(kVersos, 1) == doctest::Approx(4.0));
  const std::vector<nu::LinhaDaLetra> apertados = {
      {1.0, "um"}, {2.5, "dous"}, {2.5, "tres"}};
  // O primeiro conta desde o ZERO da faixa: um segundo, e não quatro.
  CHECK(tui::nascimento_da_linha(apertados, 0) == doctest::Approx(1.0));
  CHECK(tui::nascimento_da_linha(apertados, 1) == doctest::Approx(1.5));
  // Carimbo repetido dá intervallo zero, e o piso guarda a divisão.
  CHECK(tui::nascimento_da_linha(apertados, 2) ==
        doctest::Approx(tui::NASCIMENTO_MINIMO));
  // Indice fóra de limite não lê fóra do vector.
  CHECK(tui::nascimento_da_linha(apertados, 9) == doctest::Approx(4.0));
}

TEST_CASE("a linha nasce na base sobe e chega á leitura no instante d'ella") {
  // ANTES de nascer não ha rio nenhum.
  CHECK(tui::quadro_da_letra(kVersos, 5.9, kLargura, kAltura).vazio());

  // AO NASCER, seis segundos, que são os quatro antes dos dez.
  const tui::QuadroDaLetra nasce =
      tui::quadro_da_letra(kVersos, 6.0, kLargura, kAltura);
  REQUIRE(d_ella(nasce, 0) != nullptr);
  CHECK(d_ella(nasce, 0)->linha_da_tela == kBase);
  CHECK(d_ella(nasce, 0)->resolvida == doctest::Approx(0.0));
  CHECK(d_ella(nasce, 0)->tinta == tk::text_faint);
  CHECK_FALSE(d_ella(nasce, 0)->corrente);

  // A MEIO da subida: metade do vão de sete, que arredonda para quatro.
  const tui::QuadroDaLetra meio =
      tui::quadro_da_letra(kVersos, 8.0, kLargura, kAltura);
  CHECK(d_ella(meio, 0)->linha_da_tela == kBase - 4);
  CHECK(d_ella(meio, 0)->resolvida == doctest::Approx(0.5));
  CHECK(d_ella(meio, 0)->tinta == tk::text_body);

  // NO INSTANTE: a linha de leitura, inteira e brilhante.
  const tui::QuadroDaLetra canta =
      tui::quadro_da_letra(kVersos, 10.0, kLargura, kAltura);
  CHECK(d_ella(canta, 0)->linha_da_tela == kLeitura);
  CHECK(d_ella(canta, 0)->resolvida == doctest::Approx(1.0));
  CHECK(d_ella(canta, 0)->tinta == tk::text_bright);
  CHECK(d_ella(canta, 0)->corrente);
  CHECK(d_ella(canta, 0)->texto == "abcde");
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
