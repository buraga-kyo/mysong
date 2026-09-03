// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DO RIO — testes/prova_letra_viva.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o rio da letra da issue #109, e a chapa em XIROD da linha corrente da
// issue #110, que d'elle pende e por isso aqui mora. Caso algum abre terminal,
// som, relogio nem X11: o rio resolve-se em QUADRO por POSIÇÃO, e é pelo quadro
// que se prova, e a chapa é ORDEM tirada d'esse quadro. Sendo a
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
#include "nucleo/letreiro.hpp"
#include "tui/espectro.hpp"
#include "tui/letra_viva.hpp"
#include "tui/sala.hpp"
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

TEST_CASE("a caixa da corrente diz a linha da tela e as collunhas") {
  const tui::QuadroDaLetra canta =
      tui::quadro_da_letra(kVersos, 10.0, kLargura, kAltura);
  const tui::Rectangulo caixa = tui::caixa_da_corrente(canta);
  CHECK_FALSE(caixa.vazio());
  CHECK(caixa.x == 7);
  CHECK(caixa.y == kLeitura);
  CHECK(caixa.largura == 5);
  CHECK(caixa.altura == 1);
  // Sem corrente a caixa sae vazia, e por ahi se sabe que chapa alguma se põe.
  const tui::QuadroDaLetra subindo =
      tui::quadro_da_letra(kVersos, 8.0, kLargura, kAltura);
  CHECK(tui::linha_corrente_do_rio(subindo) == nullptr);
  CHECK(tui::caixa_da_corrente(subindo).vazio());
}

TEST_CASE("a mesma posição dá o mesmo quadro") {
  const tui::QuadroDaLetra uma =
      tui::quadro_da_letra(kVersos, 11.7, kLargura, kAltura);
  const tui::QuadroDaLetra outra =
      tui::quadro_da_letra(kVersos, 11.7, kLargura, kAltura);
  REQUIRE(uma.linhas.size() == outra.linhas.size());
  for (std::size_t i = 0; i < uma.linhas.size(); ++i) {
    CHECK(uma.linhas[i].linha_da_tela == outra.linhas[i].linha_da_tela);
    CHECK(uma.linhas[i].texto == outra.linhas[i].texto);
    CHECK(uma.linhas[i].tinta == outra.linhas[i].tinta);
  }
}

namespace {

// barras — o quadro do espectro com TODAS as bandas cheias, donde toda célulla
// é bloco cheio. É contra este chão que se lê o que o rio esconde.
tui::Quadro barras() {
  return tui::compor(std::vector<float>(24, 1.0f), kLargura, kAltura);
}

// papel — o écran de papel, que é como esta Casa prova desenho sem terminal.
ftxui::Screen papel(ftxui::Element quadro) {
  ftxui::Screen ecran =
      ftxui::Screen::Create(ftxui::Dimension::Fixed(static_cast<int>(kLargura)),
                            ftxui::Dimension::Fixed(static_cast<int>(kAltura)));
  ftxui::Render(ecran, quadro);
  return ecran;
}

// kComVao — um verso com vão no meio, para se aferir que o branco entre as
// palavras deixa passar a barra e não vira tarja.
const std::vector<nu::LinhaDaLetra> kComVao = {{10.0, "ab cd"}};

}  // namespace

TEST_CASE("a célulla com letra esconde a barra e o vão deixa-a passar") {
  const tui::Quadro espectro = barras();
  REQUIRE(espectro.em(kLeitura, 0).glifo == "█");
  const tui::QuadroDaLetra rio =
      tui::quadro_da_letra(kComVao, 10.0, kLargura, kAltura);
  REQUIRE(rio.linhas.size() == 1);
  const std::size_t x = rio.linhas[0].collunha;
  const std::vector<tui::CelulaDoRio> tapete = tui::tapete_do_rio(espectro, rio);
  const auto em = [&](std::size_t l, std::size_t c) -> const tui::CelulaDoRio& {
    return tapete[l * kLargura + c];
  };
  CHECK(em(kLeitura, x).glifo == "a");
  CHECK(em(kLeitura, x).letra);
  CHECK(tui::mesma_tinta(em(kLeitura, x).tinta, tk::rgb(tk::text_bright)));
  // O VÃO entre as palavras deixa passar a barra.
  CHECK(em(kLeitura, x + 2).glifo == "█");
  CHECK_FALSE(em(kLeitura, x + 2).letra);
  // E as célullas ao lado da linha, e a linha de cima, ficam com as barras.
  CHECK(em(kLeitura, 0).glifo == "█");
  CHECK_FALSE(em(kLeitura, 0).letra);
  CHECK(em(kLeitura - 1, x).glifo == "█");
}

TEST_CASE("o fundo do painel sae sómente debaixo da letra") {
  const tui::Quadro espectro = barras();
  const tui::QuadroDaLetra rio =
      tui::quadro_da_letra(kComVao, 10.0, kLargura, kAltura);
  const ftxui::Screen ecran = papel(tui::elemento_do_rio(espectro, rio));
  const int x = static_cast<int>(rio.linhas[0].collunha);
  const int y = static_cast<int>(kLeitura);
  const tk::Triade tom = tk::rgb(tk::panel);
  const ftxui::Color cama = ftxui::Color::RGB(tom.r, tom.g, tom.b);
  const tk::Triade brilho = tk::rgb(tk::text_bright);

  CHECK(ecran.PixelAt(x, y).character == "a");
  CHECK(ecran.PixelAt(x, y).background_color == cama);
  CHECK(ecran.PixelAt(x, y).foreground_color ==
        ftxui::Color::RGB(brilho.r, brilho.g, brilho.b));
  // A barra ao lado conserva-se, e SEM a cama do painel.
  CHECK(ecran.PixelAt(0, y).character == "█");
  CHECK_FALSE(ecran.PixelAt(0, y).background_color == cama);
  // O vão entre as palavras tambem fica com a barra e sem cama.
  CHECK(ecran.PixelAt(x + 2, y).character == "█");
  CHECK_FALSE(ecran.PixelAt(x + 2, y).background_color == cama);
}

TEST_CASE("o rio escondido devolve o espectro tal qual") {
  const tui::Quadro espectro = barras();
  const ftxui::Screen ecran =
      papel(tui::elemento_do_rio(espectro, tui::QuadroDaLetra{}));
  const tk::Triade tom = tk::rgb(tk::panel);
  const ftxui::Color cama = ftxui::Color::RGB(tom.r, tom.g, tom.b);
  // A linha de leitura é barra como as outras: o `l` esconde o rio, e o
  // espectro NÃO some por causa d'elle.
  for (int x = 0; x < static_cast<int>(kLargura); ++x) {
    CHECK(ecran.PixelAt(x, static_cast<int>(kLeitura)).character == "█");
    CHECK_FALSE(ecran.PixelAt(x, static_cast<int>(kLeitura)).background_color ==
                cama);
  }
  // E o tapete de um espectro sem medida não estoura nem lê fóra.
  CHECK(tui::tapete_do_rio(tui::Quadro{}, tui::QuadroDaLetra{}).empty());
}

TEST_CASE("a sequencia crua veste o fundo sómente na célulla da letra") {
  const tui::Quadro espectro = barras();
  const tui::QuadroDaLetra rio =
      tui::quadro_da_letra(kComVao, 10.0, kLargura, kAltura);
  const std::vector<tui::CelulaDoRio> tapete = tui::tapete_do_rio(espectro, rio);
  const std::size_t x = rio.linhas[0].collunha;
  const std::string da_letra = tui::sequencia_do_rio(tapete[kLeitura * kLargura + x]);
  // O fundo do painel escreve-se por SGR de papel 48, e antes da tinta.
  const tk::Triade tom = tk::rgb(tk::panel);
  CHECK(da_letra.find(tk::sgr(48, tom)) == 0);
  CHECK(da_letra.find(tk::tinta(tk::text_bright)) != std::string::npos);
  CHECK(da_letra.substr(da_letra.size() - 1) == "a");
  // A célulla da barra não escreve fundo algum.
  const std::string da_barra = tui::sequencia_do_rio(tapete[kLeitura * kLargura]);
  CHECK(da_barra.find(tk::sgr(48, tom)) == std::string::npos);
}

namespace {

// O rectangulo do espectro na TELA, escripto á mão: o canto em (84, 2) e a
// medida do rio de prova. Serve aos casos em que o que se afere é a DECISÃO; a
// geometria da sala de verdade prova-se em caso proprio, adeante.
const tui::Rectangulo kPainel = {84, 2, kLargura, kAltura};

// da_chapa — a ordem com o letreiro de pé, o foco dentro e o `l` a mostrar, que
// é o estado em que a chapa se põe. Os tres bools ficam soltos nos casos que
// provam justamente a falta de cada um d'elles.
tui::ChapaDaLetra da_chapa(const tui::QuadroDaLetra& rio) {
  return tui::ordem_da_chapa_da_letra(rio, kPainel, true, true, true);
}

}  // namespace

TEST_CASE("a chapa põe-se na linha de leitura e nunca antes nem depois") {
  // ANTES a linha ainda sobe e chapa alguma se põe; DEPOIS a chapa é a da
  // SEGUINTE, que é quem tomou a linha de leitura.
  CHECK_FALSE(da_chapa(tui::quadro_da_letra(kVersos, 8.0, kLargura, kAltura)).poe);
  const tui::ChapaDaLetra no_instante =
      da_chapa(tui::quadro_da_letra(kVersos, 10.0, kLargura, kAltura));
  CHECK(no_instante.poe);
  CHECK(no_instante.verso == "abcde");
  CHECK(no_instante.cellulas == 5);
  const tui::ChapaDaLetra depois =
      da_chapa(tui::quadro_da_letra(kVersos, 14.0, kLargura, kAltura));
  CHECK(depois.poe);
  CHECK(depois.verso == "fghij");
  // Faixa SEM letra alguma não tem chapa que pôr.
  CHECK_FALSE(da_chapa(tui::quadro_da_letra({}, 10.0, kLargura, kAltura)).poe);
}

TEST_CASE("sem letreiro ou sem foco ou com a letra escondida não ha chapa") {
  const tui::QuadroDaLetra canta =
      tui::quadro_da_letra(kVersos, 10.0, kLargura, kAltura);
  REQUIRE(da_chapa(canta).poe);
  CHECK_FALSE(tui::ordem_da_chapa_da_letra(canta, kPainel, false, true, true).poe);
  CHECK_FALSE(tui::ordem_da_chapa_da_letra(canta, kPainel, true, false, true).poe);
  CHECK_FALSE(tui::ordem_da_chapa_da_letra(canta, kPainel, true, true, false).poe);
  // E painel por pintar tambem não: rectangulo vazio não tem canto onde a pôr.
  CHECK_FALSE(tui::ordem_da_chapa_da_letra(canta, {}, true, true, true).poe);
}

TEST_CASE("a caixa da chapa sae em coordenadas da tela") {
  // A SALA de verdade, na tela d'elle. A 167 collunhas o painel fica com 83 e
  // começa na 84, e o verso de cinco glyphos centra-se na (83 menos 5) meio.
  const tui::Sala sala = tui::sala_da_tela(167, 45, false);
  const tui::Rectangulo espectro = tui::espectro_abaixo_da(sala, 0);
  REQUIRE(espectro.x == 84);
  REQUIRE(espectro.largura == 83);
  const tui::QuadroDaLetra rio =
      tui::quadro_da_letra(kVersos, 10.0, espectro.largura, espectro.altura);
  const tui::ChapaDaLetra ordem =
      tui::ordem_da_chapa_da_letra(rio, espectro, true, true, true);
  REQUIRE(ordem.poe);
  CHECK(ordem.collunha == 84 + 39);
  CHECK(ordem.linha == static_cast<int>(espectro.y + espectro.altura / 3));
  CHECK(ordem.cellulas == 5);
}

TEST_CASE("o verso comprido corta-se e é o cortado que se rasteriza") {
  const std::vector<nu::LinhaDaLetra> comprido = {
      {10.0, "um verso muito mais comprido que o painel"}};
  const tui::ChapaDaLetra ordem =
      da_chapa(tui::quadro_da_letra(comprido, 10.0, kLargura, kAltura));
  REQUIRE(ordem.poe);
  CHECK(ordem.verso == "um verso muito mais…");
  CHECK(ordem.cellulas == kLargura);
  // E o pedido veste a chapa das MESMAS côres com que o verso em mono se pinta:
  // o LARANJA da paleta (issue #157), que foi o que elle pediu, sobre o fundo
  // do painel. E vae em TRES fileiras, que é o corpo grande.
  const nu::PedidoDaChapa pedido =
      tui::pedido_da_chapa_da_letra(ordem.verso, ordem.cellulas);
  CHECK(pedido.texto == "um verso muito mais…");
  CHECK(pedido.tinta == std::string(tk::data3));
  CHECK(pedido.fundo == std::string(tk::panel));
  CHECK(pedido.linhas == tui::FILEIRAS_DO_VERSO);
  CHECK(pedido.corpo == nu::corpo_da_altura(tui::FILEIRAS_DO_VERSO));
  CHECK(pedido.cellulas == kLargura);
  CHECK(pedido.familia == std::string(nu::FAMILIA_DA_MARCA));
}

TEST_CASE("a chapa da proxima adianta-se assim que ella nasce na base") {
  // Aos onze segundos a primeira canta e a segunda já assomou na base, que o
  // intervallo entre as duas é de quatro segundos.
  const tui::QuadroDaLetra aos_onze =
      tui::quadro_da_letra(kVersos, 11.0, kLargura, kAltura);
  const tui::ChapaDaLetra ordem = da_chapa(aos_onze);
  REQUIRE(ordem.poe);
  CHECK(ordem.verso == "abcde");
  CHECK(ordem.adiantado == "fghij");
  CHECK(ordem.cellulas_adiantadas == 5);
  // O que se adianta é o verso INTEIRO, e não o embaralho do instante: aos onze
  // segundos a segunda linha ainda vem sem fórma, e imagem de glyphos
  // embaralhados seria chapa por deitar fóra no instante em que ella chegasse.
  const tui::LinhaViva* sobe = tui::linha_que_sobe_do_rio(aos_onze);
  REQUIRE(sobe != nullptr);
  CHECK(sobe->qual == 1);
  CHECK(sobe->verso == "fghij");
  CHECK(sobe->resolvida < 1.0);
  // Cantada a ultima linha, não ha mais nada que adiantar.
  const tui::QuadroDaLetra na_ultima =
      tui::quadro_da_letra(kVersos, 14.0, kLargura, kAltura);
  CHECK(tui::linha_que_sobe_do_rio(na_ultima) == nullptr);
  CHECK(da_chapa(na_ultima).adiantado.empty());
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
