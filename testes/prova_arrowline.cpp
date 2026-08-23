// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA FITA — testes/prova_arrowline.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a taboada chromatica e a fita arrowline. Nenhum caso abre terminal
// nem janella: a fita compõe-se em PEDAÇOS, e é por elles que se prova.
//
// DOMÍNIO ......... a taboada de tokens e as fitas que aqui se armam.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... o que se afirma é a REGRA do systema de desenho, não a
//                   apparencia: a contagem das junções, e a herança da côr nos
//                   DOUS sentidos. Do losango de duas pontas não se lavra
//                   asserção de ausencia, que sahiria vazia; a garantia é
//                   ESTRUCTURAL, e o que se prova é a unicidade do glifo que a
//                   sustenta.
// Q.E.D. .......... provado o par tinta/fundo de cada junção, a continuidade
//                   da fita deixa de depender do olho de quem a lê. O que o
//                   olho ainda deve julgar — se a fonte resolve o glifo, se o
//                   rasterizador deixa filete — vae dito por extenso na SPEC,
//                   e nenhuma d'estas provas o allega.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
#include <string>
#include <vector>

#include "tui/arrowline.hpp"
#include "tui/tokens.hpp"

namespace tk = mysong::tui::tokens;
namespace al = mysong::tui;

namespace {

// Arma uma fita de N segmentos com fundos deliberadamente DISTINCTOS entre si,
// de sorte que uma troca de côr não passe por coincidencia.
al::Fita fita_de(std::size_t quantos, al::Sentido sentido = al::Sentido::Dextra) {
  static const std::string_view cores[] = {tk::v700, tk::data2, tk::glow_hot,
                                           tk::v500, tk::data5, tk::v900,
                                           tk::data3, tk::ok};
  al::Fita fita(sentido);
  for (std::size_t i = 0; i < quantos; ++i)
    fita.junta({"S" + std::to_string(i), cores[i % 8], tk::text_bright});
  return fita;
}

// Conta as junções INTERNAS: as de encaixe que não são o remate de cauda.
std::size_t internas(const std::vector<al::Pedaco>& pedacos) {
  std::size_t quantas = 0;
  for (const al::Pedaco& pedaco : pedacos)
    if (pedaco.juncao && !pedaco.cauda) ++quantas;
  return quantas;
}

}  // namespace

TEST_CASE("a taboada conserva as côres que a fonte decretou") {
  CHECK(tk::base == "#0c0617");
  CHECK(tk::panel == "#130a24");
  CHECK(tk::inset == "#0a0514");
  CHECK(tk::v500 == "#8b5cf6");   // o acento cardeal
  CHECK(tk::v700 == "#6d28d9");
  CHECK(tk::glow_hot == "#ff2fa0");
  CHECK(tk::text_bright == "#e9dcff");
  CHECK(tk::text_muted == "#6f5a96");
  CHECK(tk::data1 == "#a855f7");
  CHECK(tk::data6 == "#ff3d3d");
}

TEST_CASE("a côr decompõe-se em tríade, e a opacidade resolve-se opaca") {
  const auto cardeal = tk::rgb(tk::v500);
  CHECK(cardeal.r == 139);
  CHECK(cardeal.g == 92);
  CHECK(cardeal.b == 246);
  CHECK(tk::rgb("8b5cf6").r == 139);        // tolera-se a falta do cerquilho
  CHECK(tk::rgb("#8b5cf6ff").b == 246);     // e despoja-se o byte de opacidade
  CHECK(tk::alfa::bar == 0.8);
  // Meia mistura de branco sobre negro dá 128, que é o arredondamento certo.
  CHECK(tk::mistura("#ffffff", "#000000", 0.5).r == 128);
  CHECK(tk::mistura("#ffffff", "#000000", 2.0).r == 255);   // alfa confinado
  CHECK(tk::mistura("#ffffff", "#000000", -1.0).r == 0);
}

TEST_CASE("a fita de N segmentos emitte N menos um encaixes") {
  for (std::size_t quantos = 1; quantos <= 8; ++quantos) {
    const auto pedacos = fita_de(quantos).compor();
    CHECK(internas(pedacos) == quantos - 1);
    for (const al::Pedaco& pedaco : pedacos)
      if (pedaco.juncao) CHECK(pedaco.texto == al::kPontaDextra);
  }
}

TEST_CASE("a fita de um só segmento não tem encaixe algum") {
  const auto pedacos = fita_de(1).compor();
  CHECK(internas(pedacos) == 0);
  CHECK(pedacos.size() == 2);  // o rotulo, e o remate de cauda
}

TEST_CASE("a fita vazia não tem sequer remate") {
  CHECK(fita_de(0).compor().empty());
  CHECK(fita_de(0).largura_exigida() == 0);
}

TEST_CASE("rotulo vazio é segmento, e não segmento inexistente") {
  al::Fita fita;
  fita.junta({"", tk::v700}).junta({"", tk::data2}).junta({"", tk::v500});
  const auto pedacos = fita.compor();
  CHECK(internas(pedacos) == 2);
  CHECK(pedacos.front().texto.empty());
  CHECK(fita.largura_exigida() == 3);  // tres glifos, e rotulo algum
}

TEST_CASE("a côr do encaixe é a côr do segmento que elle segue") {
  const al::Fita fita = fita_de(5);
  const auto pedacos = fita.compor();
  const auto& segmentos = fita.segmentos();
  std::size_t qual = 0;
  for (const al::Pedaco& pedaco : pedacos) {
    if (!pedaco.juncao || pedaco.cauda) continue;
    CHECK(pedaco.tinta == segmentos[qual].fundo);      // a tinta vem de trás
    CHECK(pedaco.fundo == segmentos[qual + 1].fundo);  // a cama, da frente
    ++qual;
  }
  CHECK(qual == segmentos.size() - 1);
}

TEST_CASE("no sentido esquerda a côr do encaixe é a do segmento que elle abre") {
  // Espelho da regra (b), e provado nos VALORES: a fita arma-se com tres côres
  // escolhidas aqui, e o que se espera de cada encaixe vae escripto á mão,
  // d'ellas deduzido pela regra, e não colhido do que a obra devolveu.
  al::Fita fita(al::Sentido::Esquerda);
  fita.junta({"um", tk::v500}).junta({"dous", tk::data2}).junta({"tres", tk::glow_hot});
  const auto p = fita.compor();
  REQUIRE(p.size() == 6);  // o remate, tres rotulos, e dous encaixes internos
  // O remate encara o terminal, e n'este sentido vem PRIMEIRO, com a tinta do
  // segmento que elle abre, que é o primeiro de todos.
  CHECK(p[0].juncao);
  CHECK(p[0].cauda);
  CHECK(p[0].tinta == tk::v500);
  CHECK(p[0].fundo == tk::transparent);
  CHECK(p[1].texto == "um");
  // A ponta aponta á esquerda: logo pertence ao segmento que ella ABRE, á sua
  // direita, e assenta na cama do que fica atrás. Invertido o par, falha aqui.
  CHECK(p[2].juncao);
  CHECK_FALSE(p[2].cauda);
  CHECK(p[2].tinta == tk::data2);
  CHECK(p[2].fundo == tk::v500);
  CHECK(p[3].texto == "dous");
  CHECK(p[4].juncao);
  CHECK(p[4].tinta == tk::glow_hot);
  CHECK(p[4].fundo == tk::data2);
  CHECK(p[5].texto == "tres");
}

TEST_CASE("a seta é de uma só direcção: nenhum pedaço traz a ponta opposta") {
  for (const al::Pedaco& pedaco : fita_de(4).compor())
    CHECK(pedaco.texto.find(al::kPontaEsquerda) == std::string::npos);
  for (const al::Pedaco& pedaco : fita_de(4, al::Sentido::Esquerda).compor())
    CHECK(pedaco.texto.find(al::kPontaDextra) == std::string::npos);
}

TEST_CASE("o losango não se exprime: a fita tem um só glifo de encaixe") {
  // A garantia contra o losango é ESTRUCTURAL, e não asserção de ausencia: a
  // fita elege UM glifo ao nascer, pelo seu sentido, e serve-se d'elle em toda
  // junção, remate inclusive. Não havendo por onde entrar um segundo, as duas
  // pontas não se encostam. Prova-se pois a unicidade, que é o que a obra pode
  // perder num descuido — eleger o glifo por junção, e não por fita.
  //
  // Este caso é o GUARDA da eleição, e desde a #20 é guarda sem limite a
  // declarar: houve uma porta, Fita::glifo(), por onde se podia passar cadeia
  // com as duas pontas juntas, e a porta sahiu com o campo que a sustentava.
  // Mudada a expressão que elege a ponta pelo Sentido, é aqui que morre.
  for (const al::Sentido sentido : {al::Sentido::Dextra, al::Sentido::Esquerda}) {
    const std::string_view eleito =
        sentido == al::Sentido::Dextra ? al::kPontaDextra : al::kPontaEsquerda;
    std::size_t encaixes = 0;
    for (const al::Pedaco& pedaco : fita_de(5, sentido).compor()) {
      if (!pedaco.juncao) continue;
      CHECK(pedaco.texto == eleito);
      ++encaixes;
    }
    CHECK(encaixes == 5);  // quatro internas, e o remate
  }
}

TEST_CASE("os glifos da fita são os pontos de codigo que a Nerd Font tem") {
  CHECK(al::kPontaDextra.size() == 3);  // U+E0B0, em tres bytes de UTF-8
  CHECK(al::kPontaEsquerda.size() == 3);
  CHECK(al::kPontaDextra == "\xee\x82\xb0");
  CHECK(al::kPontaEsquerda == "\xee\x82\xb2");
}

TEST_CASE("o remate de cauda sahe em ponta, e fóra da conta do N menos um") {
  const auto pedacos = fita_de(3).compor();
  const al::Pedaco& remate = pedacos.back();
  CHECK(remate.juncao);
  CHECK(remate.cauda);
  CHECK(remate.tinta == tk::glow_hot);  // o fundo do terceiro segmento
  CHECK(remate.fundo == tk::transparent);
  CHECK(internas(pedacos) == 2);
  al::Fita rasa(al::Sentido::Dextra, false);
  rasa.junta({"um", tk::v700}).junta({"dous", tk::data2});
  const auto rasos = rasa.compor();
  CHECK_FALSE(rasos.back().juncao);  // sem cauda, remata em aresta reta
  CHECK(internas(rasos) == 1);
}

TEST_CASE("o enchimento fica um degrau rebaixado da orla") {
  CHECK(al::rebaixar(tk::v500) == tk::v700);
  CHECK(al::rebaixar(tk::v700) == tk::v900);
  CHECK(al::rebaixar(tk::v900) == tk::v975);         // no fundo da rampa satura
  CHECK(al::rebaixar(tk::v975) == tk::v975);
  CHECK(al::rebaixar(tk::glow_hot) == tk::glow_hot);  // fóra da rampa, intacta
}

TEST_CASE("a côr veste-se em sequencia SGR de truecolor") {
  CHECK(tk::tinta(tk::v500) == "\x1b[38;2;139;92;246m");
  CHECK(tk::fundo_de(tk::v500) == "\x1b[48;2;139;92;246m");
  CHECK(tk::repouso == "\x1b[0m");
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
