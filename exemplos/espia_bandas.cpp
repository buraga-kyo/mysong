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

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "uso: espia_bandas <segundos> [faixa...]\n");
    return 2;
  }
  const double segundos = std::atof(argv[1]);

  std::string razao;
  auto talvez = nu::MotorMpv::abrir(&razao);
  if (!talvez) {
    std::fprintf(stderr, "não abri a libmpv: %s\n", razao.c_str());
    return 1;
  }
  nu::MotorMpv& motor = *talvez;
  nu::Tocador tocador(motor);

  nu::Analisador analisador;
  tocador.observa(analisador);
  std::printf("analisador vivo=%d razao=%s\n", analisador.vivo() ? 1 : 0,
              analisador.razao().c_str());

  // O MAPPA das bandas, impresso antes de tudo: é por elle que a prova sabe qual
  // columna olhar quando o intruso de 6000 Hz disparar. Pergunta-se á obra, e
  // não se refaz a conta, pela mesma razão que a bateria não a refaz.
  const nu::Espectro mappa(48000.0f, 2);
  std::printf("mappa das bandas: 100Hz=%zu 440Hz=%zu 1000Hz=%zu 6000Hz=%zu\n",
              mappa.banda_de(100.0f), mappa.banda_de(440.0f),
              mappa.banda_de(1000.0f), mappa.banda_de(6000.0f));

  for (int i = 2; i < argc; ++i) tocador.junta(argv[i]);
  std::printf("fila com %zu faixa(s)\n", tocador.retracto().tamanho);
  if (tocador.retracto().tamanho > 0 && !tocador.tocar_corrente()) {
    std::fprintf(stderr, "a primeira faixa NÃO tocou\n");
    return 3;
  }

  // A CADENCIA: quarenta millesimos, que é folgadamente mais rapido que o prazo
  // do silencio (cento e vinte) e mais devagar que o quadro do espectro (vinte e
  // um). Assim o relogio de guarda bate a tempo de apanhar o nó que morreu, e a
  // linha impressa nunca é a mesma duas vezes por descuido de cadencia.
  const auto inicio = Relogio::now();
  double passado = 0.0;
  double acabou_em = -1.0;  // negativo é «a faixa não acabou ainda»
  while (passado < segundos) {
    // O carimbo se toma AGORA, e não ao fim da volta passada: entre uma cousa e
    // outra ha uma pausa, e no caminho da reconexão ha uma de mil e duzentos
    // millesimos. Carimbo velho poria a queda inteira dentro de um quadro só, e
    // faria a barra parecer saltar a zero quando ella de facto esmoreceu.
    passado = std::chrono::duration<double>(Relogio::now() - inicio).count();
    tocador.pulsa();
    const auto bandas = tocador.bandas();
    std::printf("t=%6.2f no=%-6llu estado=%-7s", passado, analisador.no(),
                std::string(nu::nome_do_estado(tocador.estado())).c_str());
    imprime_bandas(bandas);
    std::printf("\n");
    std::fflush(stdout);  // linha a linha: a prova lê isto por tubo, ao vivo
    dorme(40);

    // A RECONEXÃO, provada n'esta mesma corrida. O nó do mpv morre com a faixa:
    // ao fim d'ella o mpv cahe em espera e o nó sahe do grafo, e é essa a morte
    // que o aceite manda provar. Vendo o estado deixar de tocar, torna-se a
    // tocar depois de uma pausa, e o serial impresso na columna «no» ha de ser
    // OUTRO: nó novo, preso sozinho, sem que esta corrida se reiniciasse.
    const bool toca = tocador.estado() == nu::Estado::Tocando;
    if (!toca && tocador.retracto().tamanho > 0 && passado > 1.0 &&
        acabou_em < 0.0) {
      acabou_em = passado;
      std::printf("a faixa acabou em t=%.2f: as bandas hão de descer a zero\n", acabou_em);
    }
    // Espera-se IMPRIMINDO, e não dormindo. Dormir aqui engoliria justamente os
    // quadros da queda, que são os que o aceite manda ver; a espera passa a ser
    // contada no proprio relogio da corrida.
    if (acabou_em >= 0.0 && passado - acabou_em > 1.5 && passado + 2.0 < segundos) {
      std::printf("torno a tocar em t=%.2f\n", passado);
      tocador.tocar_corrente();
      acabou_em = -1.0;
    }
  }
  std::printf("corrida completa\n");
  return 0;
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
