// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ROL, src/nucleo/rol.cpp
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
#include "nucleo/espelho.hpp"

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

// corre, a consulta com os inteiros amarrados primeiro e as cadeias depois. A
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

Roleiro::Roleiro(std::filesystem::path banco, std::filesystem::path acervo)
    : acervo_(std::move(acervo)), banco_(std::move(banco)) {
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
  if (!acervo_.empty()) altera([] { return true; });
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

std::vector<Rol> Roleiro::rois() const {
  std::vector<Rol> lista;
  // A conta dos itens vem por subconsulta, e não de columna guardada: columna
  // guardada desencontra-se do que ha no dia em que alguem apague item sem a
  // decrementar. Ordem de nome, que é a que o operador procura com o olho.
  corre(punho_,
        "SELECT id, nome, (SELECT COUNT(*) FROM item WHERE item.rol = rol.id)"
        " FROM rol ORDER BY nome;",
        {}, {}, [&lista](sqlite3_stmt* passo) {
          Rol rol;
          rol.id = sqlite3_column_int(passo, 0);
          rol.nome = texto(passo, 1);
          rol.quantos = sqlite3_column_int(passo, 2);
          lista.push_back(std::move(rol));
        });
  return lista;
}

bool Roleiro::altera(const std::function<bool()>& operacao) {
  erro_.clear();
  if (!corre(punho_, "BEGIN IMMEDIATE;", {}, {})) return false;
  bool pronto = operacao();
  EspelhoDePlaylists espelho(acervo_);
  if (pronto && !acervo_.empty()) {
    std::vector<ListaNoDisco> listas;
    for (const auto& rol : rois()) listas.push_back({rol.nome, faixas(rol.id)});
    pronto = espelho.prepara(listas) && espelho.publica();
    if (!pronto) erro_ = "não foi possível atualizar Playlists; confira permissões e arquivos alheios";
  }
  if (pronto && corre(punho_, "COMMIT;", {}, {})) {
    espelho.confirma();
    return true;
  }
  corre(punho_, "ROLLBACK;", {}, {});
  if (erro_.empty()) erro_ = "a alteração da playlist foi recusada";
  return false;
}

int Roleiro::cria(std::string_view nome) {
  const std::string limpo = saneia_nome_de_rol(nome);
  if (limpo.empty()) return 0;
  int identificador = 0;
  const bool criou = altera([&] {
    if (!corre(punho_, "INSERT INTO rol (nome) VALUES (?);", {}, {limpo})) return false;
    identificador = static_cast<int>(sqlite3_last_insert_rowid(punho_));
    return true;
  });
  return criou ? identificador : 0;
}

bool Roleiro::renomeia(int id, std::string_view nome) {
  const std::string limpo = saneia_nome_de_rol(nome);
  if (limpo.empty()) return false;
  if (!corre(punho_, "UPDATE rol SET nome = ?2 WHERE id = ?1;", {id}, {limpo}))
    return false;  // nome repetido cahe aqui tambem, pelo UNIQUE
  return sqlite3_changes(punho_) > 0;
}

bool Roleiro::apaga(int id) {
  // Os itens vão-se pela CASCATA da chave estrangeira, e não por um DELETE ao
  // lado. Houve os dous, e a mutação accusou-o: tirar qualquer um d'elles não
  // matava caso algum, e sómente tirando os DOUS a bateria accusava. Duas cousas
  // a prometter a mesma cousa dão duas verdades, e no dia em que uma mudasse a
  // outra ficaria a mentir. Fica a cascata, que é onde a relação se declara.
  if (!corre(punho_, "DELETE FROM rol WHERE id = ?;", {id}, {})) return false;
  return sqlite3_changes(punho_) > 0;
}

bool Roleiro::junta(int id, std::string_view caminho) {
  if (caminho.empty()) return false;
  const std::string qual(caminho);
  // A ordem nova é o que ha mais um. COALESCE porque MAX de lista vazia é nullo,
  // e nullo mais um continua nullo: sem elle, a primeira faixa nunca entrava.
  // Lista que não existe recusa-se pela CHAVE ESTRANGEIRA, e não por um WHERE
  // EXISTS ao lado: houve o WHERE, e a mutação que o tirava sobrevivia, porque a
  // chave já fazia o serviço. Vale aqui a mesma razão que em apaga().
  return corre(punho_,
               "INSERT INTO item (rol, ordem, caminho) SELECT ?1, "
               "COALESCE((SELECT MAX(ordem) + 1 FROM item WHERE rol = ?1), 0),"
               " ?2;",
               {id}, {qual}) &&
         sqlite3_changes(punho_) > 0;
}

bool Roleiro::retira(int id, int ordem) {
  if (punho_ == nullptr) return false;
  // UMA transacção para as duas cousas. Sem ella, uma queda entre o DELETE e o
  // fechamento do buraco deixaria a lista com buraco no disco, e mover para cima
  // passaria a adivinhar quem é o vizinho.
  sqlite3_exec(punho_, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
  corre(punho_, "DELETE FROM item WHERE rol = ?1 AND ordem = ?2;", {id, ordem},
        {});
  const bool havia = sqlite3_changes(punho_) > 0;
  if (havia)
    corre(punho_,
          "UPDATE item SET ordem = ordem - 1 WHERE rol = ?1 AND ordem > ?2;",
          {id, ordem}, {});
  sqlite3_exec(punho_, havia ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr,
               nullptr);
  return havia;
}

int Roleiro::retira_de_todos(std::string_view caminho) {
  if (punho_ == nullptr || caminho.empty()) return 0;
  // Colhe-se PRIMEIRO, e retira-se depois. Retirar dentro da propria consulta
  // seria mutar a taboa que se está a percorrer, e o SQLite não promette o que
  // a linha seguinte passa a ser.
  //
  // A ordem DESCENDENTE não é enfeite. Retirar FECHA o buraco, donde as ordens
  // acima da retirada baixam uma; colhendo de cima para baixo, a ordem de cada
  // occorrencia seguinte ainda vale quando lhe chega a vez. Ao contrario, a
  // mesma lista com a faixa duas vezes retiraria a segunda no logar errado.
  std::vector<std::pair<int, int>> onde;
  const std::string qual(caminho);
  corre(punho_,
        "SELECT rol, ordem FROM item WHERE caminho = ?"
        " ORDER BY rol, ordem DESC;",
        {}, {qual}, [&onde](sqlite3_stmt* passo) {
          onde.emplace_back(sqlite3_column_int(passo, 0),
                            sqlite3_column_int(passo, 1));
        });
  int quantas = 0;
  for (const std::pair<int, int>& item : onde)
    if (retira(item.first, item.second)) ++quantas;
  return quantas;
}

bool Roleiro::troca(int id, int uma, int outra) {
  if (punho_ == nullptr || uma == outra) return false;
  // Tres UPDATE, e não dous: a chave é (rol, ordem), e dous crús collidiriam a
  // meio, que o primeiro poria duas linhas na mesma ordem. A sentinella negativa
  // é logar que linha verdadeira nunca occupa.
  sqlite3_exec(punho_, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr);
  corre(punho_, "UPDATE item SET ordem = ?1 WHERE rol = ?2 AND ordem = ?3;",
        {kSentinella, id, uma}, {});
  const bool ha_uma = sqlite3_changes(punho_) > 0;
  corre(punho_, "UPDATE item SET ordem = ?1 WHERE rol = ?2 AND ordem = ?3;",
        {uma, id, outra}, {});
  const bool ha_outra = sqlite3_changes(punho_) > 0;
  corre(punho_, "UPDATE item SET ordem = ?1 WHERE rol = ?2 AND ordem = ?3;",
        {outra, id, kSentinella}, {});
  const bool ambas = ha_uma && ha_outra;
  sqlite3_exec(punho_, ambas ? "COMMIT;" : "ROLLBACK;", nullptr, nullptr,
               nullptr);
  return ambas;
}

std::vector<std::string> Roleiro::faixas(int id) const {
  std::vector<std::string> caminhos;
  corre(punho_, "SELECT caminho FROM item WHERE rol = ? ORDER BY ordem;", {id},
        {}, [&caminhos](sqlite3_stmt* passo) {
          caminhos.push_back(texto(passo, 0));
        });
  return caminhos;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
