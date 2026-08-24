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
#include <functional>
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

// Abre em SÓMENTE-LEITURA, e nullo quando não ha banco que se abra. Banco
// ausente é resposta vazia, e o nullo é como ella se carrega até ás consultas.
sqlite3* abre_para_ler(const std::filesystem::path& banco) {
  sqlite3* punho = nullptr;
  if (sqlite3_open_v2(banco.c_str(), &punho, SQLITE_OPEN_READONLY, nullptr) !=
      SQLITE_OK) {
    sqlite3_close(punho);
    return nullptr;
  }
  return punho;
}

// Corre uma consulta e entrega cada linha ao cinzel. As cadeias vão por
// sqlite3_bind_text, na ordem em que chegam, e JAMAIS por concatenação: é isto
// que faz um album chamado «Ária "Ré"» ser um nome e não um pedaço de SQL.
void corre(sqlite3* punho, const char* sql,
           const std::vector<std::string_view>& amarras,
           const std::function<void(sqlite3_stmt*)>& cinzel) {
  if (punho == nullptr) return;
  sqlite3_stmt* passo = nullptr;
  if (sqlite3_prepare_v2(punho, sql, -1, &passo, nullptr) != SQLITE_OK) return;
  for (std::size_t i = 0; i < amarras.size(); ++i)
    sqlite3_bind_text(passo, static_cast<int>(i + 1), amarras[i].data(),
                      static_cast<int>(amarras[i].size()), SQLITE_TRANSIENT);
  while (sqlite3_step(passo) == SQLITE_ROW) cinzel(passo);
  sqlite3_finalize(passo);
}

// Columna de texto em cadeia. Nullo do SQLite vira cadeia vazia, e não queda.
std::string texto(sqlite3_stmt* passo, int columna) {
  const unsigned char* bruto = sqlite3_column_text(passo, columna);
  if (bruto == nullptr) return std::string();
  return std::string(reinterpret_cast<const char*>(bruto));
}

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

Biblioteca::Biblioteca(std::filesystem::path banco)
    : banco_(std::move(banco)), punho_(abre_para_ler(banco_)) {}

Biblioteca::~Biblioteca() { sqlite3_close(punho_); }

bool Biblioteca::aberta() const noexcept { return punho_ != nullptr; }

// Sem corre(), e de proposito: esta funcção é noexcept, e o corre() aloca
// vector e std::function, que podem lançar. Aqui não se aloca nada.
int Biblioteca::versao() const noexcept {
  if (punho_ == nullptr) return 0;
  sqlite3_stmt* passo = nullptr;
  if (sqlite3_prepare_v2(punho_, "SELECT versao FROM esquema LIMIT 1;", -1,
                         &passo, nullptr) != SQLITE_OK)
    return 0;
  const int achado =
      sqlite3_step(passo) == SQLITE_ROW ? sqlite3_column_int(passo, 0) : 0;
  sqlite3_finalize(passo);
  return achado;
}

std::size_t Biblioteca::total() const {
  std::size_t quantas = 0;
  corre(punho_, "SELECT COUNT(*) FROM faixas;", {},
        [&quantas](sqlite3_stmt* passo) {
          quantas = static_cast<std::size_t>(sqlite3_column_int64(passo, 0));
        });
  return quantas;
}

namespace {

// Uma linha de `faixas` em Faixa. A ordem das columnas é a do esquema, que é a
// da estructura: quem mudar uma ha de mudar as tres, e a prova das consultas
// morre em cima se alguem mudar sómente duas.
Faixa faixa_da_linha(sqlite3_stmt* passo) {
  Faixa faixa;
  faixa.caminho = texto(passo, 0);
  faixa.raiz = texto(passo, 1);
  faixa.artista = texto(passo, 2);
  faixa.album = texto(passo, 3);
  faixa.titulo = texto(passo, 4);
  faixa.numero = sqlite3_column_int(passo, 5);
  faixa.anno = sqlite3_column_int(passo, 6);
  faixa.duracao = sqlite3_column_int(passo, 7);
  faixa.modificado = sqlite3_column_int64(passo, 8);
  faixa.tamanho = sqlite3_column_int64(passo, 9);
  faixa.deduzido = static_cast<unsigned>(sqlite3_column_int(passo, 10));
  return faixa;
}

// As columnas, na ordem, para que as quatro consultas de faixa não as repitam
// cada uma á sua maneira. Repetidas, uma d'ellas sahiria da ordem um dia.
constexpr char kColumnas[] =
    "caminho, raiz, artista, album, titulo, numero, anno, duracao,"
    " modificado, tamanho, deduzido";

}  // namespace

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
