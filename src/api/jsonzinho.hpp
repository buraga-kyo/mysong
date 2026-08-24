// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO JSONZINHO — src/api/jsonzinho.hpp
// ══════════════════════════════════════════════════════════════════════════
// O JSON d'esta Casa, e SÓ o d'esta Casa. Não é bibliotheca geral: é o
// subconjunto PLANO que o protocolo do socket usa — um objecto de UM nivel, com
// valores escalares — e mais nada. Declara-se só-cabeçalho, á maneira do
// aparar_volume que mora no tractado do motor, para que unidade alguma se
// compile por causa d'elle.
//
// DOMÍNIO ......... de um lado, cadeias cruas do systema de arquivos, que
//                   trazem aspas, contra-barra, mudança de linha e UTF-8 de
//                   mais de um byte; do outro, uma linha que chegou pelo socket
//                   e que pode ser qualquer cousa, inclusive lixo.
// CONTRA-DOMÍNIO .. JSON valido de UMA linha, e uma Mensagem que ou é valida ou
//                   traz a razão por que não é.
// INVARIANTE ...... o escape NUNCA deixa passar byte que parta o enquadramento.
//                   Uma mudança de linha crua no nome de uma faixa partiria a
//                   mensagem em duas, e o cliente do outro lado leria metade de
//                   uma e metade da seguinte: é este o defeito que o escape
//                   existe para impedir, e não a elegancia.
// Q.E.D. .......... o parser é ESTRICTO de proposito: rejeita tudo o que sae do
//                   subconjunto. Rejeitar não é falhar — «json_malformado» é
//                   resposta legitima e prevista, e parser permissivo é que
//                   seria o risco, porque acceitaria por adivinhação o que o
//                   contracto não promette. Sendo ambas as bandas funcções
//                   puras de cadeia para cadeia, provam-se sem socket algum.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace mysong::api {

// ESCAPA a cadeia crua no corpo de uma cadeia JSON (SEM as aspas de fóra).
inline std::string escapa(std::string_view cru);

}  // namespace mysong::api

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
