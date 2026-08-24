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
