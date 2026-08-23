// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DE SOM — exemplos/toca_tom.cpp
// ══════════════════════════════════════════════════════════════════════════
// Toca som de VERDADE, e é a metade do aceite que o motor dublê não pode
// provar. A bateria prova a mechanica; este binario prova o contracto com a
// libmpv e com o PipeWire, que é cousa differente e não se deduz d'aquella.
//
// DOMÍNIO ......... caminhos de arquivos de audio, na linha de commando, na
//                   ordem em que hão de formar a fila.
// CONTRA-DOMÍNIO .. um relatorio na sahida padrão, linha por linha, e o
//                   status: zero se tudo correu, e não zero se algo recusou.
// INVARIANTE ...... nenhuma linha d'este programa toca no volume do systema.
//                   O que aqui se move é o volume do mpv, e a prova d'isso se
//                   faz de fóra, medindo o do systema antes e depois.
// Q.E.D. .......... o relatorio traz, a cada batida, a posição, a duração, o
//                   estado E o playlist-count do mpv. O ultimo é a testemunha
//                   de que a fila é nossa: se ficar em uma entrada toda a
//                   corrida, o mpv nunca soube que havia fila.
// ══════════════════════════════════════════════════════════════════════════
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

#include "nucleo/tocador.hpp"

namespace {

namespace nu = mysong::nucleo;

void dorme(int millesimos) {
  std::this_thread::sleep_for(std::chrono::milliseconds(millesimos));
}

// Bate o relogio durante o tempo pedido, e imprime o que se lê a cada batida.
// Devolve a ultima posição vista, para que o chamador a possa comparar.
double relogio(nu::Tocador& tocador, nu::MotorMpv& motor, int millesimos,
               const char* rotulo) {
  double ultima = 0.0;
  for (int passado = 0; passado < millesimos; passado += 250) {
    tocador.pulsa();
    ultima = tocador.posicao();
    std::printf("  %-9s pos=%6.2f dur=%6.2f estado=%-7s playlist-count=%s\n",
                rotulo, ultima, tocador.duracao(),
                std::string(nu::nome_do_estado(tocador.estado())).c_str(),
                motor.propriedade("playlist-count").c_str());
    dorme(250);
  }
  return ultima;
}

}  // namespace

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
