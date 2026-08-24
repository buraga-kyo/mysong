// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO SOCKET, LAVRA — src/api/socket.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o cabecalho. É a UNICA unidade da superfície que nomeia AF_UNIX, poll e
// descriptor; o protocolo, que é o cerebro, nada d'isso conhece, e é d'essa
// repartição que vem a bateria em machina surda.
//
// DOMÍNIO ......... descriptors, e o que o kernel disser d'elles.
// CONTRA-DOMÍNIO .. linhas que entram, linhas que sahem, e um arquivo de socket
//                   que existe enquanto o Servidor existe, e não mais.
// INVARIANTE ...... o errno guarda-se em variavel local ANTES de qualquer close
//                   ou umask, porque fechar descriptor pode sobrescrever o errno
//                   que se queria relatar, e razão composta do errno de outra
//                   chamada é razão que manda depurar no logar errado.
// Q.E.D. .......... nenhuma chamada d'esta unidade bloqueia. O poll espera ZERO, o
//                   socket nasce O_NONBLOCK, e o accept vem por accept4 com a
//                   mesma marca; logo uma batida custa o que o kernel tiver
//                   pronto, e cliente mudo não trava nem o tocador nem os outros.
// ══════════════════════════════════════════════════════════════════════════
#include "api/socket.hpp"

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <utility>

#include <poll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "api/protocolo.hpp"

namespace mysong::api {

std::string caminho_padrao_do_socket() {
  const char* raiz = std::getenv("XDG_RUNTIME_DIR");
  if (raiz == nullptr || *raiz == '\0') return {};
  return std::string(raiz) + "/mysong.sock";
}

}  // namespace mysong::api

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
