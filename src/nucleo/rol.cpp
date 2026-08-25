// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ROL — src/nucleo/rol.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. Toda cadeia vae ao banco por AMARRAÇÃO, e jamais por
// concatenação: é isso que faz uma lista chamada «Ária "Ré"» ser um nome, e não
// um pedaço de SQL.
//
// DOMÍNIO ......... o banco das listas, e o que o operador disse.
// CONTRA-DOMÍNIO .. as listas, e as faixas de cada uma.
// INVARIANTE ...... a ordem dos itens de uma lista é 0, 1, 2, ... sem buraco.
//                   Retirar fecha o buraco na MESMA transacção que retira, donde
//                   não ha instante em que o disco tenha a lista com buraco.
// Q.E.D. .......... a troca de duas ordens faz-se por sentinella negativa, e não
//                   por dous UPDATE crús: a chave é (rol, ordem), e dous crús
//                   collidiriam a meio.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/rol.hpp"

#include <sqlite3.h>

#include <cstddef>
#include <functional>
#include <utility>

namespace mysong::nucleo {
namespace {

// O ESQUEMA. A ordem é EXPLICITA n'uma columna, e não implicita na ordem de
// insercção: `rowid` do SQLite serviria enquanto ninguem mexesse, e mover para
// cima é justamente mexer.
constexpr char kEsquemaDoRol[] =
    "PRAGMA journal_mode=DELETE;"
    "PRAGMA synchronous=FULL;"
    // A chave estrangeira é o UNICO guarda de duas promessas: item de lista que não
    // existe não entra, e apagar a lista leva os itens. O PRAGMA é por CONNEXÃO, e
    // por isso vae no esquema, que corre em toda abertura.
    "PRAGMA foreign_keys=ON;"
    "CREATE TABLE IF NOT EXISTS esquema_do_rol (versao INTEGER NOT NULL);"
    "CREATE TABLE IF NOT EXISTS rol ("
    "  id INTEGER PRIMARY KEY, nome TEXT NOT NULL UNIQUE);"
    "CREATE TABLE IF NOT EXISTS item ("
    "  rol INTEGER NOT NULL REFERENCES rol(id) ON DELETE CASCADE,"
    "  ordem INTEGER NOT NULL, caminho TEXT NOT NULL,"
    "  PRIMARY KEY (rol, ordem));";

// corre — a consulta com os inteiros amarrados primeiro e as cadeias depois. A
// ordem é FIXA e a consulta acomoda-se a ella pelo indice explicito do SQLite,
// `?1`, `?2`: assim a mesma amarração serve consulta que repita o mesmo valor em
// tres logares, e não se conta ponto de interrogação á mão.
bool corre(sqlite3* punho, const char* sql,
           const std::vector<int>& numeros,
           const std::vector<std::string>& cadeias,
           const std::function<void(sqlite3_stmt*)>& cinzel = nullptr) {
  if (punho == nullptr) return false;
  sqlite3_stmt* passo = nullptr;
  if (sqlite3_prepare_v2(punho, sql, -1, &passo, nullptr) != SQLITE_OK)
    return false;
  int alvo = 1;
  for (const int numero : numeros) sqlite3_bind_int(passo, alvo++, numero);
  for (const std::string& cadeia : cadeias)
    sqlite3_bind_text(passo, alvo++, cadeia.c_str(),
                      static_cast<int>(cadeia.size()), SQLITE_TRANSIENT);
  int veredicto = sqlite3_step(passo);
  while (veredicto == SQLITE_ROW) {
    if (cinzel) cinzel(passo);
    veredicto = sqlite3_step(passo);
  }
  sqlite3_finalize(passo);
  return veredicto == SQLITE_DONE;
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
