// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO COVER ART ARCHIVE, LAVRA — src/nucleo/caa.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o que o cabeçalho promette. As puras primeiro, e o que toca banco e
// rede por baixo, á maneira do musicbrainz.cpp, que é o irmão d'esta peça.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/caa.hpp"

#include <sqlite3.h>

#include <functional>
#include <string>
#include <vector>

namespace mysong::nucleo {
namespace {

// A raiz das capas por release. N'uma constante, para que haja UM endereço.
constexpr char kRaizDoCaa[] = "https://coverartarchive.org/release/";

// O ESQUEMA da memoria, no regime do rol: journal DELETE e synchronous FULL,
// que aqui a atomicidade é a da transacção, um assento por faixa. O `quando`
// assenta-se para o operador poder perguntar ao banco QUANDO se procurou.
constexpr char kEsquemaDaMemoria[] =
    "PRAGMA journal_mode=DELETE;"
    "PRAGMA synchronous=FULL;"
    "CREATE TABLE IF NOT EXISTS esquema_da_memoria (versao INTEGER NOT NULL);"
    "CREATE TABLE IF NOT EXISTS procurada ("
    "  caminho TEXT PRIMARY KEY, desfecho TEXT NOT NULL,"
    "  quando INTEGER NOT NULL);";

// corre — a consulta com as cadeias por AMARRAÇÃO, nunca por concatenação: é
// o desenho do rol, aparado ao que esta memoria usa (cadeia, e nada mais).
bool corre(sqlite3* punho, const std::string& sql,
           const std::vector<std::string>& cadeias,
           const std::function<void(sqlite3_stmt*)>& cinzel = nullptr) {
  if (punho == nullptr) return false;
  sqlite3_stmt* passo = nullptr;
  if (sqlite3_prepare_v2(punho, sql.c_str(), -1, &passo, nullptr) != SQLITE_OK)
    return false;
  int alvo = 1;
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

}  // namespace

std::string url_da_capa(std::string_view release_mbid) {
  // O MBID vem do proprio MB, e ainda assim não se interpola cru: é a mesma
  // doutrina da url_da_ficha, e o escapa_url é o mesmo d'ella.
  if (release_mbid.empty()) return {};
  return std::string(kRaizDoCaa) + escapa_url(release_mbid) + "/front-500";
}

DesfechoDaCapa desfecho_da_capa(DesfechoMB desfecho, long estado_http) {
  // O Recuo e o Achado passam taes e quaes; o Falhou reparte-se pelo estado:
  // o 404 é o CAA a dizer «capa não ha», definitivo, e todo o resto (rede
  // muda com estado zero, 5xx sem recuo, outra recusa) é passageiro.
  if (desfecho == DesfechoMB::Recuo) return DesfechoDaCapa::Recuo;
  if (desfecho == DesfechoMB::Achado) return DesfechoDaCapa::Achada;
  return estado_http == 404 ? DesfechoDaCapa::SemCapa
                            : DesfechoDaCapa::Transitoria;
}


MemoriaDeCapas::MemoriaDeCapas(std::filesystem::path banco)
    : banco_(std::move(banco)) {
  // Abre para ler e escrever, e cria não havendo: a primeira corrida do
  // operador não ha de falhar por falta de arquivo. O esquema é idempotente
  // (IF NOT EXISTS), donde reabrir o banco de hontem não o toca.
  if (sqlite3_open_v2(banco_.c_str(), &punho_,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                      nullptr) != SQLITE_OK ||
      sqlite3_exec(punho_, kEsquemaDaMemoria, nullptr, nullptr, nullptr) !=
          SQLITE_OK) {
    sqlite3_close(punho_);
    punho_ = nullptr;
  }
  if (punho_ == nullptr) return;
  // A versão assenta-se UMA vez, no exacto regime do Roleiro. O numero é
  // constante de compilação, e não cadeia do operador: compô-lo na consulta
  // não abre porta alguma que a amarração feche.
  int quantas = 0;
  corre(punho_, "SELECT COUNT(*) FROM esquema_da_memoria;", {},
        [&quantas](sqlite3_stmt* passo) {
          quantas = sqlite3_column_int(passo, 0);
        });
  if (quantas == 0)
    corre(punho_,
          "INSERT INTO esquema_da_memoria VALUES (" +
              std::to_string(kVersaoDaMemoria) + ");",
          {});
}

MemoriaDeCapas::~MemoriaDeCapas() {
  if (punho_ != nullptr) sqlite3_close(punho_);
}

bool MemoriaDeCapas::aberta() const noexcept { return punho_ != nullptr; }

int MemoriaDeCapas::versao() const noexcept {
  int lida = 0;
  corre(punho_, "SELECT versao FROM esquema_da_memoria;", {},
        [&lida](sqlite3_stmt* passo) { lida = sqlite3_column_int(passo, 0); });
  return lida;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
