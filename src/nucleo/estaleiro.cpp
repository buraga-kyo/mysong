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

std::string texto_do_andamento(const Andamento& andamento) {
  // Estaleiro que nunca trabalhou não tem recado: cadeia vazia, e a tela cala-se.
  // Dizer «0 a baixar» seria occupar a linha da trilha com nada.
  if (andamento.em_curso == 0 && andamento.na_espera == 0 &&
      andamento.colhidas == 0 && andamento.falhadas == 0)
    return {};
  std::string dito;
  if (andamento.em_curso > 0)
    junta(&dito, std::to_string(andamento.em_curso) + " a baixar");
  if (andamento.na_espera > 0)
    junta(&dito, std::to_string(andamento.na_espera) + " na espera");
  if (andamento.colhidas > 0) junta(&dito, plural(andamento.colhidas, "colhida"));
  if (andamento.falhadas > 0) junta(&dito, plural(andamento.falhadas, "falhada"));
  if (!andamento.ultima.empty()) dito += " (" + andamento.ultima + ")";
  return dito;
}

Estaleiro::Estaleiro(std::size_t obreiros, Obra obra) : obra_(std::move(obra)) {
  // Zero obreiro seria estaleiro que aceita encommenda e nunca a cumpre, que é
  // pior que erro: é silencio. Um, pelo menos.
  const std::size_t quantos = obreiros == 0 ? 1 : obreiros;
  obreiros_.reserve(quantos);
  for (std::size_t i = 0; i < quantos; ++i)
    obreiros_.emplace_back(&Estaleiro::obreiro, this);
}

Estaleiro::~Estaleiro() { fecha(); }

void Estaleiro::fecha() {
  {
    std::lock_guard<std::mutex> chave(tranca_);
    if (fechado_) return;  // fechado duas vezes: a segunda não junta os fios outra vez
    fechado_ = true;
    // A ESPERA abandona-se. Esperar por ella faria sahir do programa depender de
    // quantas baixas o operador encommendou, e ninguem espera meia hora para fechar
    // uma tela. A obra EM VOO espera-se, que essa já escreve no disco.
    espera_.clear();
  }
  sino_.notify_all();
  for (std::thread& fio : obreiros_)
    if (fio.joinable()) fio.join();
  obreiros_.clear();
}
}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
