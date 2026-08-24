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

// Anda byte a byte, e sómente aceita a sequencia que TODA a regra do UTF-8
// aceita: comprimento pelo primeiro byte, continuação em 10xxxxxx, e nem
// sobrelongo, nem metade de par substituto, nem ponto fóra do plano. Byte que
// não sirva vae-se e entra o U+FFFD, e anda-se UM byte — nunca o comprimento
// que o byte quebrado prometteu, que é como se perde texto bom a seguir ao mau.
std::string saneia_utf8(std::string_view crua) {
  std::string limpa;
  limpa.reserve(crua.size());
  for (std::size_t i = 0; i < crua.size();) {
    const unsigned char primeiro = static_cast<unsigned char>(crua[i]);
    std::size_t comprimento = 0;
    unsigned long ponto = 0;
    if (primeiro < 0x80u) { comprimento = 1; ponto = primeiro; }
    else if ((primeiro & 0xE0u) == 0xC0u) { comprimento = 2; ponto = primeiro & 0x1Fu; }
    else if ((primeiro & 0xF0u) == 0xE0u) { comprimento = 3; ponto = primeiro & 0x0Fu; }
    else if ((primeiro & 0xF8u) == 0xF0u) { comprimento = 4; ponto = primeiro & 0x07u; }
    bool bom = comprimento != 0 && i + comprimento <= crua.size();
    for (std::size_t k = 1; bom && k < comprimento; ++k) {
      const unsigned char seguinte = static_cast<unsigned char>(crua[i + k]);
      if ((seguinte & 0xC0u) != 0x80u) bom = false;
      else ponto = (ponto << 6) | (seguinte & 0x3Fu);
    }
    if (bom && comprimento == 2 && ponto < 0x80ul) bom = false;
    if (bom && comprimento == 3 && ponto < 0x800ul) bom = false;
    if (bom && comprimento == 4 && ponto < 0x10000ul) bom = false;
    if (bom && (ponto > 0x10FFFFul || (ponto >= 0xD800ul && ponto <= 0xDFFFul)))
      bom = false;
    if (bom) {
      limpa.append(crua.substr(i, comprimento));
      i += comprimento;
    } else {
      limpa.append("\xEF\xBF\xBD");
      ++i;
    }
  }
  return limpa;
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
