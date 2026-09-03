// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO TRANSPORTE — testes/prova_transporte.cpp
// ══════════════════════════════════════════════════════════════════════════
// A lição d'esta bateria: alvo ESCRIPTO Á MÃO, e nunca calculado pela mesma
// conta que a obra faz. Assertiva que compara o valor com a constante que o
// produziu não pode falhar, e esta Casa já a apanhou cinco vezes.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cmath>
#include <limits>
#include <string>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "tui/transporte.hpp"

namespace tui = mysong::tui;
namespace nu = mysong::nucleo;

TEST_CASE("o tempo sahe em MM:SS, e o que não é tempo sahe em traço") {
  CHECK(tui::mm_ss(0.0) == "00:00");
  CHECK(tui::mm_ss(1.0) == "00:01");
  CHECK(tui::mm_ss(59.0) == "00:59");
  CHECK(tui::mm_ss(60.0) == "01:00");
  CHECK(tui::mm_ss(125.0) == "02:05");
  CHECK(tui::mm_ss(3599.0) == "59:59");
  CHECK(tui::mm_ss(3600.0) == "60:00");
  CHECK(tui::mm_ss(7325.0) == "122:05");
  // Trunca, e não arredonda: quem lê 02:05 ouviu esse segundo.
  CHECK(tui::mm_ss(125.999) == "02:05");
  CHECK(tui::mm_ss(-1.0) == "--:--");
  CHECK(tui::mm_ss(std::numeric_limits<double>::quiet_NaN()) == "--:--");
  CHECK(tui::mm_ss(std::numeric_limits<double>::infinity()) == "--:--");
}

// O enchimento. A meia collunha é o caso que importa, porque é onde `round` e
// `floor` differem: com dez collunhas e razão de quinze centesimos, `round` dá
// DUAS e `floor` dá UMA. Os alvos aqui são os de `round`, escriptos á mão.
TEST_CASE("o enchimento arredonda ao mais proximo, e cinge-se em um") {
  CHECK(tui::enchimento(0.0, 100.0, 10) == 0u);
  CHECK(tui::enchimento(4.9, 100.0, 10) == 0u);   // 0,49 → 0
  CHECK(tui::enchimento(5.0, 100.0, 10) == 1u);   // 0,50 → 1
  CHECK(tui::enchimento(14.0, 100.0, 10) == 1u);  // 1,40 → 1
  CHECK(tui::enchimento(15.0, 100.0, 10) == 2u);  // 1,50 → 2
  CHECK(tui::enchimento(50.0, 100.0, 10) == 5u);
  CHECK(tui::enchimento(100.0, 100.0, 10) == 10u);
  // Passar do fim cinge-se, e não transborda.
  CHECK(tui::enchimento(200.0, 100.0, 10) == 10u);
  // Duração que não presta dá zero, e nenhuma divisão acontece.
  CHECK(tui::enchimento(5.0, 0.0, 10) == 0u);
  CHECK(tui::enchimento(5.0, -1.0, 10) == 0u);
  CHECK(tui::enchimento(5.0, std::numeric_limits<double>::quiet_NaN(), 10) == 0u);
  // Posição que não é numero dá barra VAZIA, e não barra cheia: barra cheia
  // seria uma affirmação sobre onde o som está, e a Casa não o sabe.
  CHECK(tui::enchimento(std::numeric_limits<double>::infinity(), 100.0, 10) == 0u);
  CHECK(tui::enchimento(std::numeric_limits<double>::quiet_NaN(), 100.0, 10) == 0u);
  CHECK(tui::enchimento(-1.0, 100.0, 10) == 0u);
  // Largura zero não pinta collunha alguma, e não divide por zero ao contrario.
  CHECK(tui::enchimento(50.0, 100.0, 0) == 0u);
  CHECK(tui::enchimento(50.0, 100.0, 1) == 1u);   // 0,5 → 1, na collunha unica
  CHECK(tui::enchimento(49.0, 100.0, 1) == 0u);
}

// A barra fecha a largura EXACTA, e banda alguma sobra ou falta. Conta-se em
// CODEPOINTS, que o bloco é multibyte e contar bytes daria tres vezes mais.
TEST_CASE("a barra fecha a largura exacta, de uma a duzentas collunhas") {
  for (std::size_t largura = 1; largura <= 200; ++largura) {
    for (int passo = 0; passo <= 10; ++passo) {
      const double posicao = static_cast<double>(passo) * 10.0;
      const std::string linha = tui::linha_da_barra(
          {nu::Estado::Tocando, posicao, 100.0, 100, "", 0, 1}, largura);
      std::size_t glifos = 0;
      for (const unsigned char byte : linha)
        if ((byte & 0xC0) != 0x80) ++glifos;
      REQUIRE(glifos == largura);
    }
  }
}

// Fila vazia: a tela ergue-se e não affirma cousa alguma sobre o som. É o estado
// em que o operador acha o programma quando o abre sem argumento.
TEST_CASE("fila vazia dá barra vazia e tempo em traço") {
  const tui::Retracto vazio;  // os valores por defeito, que é o que a tela vê
  CHECK(vazio.estado == nu::Estado::Parado);
  CHECK(tui::mm_ss(vazio.posicao) == "00:00");
  CHECK(tui::mm_ss(vazio.duracao) == "00:00");
  CHECK(tui::enchimento(vazio.posicao, vazio.duracao, 40) == 0u);
  const std::string linha = tui::linha_da_barra(vazio, 8);
  // Oito vazios, e nem um cheio: a barra não mente sobre progresso que não ha.
  CHECK(linha.find("\u2588") == std::string::npos);
}

// A linha inteira contra alvo ESCRIPTO Á MÃO. Dez collunhas, e trinta por cento
// de trinta sobre cem: tres cheias e sete vazias, nesta ordem. Comparar a linha
// com ella mesma não provaria nada, que funcção pura é egual a si por
// construcção; o que prova é a cadeia que esta mão escreveu.
TEST_CASE("a linha da barra sahe egual á cadeia escripta á mão") {
  const tui::Retracto retracto{nu::Estado::Tocando, 30.0, 100.0, 70, "x", 1, 4};
  CHECK(tui::linha_da_barra(retracto, 10) ==
        "\u2588\u2588\u2588\u2591\u2591\u2591\u2591\u2591\u2591\u2591");
  const tui::Retracto no_fim{nu::Estado::Tocando, 100.0, 100.0, 70, "x", 1, 4};
  CHECK(tui::linha_da_barra(no_fim, 4) == "\u2588\u2588\u2588\u2588");
  const tui::Retracto no_principio{nu::Estado::Parado, 0.0, 100.0, 70, "x", 0, 4};
  CHECK(tui::linha_da_barra(no_principio, 4) == "\u2591\u2591\u2591\u2591");
}
