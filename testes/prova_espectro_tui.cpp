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
