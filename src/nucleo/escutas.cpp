#include "nucleo/escutas.hpp"

#include <sqlite3.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <ctime>
#include <memory>
#include <unistd.h>

namespace mysong::nucleo {
namespace {
using Calendario = EstatisticasDeEscuta::Calendario;
std::int64_t comeco_do_dia(Calendario::time_point data, int anteriores = 0) {
  const auto tempo = Calendario::to_time_t(data);
  std::tm dia{};
  localtime_r(&tempo, &dia);
  dia.tm_hour = dia.tm_min = dia.tm_sec = 0;
  dia.tm_mday -= anteriores;
  dia.tm_isdst = -1;
  return static_cast<std::int64_t>(std::mktime(&dia));
}
std::string texto(sqlite3_stmt* passo, int coluna) {
  const auto* valor = sqlite3_column_text(passo, coluna);
  return valor ? reinterpret_cast<const char*>(valor) : "";
}
using Instrucao = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
Instrucao prepara(sqlite3* banco, const char* sql) {
  sqlite3_stmt* passo = nullptr;
  sqlite3_prepare_v2(banco, sql, -1, &passo, nullptr);
  return Instrucao(passo, sqlite3_finalize);
}
void vincula(sqlite3_stmt* passo, int coluna, const std::string& valor) {
  sqlite3_bind_text(passo, coluna, valor.c_str(), -1, SQLITE_TRANSIENT);
}
bool audivel(const Retracto& r) {
  return r.estado == Estado::Tocando && !r.mudo && r.volume > 0;
}
std::string identidade_do_arquivo(const std::string& caminho) {
  if (caminho.empty()) return {};
  std::error_code erro;
  const auto absoluto = std::filesystem::weakly_canonical(caminho, erro);
  return erro ? caminho : absoluto.string();
}
Faixa identifica(const std::string& caminho, const EstatisticasDeEscuta::Metadados& consulta) {
  Faixa faixa = consulta ? consulta(caminho) : Faixa{};
  faixa.caminho = identidade_do_arquivo(caminho);
  if (faixa.titulo.empty() || faixa.artista.empty()) {
    TagLib::FileRef arquivo(caminho.c_str(), false);
    if (!arquivo.isNull() && arquivo.tag()) {
      if (faixa.titulo.empty()) faixa.titulo = arquivo.tag()->title().to8Bit(true);
      if (faixa.artista.empty()) faixa.artista = arquivo.tag()->artist().to8Bit(true);
    }
  }
  if (faixa.titulo.empty()) faixa.titulo = std::filesystem::path(caminho).stem().string();
  if (faixa.artista.empty()) faixa.artista = "Artista desconhecido";
  return faixa;
}
}  // namespace

EstatisticasDeEscuta::EstatisticasDeEscuta(std::filesystem::path arquivo, Metadados metadados)
    : metadados_(std::move(metadados)) {
  std::error_code erro;
  if (!arquivo.parent_path().empty()) std::filesystem::create_directories(arquivo.parent_path(), erro);
  if (erro) { erro_ = "Não foi possível criar a pasta do histórico: " + erro.message(); return; }
  if (sqlite3_open_v2(arquivo.c_str(), &banco_, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE |
      SQLITE_OPEN_FULLMUTEX, nullptr) != SQLITE_OK) {
    erro_ = "Não foi possível abrir o histórico de escutas";
    sqlite3_close(banco_); banco_ = nullptr; return;
  }
  std::filesystem::permissions(arquivo, std::filesystem::perms::owner_read |
      std::filesystem::perms::owner_write, std::filesystem::perm_options::replace, erro);
  sqlite3_busy_timeout(banco_, 100);
  auto versao = prepara(banco_, "PRAGMA user_version");
  if (!versao || sqlite3_step(versao.get()) != SQLITE_ROW || sqlite3_column_int(versao.get(), 0) > 1) {
    erro_ = "Histórico criado por uma versão mais recente";
    versao.reset(); sqlite3_close(banco_); banco_ = nullptr; return;
  }
  versao.reset();
  const char* esquema =
      "PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;"
      "BEGIN IMMEDIATE;"
      "CREATE TABLE IF NOT EXISTS faixas (caminho TEXT PRIMARY KEY, titulo TEXT NOT NULL, artista TEXT NOT NULL);"
      "CREATE TABLE IF NOT EXISTS escutas (ciclo TEXT NOT NULL, dia INTEGER NOT NULL, caminho TEXT NOT NULL,"
      "milissegundos INTEGER NOT NULL CHECK(milissegundos>=0), reproducoes INTEGER NOT NULL CHECK(reproducoes IN(0,1)),"
      "PRIMARY KEY(ciclo,dia));"
      "CREATE INDEX IF NOT EXISTS escutas_dia ON escutas(dia);"
      "CREATE INDEX IF NOT EXISTS escutas_faixa ON escutas(caminho);"
      "PRAGMA user_version=1; COMMIT;";
  if (sqlite3_exec(banco_, esquema, nullptr, nullptr, nullptr) != SQLITE_OK) {
    erro_ = "Não foi possível preparar o histórico: " + std::string(sqlite3_errmsg(banco_));
    sqlite3_exec(banco_, "ROLLBACK", nullptr, nullptr, nullptr);
    sqlite3_close(banco_); banco_ = nullptr;
  }
}
EstatisticasDeEscuta::~EstatisticasDeEscuta() {
  salva();
  if (banco_) sqlite3_close(banco_);
}

bool EstatisticasDeEscuta::grava() {
  if (!banco_) return false;
  if (pendentes_.empty()) return true;
  const auto falha = [&] {
    erro_ = "Não foi possível salvar as escutas: " + std::string(sqlite3_errmsg(banco_));
    sqlite3_exec(banco_, "ROLLBACK", nullptr, nullptr, nullptr);
    return false;
  };
  if (sqlite3_exec(banco_, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr) != SQLITE_OK) return falha();
  auto faixa = prepara(banco_, "INSERT INTO faixas VALUES(?1,?2,?3) ON CONFLICT(caminho) DO UPDATE "
      "SET titulo=excluded.titulo, artista=excluded.artista");
  auto escuta = prepara(banco_, "INSERT INTO escutas VALUES(?1,?2,?3,?4,?5) ON CONFLICT(ciclo,dia) DO UPDATE "
      "SET milissegundos=excluded.milissegundos, reproducoes=excluded.reproducoes");
  if (!faixa || !escuta) return falha();
  for (const auto& entrada : pendentes_) {
    const auto& r = entrada.second;
    sqlite3_reset(faixa.get());
    vincula(faixa.get(), 1, r.caminho); vincula(faixa.get(), 2, r.titulo); vincula(faixa.get(), 3, r.artista);
    if (sqlite3_step(faixa.get()) != SQLITE_DONE) return falha();
    sqlite3_reset(escuta.get());
    vincula(escuta.get(), 1, r.ciclo); sqlite3_bind_int64(escuta.get(), 2, r.dia);
    vincula(escuta.get(), 3, r.caminho); sqlite3_bind_int64(escuta.get(), 4, r.milissegundos);
    sqlite3_bind_int64(escuta.get(), 5, r.reproducoes);
    if (sqlite3_step(escuta.get()) != SQLITE_DONE) return falha();
  }
  if (sqlite3_exec(banco_, "COMMIT", nullptr, nullptr, nullptr) != SQLITE_OK) return falha();
  pendentes_.clear(); erro_.clear();
  return true;
}
bool EstatisticasDeEscuta::salva() {
  std::lock_guard<std::mutex> chave(tranca_);
  return grava();
}

void EstatisticasDeEscuta::acrescenta(std::int64_t ms, Calendario::time_point data) {
  if (ms <= 0) return;
  const auto dia = comeco_do_dia(data);
  const auto cria = [&](std::int64_t chave) -> Registro& {
    auto& r = dias_correntes_[chave];
    r.ciclo = ciclo_; r.dia = chave; r.caminho = faixa_.caminho;
    r.titulo = faixa_.titulo; r.artista = faixa_.artista;
    return r;
  };
  const auto inicio = data - std::chrono::milliseconds(ms);
  if (inicio < Calendario::from_time_t(static_cast<std::time_t>(dia))) {
    const auto anteriores = std::chrono::duration_cast<std::chrono::milliseconds>(
        Calendario::from_time_t(static_cast<std::time_t>(dia)) - inicio).count();
    auto& r = cria(comeco_do_dia(inicio));
    r.milissegundos += anteriores;
    pendentes_[{ciclo_, r.dia}] = r;
    ms -= anteriores;
  }
  auto& r = cria(dia);
  r.milissegundos += ms;
  // O total da sessão inclui também a parte anterior à meia-noite.
  ouvido_ = 0;
  for (const auto& entrada : dias_correntes_) ouvido_ += entrada.second.milissegundos;
  const double limite = anterior_.duracao > 0 && std::isfinite(anterior_.duracao)
      ? std::min(30.0, anterior_.duracao / 2) : 30.0;
  if (!contada_ && ouvido_ >= limite * 1000) { r.reproducoes = 1; contada_ = true; }
  pendentes_[{ciclo_, dia}] = r;
}

void EstatisticasDeEscuta::observa(const Retracto& r, Relogio::time_point agora, Calendario::time_point data) {
  std::lock_guard<std::mutex> chave(tranca_);
  const auto caminho = r.faixa ? *r.faixa : std::string{};
  const auto anterior = anterior_.faixa ? *anterior_.faixa : std::string{};
  if (!amostrado_ || caminho != anterior || r.reproducao != anterior_.reproducao) {
    grava();
    faixa_ = caminho.empty() ? Faixa{} : identifica(caminho, metadados_);
    static std::atomic<std::uint64_t> serie{0};
    ciclo_ = std::to_string(Calendario::now().time_since_epoch().count()) + "-" +
        std::to_string(::getpid()) + "-" + std::to_string(++serie);
    dias_correntes_.clear(); ouvido_ = 0; contada_ = false; espera_ = fracao_ms_ = 0;
    ultima_gravacao_ = agora;
  } else {
    const double decorrido = std::chrono::duration<double>(agora - instante_anterior_).count();
    if (!caminho.empty() && audivel(anterior_) && audivel(r) &&
        std::isfinite(r.posicao) && std::isfinite(anterior_.posicao) && decorrido > 0 && decorrido <= 2) {
      espera_ = std::min(2.0, espera_ + decorrido);
      const double avanco = r.posicao - anterior_.posicao;
      if (avanco < 0 || avanco > espera_ + 0.25) espera_ = 0;
      else if (avanco > 0) {
        const double ms = std::min(espera_, avanco) * 1000 + fracao_ms_;
        const auto inteiros = static_cast<std::int64_t>(ms + 1e-6);
        fracao_ms_ = ms - inteiros;
        acrescenta(inteiros, data);
        espera_ = 0;
      }
    } else espera_ = 0;
    if (agora - ultima_gravacao_ >= std::chrono::seconds(5)) { grava(); ultima_gravacao_ = agora; }
  }
  anterior_ = r; instante_anterior_ = agora; amostrado_ = true;
}

ResumoDasEscutas EstatisticasDeEscuta::resumo(int dias, const std::string& atual, Calendario::time_point data) {
  std::lock_guard<std::mutex> chave(tranca_);
  grava();
  ResumoDasEscutas r;
  r.erro = erro_;
  if (!banco_) return r;
  const auto desde = dias > 0 ? comeco_do_dia(data, dias - 1) : 0;
  auto totais = prepara(banco_, "SELECT COALESCE(SUM(e.milissegundos),0),COALESCE(SUM(e.reproducoes),0),"
      "COUNT(DISTINCT e.caminho),COUNT(DISTINCT f.artista COLLATE NOCASE) FROM escutas e "
      "JOIN faixas f ON f.caminho=e.caminho WHERE e.dia>=?1");
  if (totais) {
    sqlite3_bind_int64(totais.get(), 1, desde);
    if (sqlite3_step(totais.get()) == SQLITE_ROW) {
      r.milissegundos = sqlite3_column_int64(totais.get(), 0); r.reproducoes = sqlite3_column_int64(totais.get(), 1);
      r.musicas = sqlite3_column_int64(totais.get(), 2); r.artistas = sqlite3_column_int64(totais.get(), 3);
    }
  }
  const auto consulta = [&](const char* sql, std::vector<ItemDeEscuta>& itens, const std::string* caminho = nullptr) {
    auto passo = prepara(banco_, sql);
    if (!passo) { r.erro = "Não foi possível consultar as escutas"; return; }
    sqlite3_bind_int64(passo.get(), 1, desde);
    if (caminho) vincula(passo.get(), 2, *caminho);
    while (sqlite3_step(passo.get()) == SQLITE_ROW)
      itens.push_back({texto(passo.get(), 0), texto(passo.get(), 1),
          sqlite3_column_int64(passo.get(), 2), sqlite3_column_int64(passo.get(), 3)});
  };
  consulta("SELECT f.titulo,f.artista,SUM(e.milissegundos),SUM(e.reproducoes) FROM escutas e "
      "JOIN faixas f ON f.caminho=e.caminho WHERE e.dia>=?1 GROUP BY e.caminho "
      "ORDER BY SUM(e.milissegundos) DESC,f.artista,f.titulo LIMIT 20", r.mais_musicas);
  consulta("SELECT f.artista,f.artista,SUM(e.milissegundos),SUM(e.reproducoes) FROM escutas e "
      "JOIN faixas f ON f.caminho=e.caminho WHERE e.dia>=?1 GROUP BY f.artista COLLATE NOCASE "
      "ORDER BY SUM(e.milissegundos) DESC,f.artista LIMIT 20", r.mais_artistas);
  if (!atual.empty()) {
    const auto caminho_atual = identidade_do_arquivo(atual);
    std::vector<ItemDeEscuta> musica;
    consulta("SELECT f.titulo,f.artista,SUM(e.milissegundos),SUM(e.reproducoes) FROM escutas e "
        "JOIN faixas f ON f.caminho=e.caminho WHERE e.dia>=?1 AND e.caminho=?2 GROUP BY e.caminho", musica, &caminho_atual);
    if (!musica.empty()) r.atual = musica.front();
  }
  return r;
}

}  // namespace mysong::nucleo
