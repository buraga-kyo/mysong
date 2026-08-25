// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ESTALEIRO — src/nucleo/estaleiro.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. Uma tranca guarda TUDO o que os obreiros vêem, e nada d'esse
// estado se lê fóra d'ella: contador lido sem tranca é contador que a machina
// pode ter meio escripto.
//
// DOMÍNIO ......... as encommendas, e a obra que as cumpre.
// CONTRA-DOMÍNIO .. o andamento, e os arquivos que a obra deixou no disco.
// INVARIANTE ...... o obreiro incrementa em_curso_ DENTRO da tranca e ANTES de
//                   chamar a obra, e decrementa-o depois: donde o pico é o pico
//                   de verdade, e não uma amostra colhida no intervallo.
// Q.E.D. .......... fechando-se, a espera ABANDONA-SE e sómente as obras em voo
//                   se esperam: assim sahir do programa não fica pendurado n'uma
//                   fila de vinte baixas que ninguem mais vae ver.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/estaleiro.hpp"

#include <utility>

namespace mysong::nucleo {

namespace {

// junta — acrescenta um pedaço á lista, com a virgula sómente quando ha o que
// separar. Existe para que a virgula não appareça no principio da linha.
void junta(std::string* dito, const std::string& pedaco) {
  if (!dito->empty()) *dito += ", ";
  *dito += pedaco;
}

// plural — «uma colhida», «duas colhidas». A concordancia faz parte do recado.
std::string plural(std::size_t quantas, const std::string& singular) {
  return std::to_string(quantas) + " " + singular + (quantas == 1 ? "" : "s");
}

}  // namespace
}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
