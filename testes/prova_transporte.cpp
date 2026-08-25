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

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
