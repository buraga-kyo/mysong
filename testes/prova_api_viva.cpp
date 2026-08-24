// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DA API, BANDA VIVA — testes/prova_api_viva.cpp
// ══════════════════════════════════════════════════════════════════════════
// Aqui mora SÓ o que não se pode provar em machina surda: socket AF_UNIX de
// verdade, aberto no systema de arquivos, com cliente de verdade do outro lado. É
// pouco de proposito — os treze verbos e todo o enquadramento provam-se em
// prova_api.cpp —, e o que fica são as cinco cousas que só o transporte pode
// errar.
//
// DOMÍNIO ......... descriptors e arquivos no systema, que é estado GLOBAL.
// CONTRA-DOMÍNIO .. veredicto do doctest, e nenhum resto em disco.
// INVARIANTE ...... cada caso abre o seu socket num directorio temporario PROPRIO,
//                   feito por mkdtemp e desfeito por destructor. É o que impede
//                   dous casos de colidirem no mesmo caminho, e o que impede um
//                   caso de envenenar o seguinte com socket orphao. Sem isto, a
//                   ordem em que o doctest corre os casos passaria a importar, e
//                   bateria cuja ordem importa é bateria que falha em outra
//                   machina.
// Q.E.D. .......... o caminho do socket é INJECTAVEL na fabrica, e é essa unica
//                   decisão de desenho que torna esta bateria possivel: sem ella
//                   todo caso teria de disputar $XDG_RUNTIME_DIR/mysong.sock, e
//                   com o mysong do operador, se elle o tivesse aberto.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

#include <dirent.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "api/socket.hpp"

namespace {

using mysong::api::Servidor;
using mysong::nucleo::Estado;

}  // namespace

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
