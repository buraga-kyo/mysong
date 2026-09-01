// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO COVER ART ARCHIVE, LAVRA — src/nucleo/caa.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o que o cabeçalho promette. As puras primeiro, e o que toca banco e
// rede por baixo, á maneira do musicbrainz.cpp, que é o irmão d'esta peça.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/caa.hpp"
#include "nucleo/capa.hpp"

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

std::string_view palavra_da_caca(CacaDeCapa desfecho) {
  switch (desfecho) {
    case CacaDeCapa::Embutida: return "capa embutida";
    case CacaDeCapa::JaTinha: return "já tinha capa";
    case CacaDeCapa::JaProcurada: return "já se procurou antes";
    case CacaDeCapa::ForaDoAlcance: return "fóra do alcance: não é MP3";
    case CacaDeCapa::SemMetadado:
      return "sem titulo ou duração no índice: duvidosa";
    case CacaDeCapa::Duvidosa: return "o MusicBrainz não casou: duvidosa";
    case CacaDeCapa::SemCapa: return "o Cover Art Archive não tem a capa";
    case CacaDeCapa::RedeFalhou: return "a rede falhou: fica para outra corrida";
    case CacaDeCapa::Recuo: return "o servidor pediu recuo: a corrida pára";
    case CacaDeCapa::FalhouAEscripta:
      return "a arte veio e a etiqueta não se escreveu";
  }
  return "desfecho sem nome";
}

std::string casa_release(const std::string& artista, const std::string& titulo,
                         int duracao_seg, const ConsultaComEstado& consulta,
                         CacaDeCapa* porque) {
  // Sem titulo ou duração não ha crivo nem rede: precedente da #13 e da #44.
  if (titulo.empty() || duracao_seg <= 0) {
    *porque = CacaDeCapa::SemMetadado;
    return {};
  }
  // O ESPIÃO anota o PIOR desfecho visto (recuo ganha de falha): é por elle
  // que o falso do resolve_gravacao, que engole ambos, se explica lá fóra.
  DesfechoMB pior = DesfechoMB::Achado;
  const Consulta espiada = [&consulta, &pior](const std::string& url,
                                              std::string* corpo) {
    long estado = 0;
    const DesfechoMB dito = consulta(url, corpo, &estado);
    if (dito == DesfechoMB::Recuo ||
        (dito == DesfechoMB::Falhou && pior == DesfechoMB::Achado))
      pior = dito;
    return dito;
  };
  FichaMB ficha;
  const bool casou = resolve_gravacao("", artista, titulo, duracao_seg * 1000,
                                      &ficha, espiada);
  if (pior == DesfechoMB::Recuo) { *porque = CacaDeCapa::Recuo; return {}; }
  if (!casou) {
    *porque = pior == DesfechoMB::Falhou ? CacaDeCapa::RedeFalhou
                                         : CacaDeCapa::Duvidosa;
    return {};
  }
  // Gravação sem release não tem porta para o CAA: confessa-se a duvida.
  if (ficha.release_mbid.empty()) { *porque = CacaDeCapa::Duvidosa; return {}; }
  return ficha.release_mbid;
}

CacaDeCapa caca_uma_faixa(const Faixa& faixa, MemoriaDeCapas* memoria,
                          const ConsultaComEstado& consulta,
                          std::map<std::string, std::string>* arte_da_release,
                          std::set<std::string>* release_sem_capa) {
  const std::filesystem::path caminho(faixa.caminho);
  // Capa que já ha poupa tudo, e ANTES do formato: flac com capa é «já tinha».
  if (!capa_ao_lado(caminho).empty() || !arte_embutida(caminho).empty())
    return CacaDeCapa::JaTinha;
  std::string extensao = caminho.extension().string();
  for (char& l : extensao) l = l >= 'A' && l <= 'Z' ? char(l + 32) : l;
  if (extensao != ".mp3") return CacaDeCapa::ForaDoAlcance;
  if (memoria->ja_procurada(faixa.caminho)) return CacaDeCapa::JaProcurada;
  CacaDeCapa porque = CacaDeCapa::Duvidosa;
  const std::string release = casa_release(faixa.artista, faixa.titulo,
                                           faixa.duracao, consulta, &porque);
  if (release.empty()) {
    // O definitivo assenta-se JÁ; recuo e rede muda amanhã podem casar.
    if (porque == CacaDeCapa::Duvidosa || porque == CacaDeCapa::SemMetadado)
      memoria->lembra(faixa.caminho, Procurada::Duvidosa);
    return porque;
  }
  // O 404 da mesma release visto NESTA corrida reusa-se sem nova requisição.
  if (release_sem_capa->count(release) != 0) {
    memoria->lembra(faixa.caminho, Procurada::SemCapa);
    return CacaDeCapa::SemCapa;
  }
  std::string arte;
  const auto guardada = arte_da_release->find(release);
  if (guardada != arte_da_release->end()) {
    arte = guardada->second;  // a irmã de album já a baixou nesta corrida
  } else {
    long estado = 0;
    std::string corpo;
    const DesfechoDaCapa dito = desfecho_da_capa(
        consulta(url_da_capa(release), &corpo, &estado), estado);
    if (dito == DesfechoDaCapa::Recuo) return CacaDeCapa::Recuo;
    if (dito == DesfechoDaCapa::SemCapa) {
      release_sem_capa->insert(release);
      memoria->lembra(faixa.caminho, Procurada::SemCapa);
      return CacaDeCapa::SemCapa;
    }
    if (dito != DesfechoDaCapa::Achada || corpo.empty())
      return CacaDeCapa::RedeFalhou;
    arte = (*arte_da_release)[release] = corpo;
  }
  return embute_arte(caminho, arte) ? CacaDeCapa::Embutida
                                    : CacaDeCapa::FalhouAEscripta;
}

std::string_view palavra_da_procurada(Procurada procurada) {
  switch (procurada) {
    case Procurada::SemCapa: return "sem capa no CAA";
    case Procurada::Duvidosa: return "duvidosa";
  }
  return "desfecho sem nome";
}

bool MemoriaDeCapas::ja_procurada(std::string_view caminho) const {
  bool achou = false;
  corre(punho_, "SELECT 1 FROM procurada WHERE caminho = ?1;",
        {std::string(caminho)},
        [&achou](sqlite3_stmt*) { achou = true; });
  return achou;
}

bool MemoriaDeCapas::lembra(std::string_view caminho, Procurada procurada) {
  // Caminho vazio não se assenta: linha sem identidade não se acharia mais.
  // O REPLACE cobre a corrida repetida por cima de memoria velha, e o relogio
  // é o do proprio SQLite, que poupa amarração de inteiro a esta peça.
  if (caminho.empty()) return false;
  return corre(punho_,
               "INSERT OR REPLACE INTO procurada VALUES "
               "(?1, ?2, strftime('%s','now'));",
               {std::string(caminho), std::string(palavra_da_procurada(procurada))});
}

std::size_t MemoriaDeCapas::quantas() const {
  std::size_t conta = 0;
  corre(punho_, "SELECT COUNT(*) FROM procurada;", {},
        [&conta](sqlite3_stmt* passo) {
          conta = static_cast<std::size_t>(sqlite3_column_int64(passo, 0));
        });
  return conta;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
