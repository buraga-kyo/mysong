// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA BIBLIOTHECA — src/nucleo/biblioteca.hpp
// ══════════════════════════════════════════════════════════════════════════
// Guarda o ÍNDICE do acervo, e sómente elle: não abre arquivo de audio, não
// conhece taglib, não sabe o que é um directorio de artista. Quem varre o disco
// é a Varredura, que vive ao lado; esta peça sabe de SQLite, e mais nada.
//
// DOMÍNIO ......... um caminho de banco, que ENTRA POR PARÂMETRO, e faixas já
//                   apuradas por quem as apurou.
// CONTRA-DOMÍNIO .. as consultas por artista, por album e por faixa; e, do
//                   outro lado, a escripta de um índice novo.
// INVARIANTE ...... a escripta NUNCA toca o banco em uso. O Escriba lavra
//                   n'um temporario, com journal_mode=DELETE e
//                   synchronous=FULL, e sómente ao cabo renomeia atomicamente
//                   sobre o antigo: donde uma interrupção conserva a imagem
//                   anterior INTEIRA, e jamais uma imagem meia. É o precedente
//                   que o operador já usa em agenda_index.py, e a razão d'elle
//                   é que índice corrompido a meio de escripta é pior que
//                   índice ausente. A leitura abre em SÓMENTE-LEITURA, e banco
//                   ausente é resposta vazia e não erro. Nada sahe por
//                   excepção pela borda, e toda cadeia vai ao banco por
//                   amarração, nunca por concatenação de SQL.
// Q.E.D. .......... entrando o caminho do banco por parâmetro, a bateria inteira
//                   corre em directorio temporario, e nenhuma corrida de prova
//                   pode tocar o índice do operador. Não é commodidade: é o que
//                   impede a prova de corromper o acervo de quem nos usa.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// O punho do SQLite, declarado ADIANTE e no escopo global, que é onde o
// sqlite3.h o declara. Assim este cabeçalho não arrasta o sqlite3.h consigo, e
// quem inclue a bibliotheca não herda uma dependencia que não pediu.
struct sqlite3;

namespace mysong::nucleo {

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
