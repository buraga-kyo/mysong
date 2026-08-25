// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROJECTOR — src/nucleo/video.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. Não ha shell em logar algum: o `execvp` recebe o vector tal e
// qual, donde faixa chamada `; rm -rf ~` é nome de arquivo e não commando.
//
// DOMÍNIO ......... a faixa, e as ordens do operador.
// CONTRA-DOMÍNIO .. o processo da janella, e as linhas que lhe vão pelo soquete.
// INVARIANTE ...... fita alguma sobrevive ao projector: o destructor fecha. E
//                   fechar ESPERA o filho, donde processo algum fica orfão.
//                   Todo punho publico toma a TRANCA: este objecto é chamado do fio
//                   da tela e do fio do relogio, e `waitpid` chamado dos dous ao
//                   mesmo tempo daria um a colher o filho e outro a não o achar.
// Q.E.D. .......... abrir a segunda fita fecha a primeira, donde nunca ha duas
//                   janellas, e o audio nunca dobra por duas fitas.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/video.hpp"

#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>
#include <utility>

namespace mysong::nucleo {
namespace {

// As extensões que costumam trazer video. Lista FECHADA: extensão que não está
// aqui não abre janella, e é melhor recusar de mais que abrir janella vazia.
constexpr const char* kExtensoesComVideo[] = {
    ".mkv", ".mp4", ".webm", ".avi", ".mov", ".m4v", ".ogv", ".flv", ".wmv",
    ".mpg", ".mpeg", ".ts",
};

// O PRAZO da espera pelo soquete, e o passo d'ella. O mpv cria o soquete depois
// de subir, e subir leva tempo de disco: tentar uma vez e desistir daria «sem
// soquete» na machina carregada, que é falha inventada.
constexpr int kTentativasDoSoquete = 60;
constexpr int kMilesimosPorTentativa = 25;

std::string minuscula(std::string_view crua) {
  std::string baixa;
  baixa.reserve(crua.size());
  for (const unsigned char letra : crua)
    baixa += static_cast<char>(letra >= 'A' && letra <= 'Z' ? letra + 32
                                                            : letra);
  return baixa;
}

// abre_soquete — o punho ligado ao soquete do mpv, e menos um não havendo. Tenta
// mais de uma vez, com pausa: o soquete nasce depois do processo.
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
