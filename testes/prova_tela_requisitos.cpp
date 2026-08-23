// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA TELA — testes/prova_tela_requisitos.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a tela dos requisitos SEM abrir terminal algum: pinta-se o quadro num
// écran de papel, de largura escolhida, e afere-se o que elle diz.
//
// É d'isto que a tela de erro não apodrece. Tela julgada pelo olho de quem a
// abriu prova-se uma vez, no dia em que se escreveu, e nunca mais; pintada em
// écran de papel, a asserção corre em cada ctest.
//
// DOMÍNIO ......... relatorios de dublê, e larguras de écran escolhidas aqui.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... nenhum caso abre terminal, nem consulta o systema, nem lê
//                   ambiente. A largura é parametro, e não a do terminal.
// Q.E.D. .......... afere-se em quarenta collunas a mesma tela que se afere em
//                   cem; donde a tela estreita deixa de ser esperança.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "nucleo/sonda.hpp"
#include "tui/tela_requisitos.hpp"
#include "tui/tokens.hpp"

namespace nu = mysong::nucleo;
namespace tl = mysong::tui;
namespace tk = mysong::tui::tokens;

