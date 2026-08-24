// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ESPIA DAS BANDAS — exemplos/espia_bandas.cpp
// ══════════════════════════════════════════════════════════════════════════
// O INSTRUMENTO DO ACEITE. A bateria prova a mathematica em machina surda;
// este binario prova o que ella não alcança: que as bandas vêm do nó do NOSSO
// mpv, e de mais nada. Imprime linha por quadro, com carimbo de tempo, para
// que a prova da NEGATIVA se leia com o olho e se confira com o grep.
//
// DOMÍNIO ......... quantos segundos espiar, e as faixas a tocar. Zero faixas
//                   é caso legitimo: é como se prova que sem nó ha zeros.
// CONTRA-DOMÍNIO .. uma linha por quadro na sahida padrão, com o carimbo, o
//                   serial do nó a que nos prendemos, e as bandas em dous
//                   algarismos cada.
// INVARIANTE ...... este binario NÃO linka a janella: o espectro é do nucleo,
//                   e provar-se sem FTXUI é parte do aceite.
// Q.E.D. .......... dispara-se notify-send e um segundo tocador em 6000 Hz com
//                   isto a correr, e a banda de 6000 Hz não se move. Mata-se o
//                   mpv, e as bandas descem a zero sem que a corrida acabe;
//                   toca-se de novo, e ellas voltam sem reiniciar nada.
// ══════════════════════════════════════════════════════════════════════════
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include "nucleo/analisador.hpp"
#include "nucleo/espectro.hpp"
#include "nucleo/tocador.hpp"

namespace {

namespace nu = mysong::nucleo;
using Relogio = std::chrono::steady_clock;

void dorme(int millesimos) {
  std::this_thread::sleep_for(std::chrono::milliseconds(millesimos));
}

// As bandas em dous algarismos cada, de 00 a 99. Dous algarismos porque a linha
// ha de caber em oitenta columnas com o carimbo e o serial, e porque o que se
// afere n'esta prova é se a banda se MOVE, e não a sua terceira casa.
void imprime_bandas(const std::vector<float>& bandas) {
  for (const float valor : bandas) {
    std::printf(" %02d", static_cast<int>(valor * 99.0f + 0.5f));
  }
}

}  // namespace
