// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA FITA — testes/prova_arrowline.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a taboada chromatica e a fita arrowline. Nenhum caso abre terminal
// nem janella: a fita compõe-se em PEDAÇOS, e é por elles que se prova.
//
// DOMÍNIO ......... a taboada de tokens e as fitas que aqui se armam.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... o que se afirma é a REGRA do systema de desenho, não a
//                   apparencia: contagem de junções, herança de côr, e a
//                   proscripção do losango de duas pontas.
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

TEST_CASE("a seta é de uma só direcção, e nunca losango de duas pontas") {
  for (const al::Pedaco& pedaco : fita_de(4).compor())
    CHECK(pedaco.texto.find(al::kPontaEsquerda) == std::string::npos);
  const auto esquerda = fita_de(4, al::Sentido::Esquerda).compor();
  for (const al::Pedaco& pedaco : esquerda)
    CHECK(pedaco.texto.find(al::kPontaDextra) == std::string::npos);
  // O losango proscripto seria a ponta esquerda encostada na dextra.
  for (std::size_t i = 1; i < esquerda.size(); ++i)
    CHECK_FALSE(esquerda[i - 1].texto == al::kPontaEsquerda &&
                esquerda[i].texto == al::kPontaDextra);
}
