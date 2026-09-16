// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA BIBLIOTHECA, LAVRA, src/nucleo/biblioteca.cpp
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
#include <mutex>
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
    "  deduzido INTEGER NOT NULL, ordem INTEGER NOT NULL);"
    "CREATE INDEX faixas_artista_album ON faixas(artista, album, numero,"
    "  titulo);"
    "CREATE INDEX faixas_titulo ON faixas(titulo);"
    "CREATE INDEX faixas_ordem ON faixas(ordem);";

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

// Abre para ESCREVER, sem crear: a escripta de UMA linha não inventa índice.
sqlite3* abre_para_escrever(const std::filesystem::path& banco) {
  sqlite3* punho = nullptr;
  if (sqlite3_open_v2(banco.c_str(), &punho, SQLITE_OPEN_READWRITE, nullptr) !=
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

// Assenta a versão do esquema, e o limite de paginas quando a prova o pede.
//
// O limite é INJECÇÃO de falha, e não affinação: assenta-se DEPOIS do esquema,
// e contando as paginas que o esquema já gastou, porque o max_page_count que se
// peça abaixo do que já se usa o SQLite não o baixa, e a injecção não pegaria.
// Assim `limite` conta paginas ADDICIONAES, e um é quasi cheio de saida.
void assenta_versao_e_limite(sqlite3* punho, long limite) {
  char sql[96] = {0};
  std::snprintf(sql, sizeof(sql), "INSERT INTO esquema VALUES (%d);",
                kVersaoDoEsquema);
  sqlite3_exec(punho, sql, nullptr, nullptr, nullptr);
  if (limite <= 0) return;
  long ja_gastas = 0;
  sqlite3_stmt* passo = nullptr;
  if (sqlite3_prepare_v2(punho, "PRAGMA page_count;", -1, &passo, nullptr) ==
      SQLITE_OK) {
    if (sqlite3_step(passo) == SQLITE_ROW)
      ja_gastas = sqlite3_column_int(passo, 0);
    sqlite3_finalize(passo);
  }
  std::snprintf(sql, sizeof(sql), "PRAGMA max_page_count=%ld;",
                ja_gastas + limite);
  sqlite3_exec(punho, sql, nullptr, nullptr, nullptr);
}


// A MIGRAÇÃO do banco velho, que corre em toda abertura. O acervo de quem já
// nos usa não ha de baralhar-se ao abrir: a collunha nova enche-se com a ordem
// que elle HOJE vê, a de artista, album, numero e titulo. Banco que não se
// deixe escrever fica como está, e lê-se á mesma: índice velho legivel vale
// mais que índice recusado.
void migra_esquema(const std::filesystem::path& banco) {
  std::error_code erro;
  if (!std::filesystem::exists(banco, erro)) return;
  sqlite3* punho = nullptr;
  if (sqlite3_open_v2(banco.c_str(), &punho, SQLITE_OPEN_READWRITE, nullptr) !=
      SQLITE_OK) {
    sqlite3_close(punho);
    return;
  }
  int achada = 0;
  sqlite3_stmt* passo = nullptr;
  if (sqlite3_prepare_v2(punho, "SELECT versao FROM esquema LIMIT 1;", -1,
                         &passo, nullptr) == SQLITE_OK) {
    if (sqlite3_step(passo) == SQLITE_ROW) achada = sqlite3_column_int(passo, 0);
    sqlite3_finalize(passo);
  }
  if (achada == 1) {
    char sql[80] = {0};
    std::snprintf(sql, sizeof(sql), "UPDATE esquema SET versao = %d;",
                  kVersaoDoEsquema);
    sqlite3_exec(punho, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
    sqlite3_exec(punho,
                 "ALTER TABLE faixas ADD COLUMN ordem INTEGER NOT NULL"
                 " DEFAULT -1;"
                 "WITH posta AS (SELECT caminho, ROW_NUMBER() OVER (ORDER BY"
                 " artista, album, numero, titulo) - 1 AS logar FROM faixas)"
                 " UPDATE faixas SET ordem = (SELECT logar FROM posta WHERE"
                 " posta.caminho = faixas.caminho);"
                 "CREATE INDEX IF NOT EXISTS faixas_ordem ON faixas(ordem);",
                 nullptr, nullptr, nullptr);
    sqlite3_exec(punho, sql, nullptr, nullptr, nullptr);
    sqlite3_exec(punho, "COMMIT;", nullptr, nullptr, nullptr);
  }
  sqlite3_close(punho);
}

// O LOGAR PROVISORIO da faixa que entra sem ordem. Alto de proposito: assim
// ella fica DEPOIS de toda faixa que já tinha logar, e o renumerar do conclui()
// a traz ao fim da fila contigua. Um bilhão cabe folgado n'um inteiro de oito
// octetos, e nenhum acervo o alcança por baixo.
constexpr std::int64_t kFimDaFila = 1000000000;

// RENUMERA a ordem de zero até ao que ha menos um, conservando a ordem
// relativa. É o que faz a ordem ficar CONTIGUA depois de uma varredura em que
// umas faixas trouxeram logar antigo e outras nasceram sem elle. O desempate
// pelo caminho é necessario: duas faixas com a mesma ordem antiga hão de sahir
// n'uma ordem que se repita de corrida para corrida.
constexpr char kRenumera[] =
    "WITH posta AS (SELECT caminho, ROW_NUMBER() OVER"
    " (ORDER BY ordem, caminho) - 1 AS logar FROM faixas)"
    " UPDATE faixas SET ordem ="
    " (SELECT logar FROM posta WHERE posta.caminho = faixas.caminho);";

// Escapa os curingas do LIKE. Amarrar o termo NÃO os neutraliza: o SQLite
// concatena o valor amarrado no padrão e sómente depois o lê como padrão, donde
// um por cento vindo do operador continua a valer «qualquer cousa». Medido nesta
// Casa antes de se corrigir: buscar «%» devolvia o acervo inteiro.
std::string escapa_curingas(std::string_view termo) {
  std::string limpo;
  limpo.reserve(termo.size());
  for (const char letra : termo) {
    if (letra == '\\' || letra == '%' || letra == '_') limpo += '\\';
    limpo += letra;
  }
  return limpo;
}
}  // namespace

// Anda byte a byte, e sómente aceita a sequencia que TODA a regra do UTF-8
// aceita: comprimento pelo primeiro byte, continuação em 10xxxxxx, e nem
// sobrelongo, nem metade de par substituto, nem ponto fóra do plano. Byte que
// não sirva vae-se e entra o U+FFFD, e anda-se UM byte, nunca o comprimento
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
    : banco_(std::move(banco)) {
  migra_esquema(banco_);
  punho_ = abre_para_ler(banco_);
}

Biblioteca::~Biblioteca() { sqlite3_close(punho_); }

// O punho velho fecha-se ANTES de o novo abrir, e não depois: abrindo primeiro,
// um caminho que já não existe deixaria o velho aberto e a Casa a ler o inode
// antigo sem o saber, que é justamente o defeito que esta funcção veio corrigir.
void Biblioteca::reabre() {
  const std::lock_guard<std::mutex> chave(tranca_);
  sqlite3_close(punho_);
  migra_esquema(banco_);
  punho_ = abre_para_ler(banco_);
}

bool Biblioteca::aberta() const noexcept {
  const std::lock_guard<std::mutex> chave(tranca_);
  return punho_ != nullptr;
}

// Sem corre(), e de proposito: esta funcção é noexcept, e o corre() aloca
// vector e std::function, que podem lançar. Aqui não se aloca nada.
int Biblioteca::versao() const noexcept {
  const std::lock_guard<std::mutex> chave(tranca_);
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
  const std::lock_guard<std::mutex> chave(tranca_);
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
  faixa.ordem = sqlite3_column_int64(passo, 11);
  return faixa;
}

// As columnas, na ordem, para que as quatro consultas de faixa não as repitam
// cada uma á sua maneira. Repetidas, uma d'ellas sahiria da ordem um dia.
constexpr char kColumnas[] =
    "caminho, raiz, artista, album, titulo, numero, anno, duracao,"
    " modificado, tamanho, deduzido, ordem";

}  // namespace

std::vector<std::string> Biblioteca::artistas() const {
  const std::lock_guard<std::mutex> chave(tranca_);
  std::vector<std::string> nomes;
  corre(punho_,
        "SELECT DISTINCT artista FROM faixas ORDER BY artista;", {},
        [&nomes](sqlite3_stmt* passo) { nomes.push_back(texto(passo, 0)); });
  return nomes;
}

std::vector<std::string> Biblioteca::albuns(std::string_view artista) const {
  const std::lock_guard<std::mutex> chave(tranca_);
  std::vector<std::string> nomes;
  corre(punho_,
        "SELECT DISTINCT album FROM faixas WHERE artista = ?1"
        " ORDER BY album;",
        {artista},
        [&nomes](sqlite3_stmt* passo) { nomes.push_back(texto(passo, 0)); });
  return nomes;
}

std::vector<Faixa> Biblioteca::faixas_do_album(std::string_view artista,
                                               std::string_view album) const {
  const std::lock_guard<std::mutex> chave(tranca_);
  std::vector<Faixa> faixas;
  const std::string sql = std::string("SELECT ") + kColumnas +
                          " FROM faixas WHERE artista = ?1 AND album = ?2"
                          " ORDER BY numero, titulo;";
  corre(punho_, sql.c_str(), {artista, album},
        [&faixas](sqlite3_stmt* passo) {
          faixas.push_back(faixa_da_linha(passo));
        });
  return faixas;
}

std::vector<Faixa> Biblioteca::busca_faixa(std::string_view termo) const {
  const std::lock_guard<std::mutex> chave(tranca_);
  std::vector<Faixa> faixas;
  // Termo VAZIO é a vista plana do acervo, e é ella que leva a ordem propria
  // do operador; termo escripto é BUSCA, e busca ordena-se pelo que se procura,
  // que é o artista e o album. Fica assim declarado: a ordem arrumada á mão vale
  // na lista inteira, e não no resultado de uma procura.
  const std::string sql =
      std::string("SELECT ") + kColumnas +
      " FROM faixas WHERE titulo LIKE '%' || ?1 || '%' ESCAPE '\\' ORDER BY " +
      (termo.empty() ? "ordem;" : "artista, album, numero, titulo;");
  const std::string procurado = escapa_curingas(termo);
  corre(punho_, sql.c_str(), {procurado},
        [&faixas](sqlite3_stmt* passo) {
          faixas.push_back(faixa_da_linha(passo));
        });
  return faixas;
}

bool Biblioteca::acha_por_caminho(std::string_view caminho,
                                  Faixa& sahida) const {
  const std::lock_guard<std::mutex> chave(tranca_);
  const std::string sql = std::string("SELECT ") + kColumnas +
                          " FROM faixas WHERE caminho = ?1;";
  bool achou = false;
  corre(punho_, sql.c_str(), {caminho},
        [&achou, &sahida](sqlite3_stmt* passo) {
          sahida = faixa_da_linha(passo);
          achou = true;
        });
  return achou;
}

std::vector<std::string> Biblioteca::ordem_das_faixas() const {
  const std::lock_guard<std::mutex> chave(tranca_);
  std::vector<std::string> caminhos;
  corre(punho_, "SELECT caminho FROM faixas ORDER BY ordem;", {},
        [&caminhos](sqlite3_stmt* passo) {
          caminhos.push_back(texto(passo, 0));
        });
  return caminhos;
}

namespace {

// Uma escripta de UMA linha, por punho proprio aberto e fechado na chamada.
// Abrir o da classe para escrever daria licença de escripta a toda consulta.
bool muda_uma_linha(const std::filesystem::path& banco, const char* sql,
                    const std::vector<std::string_view>& amarras) {
  sqlite3* punho = nullptr;
  if (sqlite3_open_v2(banco.c_str(), &punho, SQLITE_OPEN_READWRITE, nullptr) !=
      SQLITE_OK) {
    sqlite3_close(punho);
    return false;
  }
  sqlite3_stmt* passo = nullptr;
  bool mudou = false;
  if (sqlite3_prepare_v2(punho, sql, -1, &passo, nullptr) == SQLITE_OK) {
    for (std::size_t i = 0; i < amarras.size(); ++i)
      sqlite3_bind_text(passo, static_cast<int>(i + 1), amarras[i].data(),
                        static_cast<int>(amarras[i].size()), SQLITE_TRANSIENT);
    mudou = sqlite3_step(passo) == SQLITE_DONE && sqlite3_changes(punho) > 0;
    sqlite3_finalize(passo);
  }
  sqlite3_close(punho);
  return mudou;
}
}  // namespace

bool Biblioteca::muda_o_titulo(std::string_view caminho,
                               std::string_view titulo) {
  const std::lock_guard<std::mutex> chave(tranca_);
  const std::string limpo = saneia_utf8(titulo);
  if (limpo.empty()) return false;
  return muda_uma_linha(banco_,
                        "UPDATE faixas SET titulo = ?2 WHERE caminho = ?1;",
                        {caminho, limpo});
}

// A faixa que sae FECHA o buraco d'ella, e na MESMA transacção em que sae: sem
// isso o disco teria, entre as duas escriptas, uma ordem com buraco, e mover
// para o logar de um buraco passaria a adivinhar quem é o visinho.
bool Biblioteca::esquece(std::string_view caminho) {
  const std::lock_guard<std::mutex> chave(tranca_);
  sqlite3* punho = abre_para_escrever(banco_);
  if (punho == nullptr) return false;
  sqlite3_exec(punho, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
  corre(punho, "DELETE FROM faixas WHERE caminho = ?1;", {caminho}, nullptr);
  const bool havia = sqlite3_changes(punho) > 0;
  if (havia) sqlite3_exec(punho, kRenumera, nullptr, nullptr, nullptr);
  sqlite3_exec(punho, havia ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr,
               nullptr);
  sqlite3_close(punho);
  return havia;
}

// Acha o logar d'ella, empurra as visinhas para o vão que ella deixa, e
// assenta-a. Sem sentinella, e não é descuido: aqui a ordem NÃO é chave, ao
// contrario do (rol, ordem) das listas, donde dous logares eguaes a meio da
// transacção não collidem com cousa alguma.
bool Biblioteca::move_faixa(std::string_view caminho, std::size_t para) {
  const std::lock_guard<std::mutex> chave(tranca_);
  sqlite3* punho = abre_para_escrever(banco_);
  if (punho == nullptr) return false;
  sqlite3_exec(punho, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
  long long de = -1, quantas = 0;
  corre(punho, "SELECT ordem FROM faixas WHERE caminho = ?1;", {caminho},
        [&de](sqlite3_stmt* passo) { de = sqlite3_column_int64(passo, 0); });
  corre(punho, "SELECT COUNT(*) FROM faixas;", {},
        [&quantas](sqlite3_stmt* passo) {
          quantas = sqlite3_column_int64(passo, 0);
        });
  if (de < 0) {
    sqlite3_exec(punho, "ROLLBACK;", nullptr, nullptr, nullptr);
    sqlite3_close(punho);
    return false;
  }
  // Pedido para além do fim assenta no ultimo logar: arrastar para baixo do
  // ultimo quer dizer «ao fim», e não «erro».
  long long ate = static_cast<long long>(para);
  if (ate >= quantas) ate = quantas - 1;
  if (ate != de) {
    char sql[176] = {0};
    std::snprintf(sql, sizeof(sql),
                  ate < de ? "UPDATE faixas SET ordem = ordem + 1 WHERE ordem"
                             " >= %lld AND ordem < %lld;"
                           : "UPDATE faixas SET ordem = ordem - 1 WHERE ordem"
                             " > %lld AND ordem <= %lld;",
                  ate < de ? ate : de, ate < de ? de : ate);
    sqlite3_exec(punho, sql, nullptr, nullptr, nullptr);
    std::snprintf(sql, sizeof(sql),
                  "UPDATE faixas SET ordem = %lld WHERE caminho = ?1;", ate);
    corre(punho, sql, {caminho}, nullptr);
  }
  sqlite3_exec(punho, "COMMIT;", nullptr, nullptr, nullptr);
  sqlite3_close(punho);
  return true;
}

Escriba::Escriba(std::filesystem::path banco, long limite_de_paginas)
    : banco_(std::move(banco)), temporario_(banco_.string() + ".tmp") {
  std::error_code erro;
  if (!banco_.parent_path().empty()) {
    std::filesystem::create_directories(banco_.parent_path(), erro);
    std::filesystem::permissions(banco_.parent_path(),
                                 std::filesystem::perms::owner_all, erro);
  }
  // Temporario de corrida anterior que se tenha ido abaixo com o processo: cae
  // aqui, e não se aproveita. Aproveitá-lo seria herdar metade de um índice.
  std::filesystem::remove(temporario_, erro);
  if (sqlite3_open_v2(temporario_.c_str(), &punho_,
                      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                      nullptr) != SQLITE_OK ||
      sqlite3_exec(punho_, kEsquema, nullptr, nullptr, nullptr) != SQLITE_OK) {
    sqlite3_close(punho_);
    punho_ = nullptr;
    std::filesystem::remove(temporario_, erro);
    return;
  }
  assenta_versao_e_limite(punho_, limite_de_paginas);
}

bool Escriba::grava(const Faixa& faixa) {
  if (punho_ == nullptr) return false;
  const std::string sql = std::string("INSERT OR REPLACE INTO faixas (") +
                          kColumnas + ") VALUES" +
                          " (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12);";
  sqlite3_stmt* passo = nullptr;
  if (sqlite3_prepare_v2(punho_, sql.c_str(), -1, &passo, nullptr) != SQLITE_OK)
    return false;
  // Saneadas ANTES de amarrar, e guardadas em cadeias que vivem até ao step:
  // amarrar um temporario que morra na mesma linha seria amarrar ponteiro morto.
  const std::string cadeias[5] = {
      saneia_utf8(faixa.caminho), saneia_utf8(faixa.raiz),
      saneia_utf8(faixa.artista), saneia_utf8(faixa.album),
      saneia_utf8(faixa.titulo)};
  for (int i = 0; i < 5; ++i)
    sqlite3_bind_text(passo, i + 1, cadeias[i].c_str(),
                      static_cast<int>(cadeias[i].size()), SQLITE_TRANSIENT);
  sqlite3_bind_int(passo, 6, faixa.numero);
  sqlite3_bind_int(passo, 7, faixa.anno);
  sqlite3_bind_int(passo, 8, faixa.duracao);
  sqlite3_bind_int64(passo, 9, faixa.modificado);
  sqlite3_bind_int64(passo, 10, faixa.tamanho);
  sqlite3_bind_int(passo, 11, static_cast<int>(faixa.deduzido));
  // Faixa sem logar vae para o FIM, e não para o principio: por isso a ordem
  // provisoria d'ella nasce ALTA, e cresce na ordem em que a varredura a acha.
  sqlite3_bind_int64(passo, 12, faixa.ordem >= 0 ? faixa.ordem
                                                 : kFimDaFila + ao_cabo_++);
  const int veredicto = sqlite3_step(passo);
  sqlite3_finalize(passo);
  return veredicto == SQLITE_DONE;
}

bool Escriba::conclui() {
  if (punho_ == nullptr) return false;
  // Fecha ANTES de renomear. O SQLite guarda o nome com que abriu, e renomear
  // por baixo de um punho aberto é pedir que elle escreva n'um arquivo que já
  // não é o que elle crê ser.
  sqlite3_exec(punho_, kRenumera, nullptr, nullptr, nullptr);
  const bool fechou = sqlite3_close(punho_) == SQLITE_OK;
  punho_ = nullptr;
  std::error_code erro;
  if (!fechou) {
    std::filesystem::remove(temporario_, erro);
    return false;
  }
  ::chmod(temporario_.c_str(), S_IRUSR | S_IWUSR);
  std::filesystem::rename(temporario_, banco_, erro);
  if (erro) {
    std::filesystem::remove(temporario_, erro);
    return false;
  }
  return true;
}

void Escriba::abandona() noexcept {
  if (punho_ != nullptr) {
    sqlite3_close(punho_);
    punho_ = nullptr;
  }
  std::error_code erro;
  std::filesystem::remove(temporario_, erro);
}

Escriba::~Escriba() { abandona(); }

bool Escriba::aberto() const noexcept { return punho_ != nullptr; }

const std::filesystem::path& Escriba::temporario() const noexcept {
  return temporario_;
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//, Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
