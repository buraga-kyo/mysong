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

TEST_CASE("a linha fica na leitura até a proxima chegar e depois apaga") {
  // Ainda corrente um decimo antes do instante da seguinte.
  const tui::QuadroDaLetra ainda =
      tui::quadro_da_letra(kVersos, 13.9, kLargura, kAltura);
  CHECK(d_ella(ainda, 0)->linha_da_tela == kLeitura);
  CHECK(d_ella(ainda, 0)->corrente);

  // NO INSTANTE DA SEGUINTE sae, que é a seguinte que toma o logar: uma linha
  // acima, em text_muted, e a corrente passa a ser a outra.
  const tui::QuadroDaLetra sae =
      tui::quadro_da_letra(kVersos, 14.0, kLargura, kAltura);
  CHECK(d_ella(sae, 0)->linha_da_tela == kLeitura - 1);
  CHECK(d_ella(sae, 0)->tinta == tk::text_muted);
  CHECK_FALSE(d_ella(sae, 0)->corrente);
  CHECK(d_ella(sae, 1)->linha_da_tela == kLeitura);
  CHECK(d_ella(sae, 1)->corrente);

  // Uma linha por segundo, e do segundo degrau em deante em text_faint.
  const tui::QuadroDaLetra dous =
      tui::quadro_da_letra(kVersos, 15.0, kLargura, kAltura);
  CHECK(d_ella(dous, 0)->linha_da_tela == kLeitura - 2);
  CHECK(d_ella(dous, 0)->tinta == tk::text_faint);

  // SOME NA LINHA ZERO: no quarto degrau ainda se vê, no quinto já não.
  CHECK(d_ella(tui::quadro_da_letra(kVersos, 17.0, kLargura, kAltura), 0)
            ->linha_da_tela == 0);
  CHECK(d_ella(tui::quadro_da_letra(kVersos, 18.0, kLargura, kAltura), 0) ==
        nullptr);
}

TEST_CASE("duas linhas seguidas e perto não se pisam") {
  const tui::QuadroDaLetra dous =
      tui::quadro_da_letra(kVersos, 12.0, kLargura, kAltura);
  REQUIRE(dous.linhas.size() == 2);
  // Uma na leitura e a outra a meio da subida: linhas differentes da tela.
  CHECK(d_ella(dous, 0)->linha_da_tela == kLeitura);
  CHECK(d_ella(dous, 1)->linha_da_tela == kBase - 4);
  // E UMA corrente só, que é a que se canta.
  CHECK(tui::linha_corrente_do_rio(dous) == d_ella(dous, 0));
  CHECK_FALSE(d_ella(dous, 1)->corrente);
  // No instante do primeiro verso a segunda já assomou na base, que o
  // intervallo entre os dous é justamente o nascimento d'ella.
  const tui::QuadroDaLetra assoma =
      tui::quadro_da_letra(kVersos, 10.0, kLargura, kAltura);
  REQUIRE(d_ella(assoma, 1) != nullptr);
  CHECK(d_ella(assoma, 1)->linha_da_tela == kBase);
  CHECK(d_ella(assoma, 1)->tinta == tk::text_faint);
}

TEST_CASE("a linha comprida corta-se com reticencias e a curta centra-se") {
  const std::vector<nu::LinhaDaLetra> comprida = {{10.0, "abcdefghijklm"}};
  const tui::QuadroDaLetra apertado =
      tui::quadro_da_letra(comprida, 10.0, 10, kAltura);
  REQUIRE(apertado.linhas.size() == 1);
  CHECK(apertado.linhas[0].texto == "abcdefghi…");
  CHECK(apertado.linhas[0].collunha == 0);
  // Painel de UMA collunha dá as reticencias e mais nada, e não estoura.
  const tui::QuadroDaLetra fio = tui::quadro_da_letra(comprida, 10.0, 1, kAltura);
  REQUIRE(fio.linhas.size() == 1);
  CHECK(fio.linhas[0].texto == "…");
  // A CURTA centra-se: cinco glyphos em vinte deixam sete de cada lado.
  const tui::QuadroDaLetra folgado =
      tui::quadro_da_letra(kVersos, 10.0, kLargura, kAltura);
  CHECK(folgado.linhas[0].collunha == 7);
}

TEST_CASE("a faixa sem letra dá quadro vazio") {
  const std::vector<nu::LinhaDaLetra> nenhuma;
  const tui::QuadroDaLetra nada =
      tui::quadro_da_letra(nenhuma, 30.0, kLargura, kAltura);
  CHECK(nada.vazio());
  CHECK(nada.corrente == -1);
  // A medida conserva-se, que é o que a composição lê para não pintar nada.
  CHECK(nada.largura == kLargura);
  CHECK(nada.altura == kAltura);
  CHECK(tui::linha_corrente_do_rio(nada) == nullptr);
  CHECK(tui::caixa_da_corrente(nada).vazio());
  // Painel sem medida tambem não estoura.
  CHECK(tui::quadro_da_letra(kVersos, 10.0, 0, 0).vazio());
  // O SILENCIO que o LRCLIB marca com carimbo e texto vazio não pinta linha.
  const std::vector<nu::LinhaDaLetra> calado = {{10.0, ""}};
  CHECK(tui::quadro_da_letra(calado, 10.0, kLargura, kAltura).vazio());
}

TEST_CASE("o embaralhado sahe das proprias letras e é determinístico") {
  const std::vector<std::string> letras = tui::glifos_da_linha("abcde");
  const std::string mexido = tui::embaralha(letras, 0.0, 3, 17);
  // DETERMINISTICO: a mesma chamada dá a mesma fita, sem excepção.
  CHECK(tui::embaralha(letras, 0.0, 3, 17) == mexido);
  // E cada glypho sahe das PROPRIAS letras da linha.
  for (const std::string& glifo : tui::glifos_da_linha(mexido))
    CHECK(std::string("abcde").find(glifo) != std::string::npos);
  // Resolvido por inteiro dá a linha tal qual.
  CHECK(tui::embaralha(letras, 1.0, 3, 17) == "abcde");
  // A MEIO, os resolvidos são os do meio: tres de cinco, do um ao tres.
  const std::vector<std::string> meio =
      tui::glifos_da_linha(tui::embaralha(letras, 0.5, 3, 17));
  REQUIRE(meio.size() == 5);
  CHECK(meio[1] == "b");
  CHECK(meio[2] == "c");
  CHECK(meio[3] == "d");
  // O BRANCO conserva-se branco, e é elle que deixa ler a fórma das palavras.
  const std::vector<std::string> com_vao = tui::glifos_da_linha("ab cd");
  CHECK(tui::glifos_da_linha(tui::embaralha(com_vao, 0.0, 1, 2))[2] == " ");
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
