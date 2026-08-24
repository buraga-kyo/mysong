// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA BIBLIOTHECA, LAVRA — src/nucleo/biblioteca.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o cabecalho. É o UNICO arquivo d'esta obra que inclue sqlite3.h, e é
// d'ahi que lhe vem a fronteira: quem consulta o acervo não herda o SQLite.
//
// DOMÍNIO ......... um caminho de banco, e faixas já apuradas.
// CONTRA-DOMÍNIO .. as consultas, e um índice novo posto no logar por rename.
// INVARIANTE ...... toda cadeia vae ao banco por sqlite3_bind_text, e JAMAIS
//                   por concatenação: nome de album com aspa não pode virar
//                   SQL. Nenhuma funcção lança pela borda, e o Escriba que se
//                   destrua sem concluir desfaz o seu temporario.
#include "nucleo/biblioteca.hpp"

#include <sqlite3.h>
#include <sys/stat.h>

#include <cstdio>
#include <utility>

namespace mysong::nucleo {
namespace {

// O ESQUEMA, n'uma peça. A ordem das columnas é a da estructura Faixa, para
// que a amarração se leia contra a declaração sem se ter de contar á mão.
constexpr char kEsquema[] =
    "PRAGMA journal_mode=DELETE;"
    "PRAGMA synchronous=FULL;"
    "CREATE TABLE esquema (versao INTEGER NOT NULL);"
    "CREATE TABLE faixas ("
    "  caminho TEXT PRIMARY KEY, raiz TEXT NOT NULL, artista TEXT NOT NULL,"
    "  album TEXT NOT NULL, titulo TEXT NOT NULL, numero INTEGER NOT NULL,"
    "  anno INTEGER NOT NULL, duracao INTEGER NOT NULL,"
    "  modificado INTEGER NOT NULL, tamanho INTEGER NOT NULL,"
    "  deduzido INTEGER NOT NULL);"
    "CREATE INDEX faixas_artista_album ON faixas(artista, album, numero,"
    "  titulo);"
    "CREATE INDEX faixas_titulo ON faixas(titulo);";

}  // namespace

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
