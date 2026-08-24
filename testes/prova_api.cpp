// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DA API, BANDA PURA — testes/prova_api.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o jsonzinho e o protocolo em MACHINA SURDA: sem socket, sem barramento,
// sem placa de som e sem arquivo em disco. É onde treze verbos e todo o
// enquadramento se provam de graça; o que de facto precisa de socket mora em
// prova_api_viva.cpp, e são cinco casos, não trinta.
//
// DOMÍNIO ......... cadeias, e sómente cadeias: nomes de faixa que trazem o que
//                   o systema de arquivos permitte, e linhas de mensagem que
//                   podem ser qualquer cousa, inclusive lixo.
// CONTRA-DOMÍNIO .. veredicto do doctest, e por elle o status do ctest.
// INVARIANTE ...... toda resposta d'esta obra é UMA linha. É o que se assere por
//                   find('\n') == npos, e não por inspecção de olho: o
//                   enquadramento de uma-mensagem-por-linha cae inteiro se um
//                   nome de faixa levar mudança de linha crua ao emissor.
// Q.E.D. .......... o dublê aqui NÃO é mais simples que o mundo nas tres cousas
//                   em que o mundo morde: nome de arquivo com aspas e UTF-8, fim
//                   natural da faixa mudando posição E estado na mesma batida, e
//                   ordem que o nucleo recusa. Dublê mais simples que o mundo é
//                   onde o defeito se aloja, e por isso se enumeram as
//                   differenças em vez de se as presumir ausentes.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>

#include "api/jsonzinho.hpp"

namespace {
using mysong::api::analysa;
using mysong::api::escapa;
using mysong::api::Mensagem;
using mysong::api::texto;
using mysong::api::Typo;
}  // namespace

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
