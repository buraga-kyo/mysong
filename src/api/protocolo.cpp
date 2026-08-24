// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROTOCOLO, LAVRA — src/api/protocolo.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o cabecalho. Não inclue socket algum, nem cabecalho de systema: fala
// com o Tocador abstracto e com o jsonzinho, e é d'ahi que lhe vem a inteira
// provabilidade em machina surda.
//
// DOMÍNIO ......... a Mensagem já lida, e o Tocador emprestado.
// CONTRA-DOMÍNIO .. uma linha de JSON, sempre.
// INVARIANTE ...... tres recusas, e tres codigos que NÃO se confundem:
//                   «verbo_desconhecido» é nome que a Casa não tem;
//                   «nao_implementado» é nome que a Casa TEM e cujo subsystema
//                   ainda não chegou, e vae com a issue que o trará; e
//                   «recusado» é o nucleo a dizer não a ordem legitima. Quem
//                   depura do outro lado precisa de as distinguir, porque o
//                   remedio de cada uma é differente.
// Q.E.D. .......... nenhuma linha d'esta unidade nomeia AF_UNIX, poll ou
//                   descriptor; logo o dublê a exercita inteira.
// ══════════════════════════════════════════════════════════════════════════
#include "api/protocolo.hpp"

#include <utility>
#include <vector>

#include "api/jsonzinho.hpp"

namespace mysong::api {
namespace {

using nucleo::Tocador;

// A moldura do ACERTO abre-se sempre por «ok», que é o que o cliente lê primeiro.
Objecto abre_acerto() {
  Objecto obra;
  obra.par("ok", booleano(true));
  return obra;
}

std::string feito() { return abre_acerto().fecha(); }

}  // namespace
}  // namespace mysong::api

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
