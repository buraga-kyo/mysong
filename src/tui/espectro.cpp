// ══════════════════════════════════════════════════════════════════════════
//   O CORPO DO DESENHO DO ESPECTRO — src/tui/espectro.cpp
// ══════════════════════════════════════════════════════════════════════════
// A conta que o tractado promette. Nada aqui abre terminal, lê ambiente ou
// consulta relogio: d'onde toda affirmação d'este manuscripto se prova em
// machina surda, e a fita deixa de depender do olho de quem a abriu.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/espectro.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace mysong::tui {

namespace {

// cingido — a magnitude reduzida ao intervallo [0,1] que o contracto promette,
// e que esta Casa não confia. A ORDEM das duas guardas é o que importa: a
// finitude PRIMEIRO, porque toda comparação com NaN é falsa, e um cingir
// escripto na ordem natural (`m < 0 ? 0 : m > 1 ? 1 : m`) devolveria o NaN
// intacto ao floor, d'onde sahiria conta indefinida e indice fóra de limite.
float cingido(float magnitude) {
  if (!std::isfinite(magnitude)) return 0.0f;
  if (magnitude < 0.0f) return 0.0f;
  if (magnitude > 1.0f) return 1.0f;
  return magnitude;
}

}  // namespace

int oitavos(float magnitude, std::size_t altura) {
  const int teto = static_cast<int>(altura) * DEGRAUS_POR_CELULA;
  if (teto <= 0) return 0;
  // O floor, e não o arredondamento: enche-se o degrau que a magnitude JÁ
  // conquistou, e não o que ella quasi conquistou. D'onde a magnitude cheia dá o
  // teto EXACTO (floor de 1 * teto é teto) e o silencio dá zero exacto, que são
  // os dous extremos que o aceite cobra por nome.
  const float degraus = cingido(magnitude) * static_cast<float>(teto);
  const int conquistados = static_cast<int>(std::floor(degraus));
  return conquistados < 0 ? 0 : (conquistados > teto ? teto : conquistados);
}
