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

const char* nome_do_aviso(nu::Aviso aviso) {
  switch (aviso) {
    case nu::Aviso::FaixaMudou: return "faixa-mudou";
    case nu::Aviso::EstadoMudou: return "estado-mudou";
    case nu::Aviso::PosicaoAndou: return "posicao-andou";
    case nu::Aviso::FalhouAoTocar: return "falhou-ao-tocar";
  }
  return "?";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "uso: toca_tom <faixa> [faixa...]\n");
    return 2;
  }

  std::string razao;
  auto talvez = nu::MotorMpv::abrir(&razao);
  if (!talvez) {
    std::fprintf(stderr, "não abri a libmpv: %s\n", razao.c_str());
    return 1;
  }
  nu::MotorMpv& motor = *talvez;
  std::printf("libmpv interface=%lu\n", nu::MotorMpv::versao_da_interface());

  nu::Tocador tocador(motor);
  tocador.escuta([](const nu::Evento& evento) {
    std::printf("    « %-15s estado=%-7s faixa=%s pos=%.2f %s\n",
                nome_do_aviso(evento.aviso),
                std::string(nu::nome_do_estado(evento.estado)).c_str(),
                evento.faixa.c_str(), evento.posicao, evento.razao.c_str());
  });
  for (int i = 1; i < argc; ++i) tocador.fila().junta(argv[i]);
  std::printf("fila com %zu faixa(s)\n", tocador.fila().tamanho());

  if (!tocador.tocar_corrente()) {
    std::fprintf(stderr, "a primeira faixa NÃO tocou\n");
    return 3;
  }
  std::printf("ao=%s duracao=%s\n", motor.propriedade("current-ao").c_str(),
              motor.propriedade("duration").c_str());

  std::printf("[C2] o relogio anda\n");
  const double antes_da_pausa = relogio(tocador, motor, 1500, "tocando");

  std::printf("[C3] pausar congela o relogio\n");
  if (!tocador.pausar()) {
    std::fprintf(stderr, "pausar recusou\n");
    return 4;
  }
  tocador.pulsa();
  const double na_pausa = tocador.posicao();
  dorme(1200);
  tocador.pulsa();
  std::printf("  pausado pos=%.2f e 1,2s depois pos=%.2f estado=%s\n", na_pausa,
              tocador.posicao(),
              std::string(nu::nome_do_estado(tocador.estado())).c_str());

  std::printf("[C4] retomar destrava o relogio\n");
  if (!tocador.retomar()) {
    std::fprintf(stderr, "retomar recusou\n");
    return 5;
  }
  const double depois = relogio(tocador, motor, 1000, "retomado");
  std::printf("  resumo do relogio: antes=%.2f pausa=%.2f depois=%.2f\n",
              antes_da_pausa, na_pausa, depois);

  return 0;
}


// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
