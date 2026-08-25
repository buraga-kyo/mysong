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

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
