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

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
