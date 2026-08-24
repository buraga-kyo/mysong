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

// ESCAPA a cadeia crua no corpo de uma cadeia JSON (SEM as aspas de fóra). O
// UTF-8 passa INTACTO, byte a byte: o JSON o admitte cru, e transcrevê-lo em
// \u seria inflar a mensagem para nada. O que NÃO passa é byte de controle,
// que vae em \u00XX; e no meio d'elles está a mudança de linha, que é a que
// partiria o enquadramento de uma-mensagem-por-linha.
inline std::string escapa(std::string_view cru) {
  std::string obra;
  obra.reserve(cru.size() + 8);
  for (const char bruto : cru) {
    const unsigned char byte = static_cast<unsigned char>(bruto);
    switch (byte) {
      case '"':  obra += "\\\""; continue;
      case '\\': obra += "\\\\"; continue;
      case '\b': obra += "\\b";  continue;
      case '\f': obra += "\\f";  continue;
      case '\n': obra += "\\n";  continue;
      case '\r': obra += "\\r";  continue;
      case '\t': obra += "\\t";  continue;
      default: break;
    }
    if (byte < 0x20u) {
      char cifra[7];
      std::snprintf(cifra, sizeof(cifra), "\\u%04x", static_cast<unsigned>(byte));
      obra += cifra;
      continue;
    }
    obra += bruto;
  }
  return obra;
}

}  // namespace mysong::api

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
