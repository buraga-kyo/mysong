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

std::string texto(sqlite3_stmt* passo, int columna) {
  const unsigned char* bruto = sqlite3_column_text(passo, columna);
  if (bruto == nullptr) return std::string();
  return std::string(reinterpret_cast<const char*>(bruto));
}

// O COMPRIMENTO maximo do nome, em octetos. Cento e vinte: o que cabe na barra
// lateral do terminal mais estreito que esta Casa promette.
constexpr std::size_t kOctetosDoNome = 120;

// A SENTINELLA da troca. Negativa de proposito: ordem de item verdadeiro é
// sempre zero ou mais, donde menos um não collide com linha alguma.
constexpr int kSentinella = -1;

}  // namespace

std::string saneia_nome_de_rol(std::string_view crua) {
  std::size_t principio = 0, fim = crua.size();
  const auto branco = [](char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
  };
  while (principio < fim && branco(crua[principio])) ++principio;
  while (fim > principio && branco(crua[fim - 1])) --fim;
  std::string nome(crua.substr(principio, fim - principio));
  // Cento e vinte octetos. Nome mais comprido não cabe na barra lateral, e cortar
  // aqui é melhor que cortar na pintura: assim o que se grava é o que se vê.
  //
  // O corte anda para a FRENTE e guarda a ultima fronteira que caiba. Cortar em
  // cento e vinte e RECUAR até ao byte lider não presta, e a bateria accusou-o: o
  // byte que sobra na ponta pode ser elle MESMO um lider, e ahi o recuo pára logo
  // e deixa o lider solto sem os seus continuadores. Foi o que succedeu com cento e
  // dezenove letras mais um «á»: sahiam cento e vinte octetos, e o ultimo era meio
  // caracter. É o mesmo engano que a issue #11 corrigiu, e por isso vae escripto.
  if (nome.size() > kOctetosDoNome) {
    std::size_t corte = 0;
    for (std::size_t i = 0; i <= nome.size(); ++i) {
      const bool fronteira =
          i == nome.size() ||
          (static_cast<unsigned char>(nome[i]) & 0xC0) != 0x80;
      if (fronteira && i <= kOctetosDoNome) corte = i;
    }
    nome.resize(corte);
  }
  return nome;
}

Roleiro::Roleiro(std::filesystem::path banco) : banco_(std::move(banco)) {
  if (sqlite3_open_v2(banco_.c_str(), &punho_,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                      nullptr) != SQLITE_OK) {
    sqlite3_close(punho_);
    punho_ = nullptr;
    return;
  }
  if (sqlite3_exec(punho_, kEsquemaDoRol, nullptr, nullptr, nullptr) !=
      SQLITE_OK) {
    sqlite3_close(punho_);
    punho_ = nullptr;
    return;
  }
  // A versão assenta-se UMA vez. `IF NOT EXISTS` no esquema faz d'este
  // constructor idempotente, e a conta abaixo impede a versão de se repetir.
  int quantas = 0;
  corre(punho_, "SELECT COUNT(*) FROM esquema_do_rol;", {}, {},
        [&quantas](sqlite3_stmt* passo) {
          quantas = sqlite3_column_int(passo, 0);
        });
  if (quantas == 0)
    corre(punho_, "INSERT INTO esquema_do_rol VALUES (?);", {kVersaoDoRol}, {});
}

Roleiro::~Roleiro() {
  if (punho_ != nullptr) sqlite3_close(punho_);
}

bool Roleiro::aberto() const noexcept { return punho_ != nullptr; }

int Roleiro::versao() const noexcept {
  int qual = 0;
  corre(punho_, "SELECT versao FROM esquema_do_rol LIMIT 1;", {}, {},
        [&qual](sqlite3_stmt* passo) { qual = sqlite3_column_int(passo, 0); });
  return qual;
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
