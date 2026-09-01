// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO COVER ART ARCHIVE — testes/prova_caa.cpp
// ══════════════════════════════════════════════════════════════════════════
// SEM REDE, do principio ao fim: a Consulta entra de mentira, os corpos são
// recortes escriptos á mão sobre a fórma viva, e as faixas são fixtures
// lavradas em directorio temporario. A UNICA prova viva da issue corre fóra
// da bateria, pelo espia_capa, e o PR diz o que se viu.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <fstream>
#include <string>

#include "nucleo/caa.hpp"
#include "nucleo/capa.hpp"

namespace nu = mysong::nucleo;

namespace {

// A COVA de cada caso, no molde da prova do rol: prova alguma toca banco alheio.
class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-caa-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~Cova() {
    std::error_code erro; std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  const std::filesystem::path& raiz() const { return caminho_; }
  std::filesystem::path banco() const { return caminho_ / "capas.sqlite3"; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

}  // namespace

// Nome SEM virgula nem ponto-e-virgula: a virgula quebra o -tc do doctest, e o
// ponto-e-virgula é separador de LISTA do CMake, que partiria a entrada do
// ctest em duas fantasmas que filtram nada e passam vazias. Foi medido aqui.
TEST_CASE("a URL da capa sahe pela release escapada e vazia sem MBID") {
  CHECK(nu::url_da_capa("bc9051ea-9d77-3ae3-8bd6-45960a8c0e4f") ==
        "https://coverartarchive.org/release/"
        "bc9051ea-9d77-3ae3-8bd6-45960a8c0e4f/front-500");
  // MBID não se interpola cru: byte fóra da taboa sahe por cento e hexa. Não
  // é caso do vivo, e é justamente por isso que a guarda se prova aqui.
  CHECK(nu::url_da_capa("a b") ==
        "https://coverartarchive.org/release/a%20b/front-500");
  CHECK(nu::url_da_capa("").empty());
}

TEST_CASE("a taboa dos desfechos do CAA reparte o Falhou pelo estado") {
  using D = nu::DesfechoDaCapa;
  // O que o MB já nomeou passa tal e qual.
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Achado, 200) == D::Achada);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Recuo, 503) == D::Recuo);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Recuo, 429) == D::Recuo);
  // O 404 é o CAA a dizer «capa não ha»: definitivo, lembra-se.
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 404) == D::SemCapa);
  // Rede muda (estado zero) e 5xx sem recuo respondem amanhã: não se lembram.
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 0) == D::Transitoria);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 500) == D::Transitoria);
  CHECK(nu::desfecho_da_capa(nu::DesfechoMB::Falhou, 403) == D::Transitoria);
}

TEST_CASE("a memoria das procuradas escreve reabre e relê") {
  const Cova cova;
  {
    nu::MemoriaDeCapas memoria(cova.banco());
    REQUIRE(memoria.aberta());
    CHECK(memoria.versao() == nu::kVersaoDaMemoria);
    CHECK(memoria.lembra("/a/faixa.mp3", nu::Procurada::SemCapa));
    CHECK(memoria.lembra("/b/outra.mp3", nu::Procurada::Duvidosa));
    CHECK_FALSE(memoria.lembra("", nu::Procurada::SemCapa));  // sem identidade
  }
  // REABERTA de um punho novo, como a corrida de amanhã: o que poupa a rede
  // é o dado que persiste, e não o cache de um objecto vivo.
  nu::MemoriaDeCapas relida(cova.banco());
  REQUIRE(relida.aberta());
  CHECK(relida.ja_procurada("/a/faixa.mp3"));
  CHECK(relida.ja_procurada("/b/outra.mp3"));
  CHECK_FALSE(relida.ja_procurada("/c/terceira.mp3"));
  CHECK(relida.lembra("/a/faixa.mp3", nu::Procurada::Duvidosa));
  CHECK(relida.quantas() == 2);
}

namespace {

// Os corpos de MENTIRA, na fórma viva do ws/2 (a fórma inteira está presa na
// prova do MusicBrainz; aqui basta o que os leitores da caça consomem).
constexpr char kBuscaComEleita[] =
    R"({"recordings":[{"id":"8f3471b5-7e6a-48da-86a9-c1c07a0f47ae",)"
    R"("score":100,"title":"Never Gonna Give You Up","length":213000,)"
    R"("first-release-date":"1987-07-27"}]})";
constexpr char kFichaComRelease[] =
    R"({"length":213000,"title":"Never Gonna Give You Up",)"
    R"("id":"8f3471b5-7e6a-48da-86a9-c1c07a0f47ae","releases":[)"
    R"({"title":"Whenever You Need Somebody","date":"1987-10-01",)"
    R"("status":"Official","release-group":{"primary-type":"Album",)"
    R"("secondary-types":[]},"id":"bc9051ea-9d77-3ae3-8bd6-45960a8c0e4f"}],)"
    R"("artist-credit":[{"name":"Rick Astley"}]})";

// A REDE DE MENTIRA: responde pelo pedaço da URL e CONTA as requisições, que
// é como o «zero consultas» do aceite se afere. O que devolver a cada porta
// entra pelos tres corpos; vazio quer dizer «404 sem corpo».
struct RedeDeMentira {
  std::string busca = kBuscaComEleita;
  std::string ficha = kFichaComRelease;
  std::string capa = std::string("\xFF\xD8\xFF\xE0", 4) + "arte de mentira";
  nu::DesfechoMB desfecho = nu::DesfechoMB::Achado;  // o que a rede responde
  long estado = 200;
  nu::DesfechoMB desfecho_capa = nu::DesfechoMB::Achado;  // a porta do CAA
  long estado_capa = 200;
  int gastas = 0;
  mysong::nucleo::ConsultaComEstado consulta() {
    return [this](const std::string& url, std::string* corpo, long* dito) {
      ++gastas;
      *dito = estado;
      if (url.find("recording?query=") != std::string::npos) *corpo = busca;
      else if (url.find("recording/") != std::string::npos) *corpo = ficha;
      else if (url.find("coverartarchive") != std::string::npos) {
        *corpo = capa;
        *dito = estado_capa;
        return desfecho_capa;
      }
      return desfecho;
    };
  }
};

}  // namespace

TEST_CASE("o casamento feliz gasta duas consultas e dá a release canonica") {
  RedeDeMentira rede;
  nu::CacaDeCapa porque = nu::CacaDeCapa::Embutida;
  const std::string mbid =
      nu::casa_release("Rick Astley", "Never Gonna Give You Up", 213,
                       rede.consulta(), &porque);
  CHECK(mbid == "bc9051ea-9d77-3ae3-8bd6-45960a8c0e4f");
  CHECK(rede.gastas == 2);  // a busca e a ficha, e nada mais
}

TEST_CASE("sem titulo ou sem duração o casamento nem toca a rede") {
  RedeDeMentira rede;
  nu::CacaDeCapa porque = nu::CacaDeCapa::Embutida;
  CHECK(nu::casa_release("Rick", "", 213, rede.consulta(), &porque).empty());
  CHECK(porque == nu::CacaDeCapa::SemMetadado);
  CHECK(nu::casa_release("Rick", "Never", 0, rede.consulta(), &porque).empty());
  CHECK(porque == nu::CacaDeCapa::SemMetadado);
  CHECK(rede.gastas == 0);  // é o «sem gastar rede alguma» do aceite
}

TEST_CASE("duvidosa recuo e rede muda explicam o casamento vazio") {
  nu::CacaDeCapa porque = nu::CacaDeCapa::Embutida;
  {  // score baixo: o MB respondeu e não casou. É a duvidosa que se lembra.
    RedeDeMentira rede;
    rede.busca = R"({"recordings":[{"id":"x","score":80,"length":213000}]})";
    CHECK(nu::casa_release("R", "N", 213, rede.consulta(), &porque).empty());
    CHECK(porque == nu::CacaDeCapa::Duvidosa);
    CHECK(rede.gastas == 1);  // sem eleita, a ficha não se gasta
  }
  {  // gravação casada sem release alguma: não ha porta para o CAA.
    RedeDeMentira rede;
    rede.ficha = R"({"title":"Never Gonna Give You Up","releases":[]})";
    CHECK(nu::casa_release("R", "N", 213, rede.consulta(), &porque).empty());
    CHECK(porque == nu::CacaDeCapa::Duvidosa);
  }
  {  // recuo: o espião o vê por baixo do falso do resolve_gravacao.
    RedeDeMentira rede;
    rede.desfecho = nu::DesfechoMB::Recuo;
    rede.estado = 503;
    CHECK(nu::casa_release("R", "N", 213, rede.consulta(), &porque).empty());
    CHECK(porque == nu::CacaDeCapa::Recuo);
  }
  {  // rede muda: transitoria, e NÃO duvidosa, que duvidosa se lembraria.
    RedeDeMentira rede;
    rede.desfecho = nu::DesfechoMB::Falhou;
    rede.estado = 0;
    CHECK(nu::casa_release("R", "N", 213, rede.consulta(), &porque).empty());
    CHECK(porque == nu::CacaDeCapa::RedeFalhou);
  }
}

namespace {

// poe_mp3 — a fixture minima da prova da capa: um quadro de MPEG que a taglib
// aceita. Etiqueta não precisa: a caça lê o metadado da Faixa do ÍNDICE.
std::filesystem::path poe_mp3(const Cova& cova, const std::string& nome) {
  const std::filesystem::path faixa = cova.raiz() / nome;
  std::ofstream(faixa, std::ios::binary) << "\xFF\xFB\x90\x00";
  return faixa;
}

nu::Faixa faixa_de(const std::filesystem::path& caminho) {
  nu::Faixa faixa;
  faixa.caminho = caminho.string();
  faixa.artista = "Rick Astley";
  faixa.titulo = "Never Gonna Give You Up";
  faixa.duracao = 213;
  return faixa;
}

}  // namespace

TEST_CASE("a caça embute a arte da release casada e o painel a relê") {
  const Cova cova;
  nu::MemoriaDeCapas memoria(cova.banco());
  RedeDeMentira rede;
  std::map<std::string, std::string> artes;
  std::set<std::string> sem_capa;
  const std::filesystem::path faixa = poe_mp3(cova, "01 - Never.mp3");
  CHECK(nu::caca_uma_faixa(faixa_de(faixa), &memoria, rede.consulta(), &artes,
                           &sem_capa) == nu::CacaDeCapa::Embutida);
  CHECK(rede.gastas == 3);  // busca, ficha, e a arte: nem uma a mais
  // O MESMO leitor que abastece o painel desde a issue #81 relê os octetos.
  CHECK(nu::arte_embutida(faixa) == rede.capa);
  CHECK(memoria.quantas() == 0);  // desfecho feliz não é negativo: nada se assenta
}

TEST_CASE("o 404 do CAA lembra-se e a corrida seguinte fica em casa") {
  const Cova cova;
  nu::MemoriaDeCapas memoria(cova.banco());
  RedeDeMentira rede;
  rede.desfecho_capa = nu::DesfechoMB::Falhou;  // o 404: capa não ha lá
  rede.estado_capa = 404;
  std::map<std::string, std::string> artes;
  std::set<std::string> sem_capa;
  const std::filesystem::path faixa = poe_mp3(cova, "01 - Never.mp3");
  CHECK(nu::caca_uma_faixa(faixa_de(faixa), &memoria, rede.consulta(), &artes,
                           &sem_capa) == nu::CacaDeCapa::SemCapa);
  CHECK(rede.gastas == 3);
  CHECK(nu::arte_embutida(faixa).empty());  // byte algum entrou na faixa
  // A corrida de amanhã relê a memoria do DISCO e fica em casa: zero rede,
  // que é o «não voltar a bater na rede por nada» da issue.
  nu::MemoriaDeCapas relida(cova.banco());
  std::map<std::string, std::string> artes2;
  std::set<std::string> sem2;
  CHECK(nu::caca_uma_faixa(faixa_de(faixa), &relida, rede.consulta(), &artes2,
                           &sem2) == nu::CacaDeCapa::JaProcurada);
  CHECK(rede.gastas == 3);
}

TEST_CASE("capa que já ha formato alheio e sem metadado ficam em casa") {
  const Cova cova;
  nu::MemoriaDeCapas memoria(cova.banco());
  RedeDeMentira rede;
  std::map<std::string, std::string> artes;
  std::set<std::string> sem_capa;
  // Com APIC na etiqueta: «já tinha», e a rede fica parada.
  const std::filesystem::path com_apic = poe_mp3(cova, "01 - Com.mp3");
  REQUIRE(nu::embute_arte(com_apic, rede.capa));
  CHECK(nu::caca_uma_faixa(faixa_de(com_apic), &memoria, rede.consulta(),
                           &artes, &sem_capa) == nu::CacaDeCapa::JaTinha);
  // Flac não se embute (RULINGS R7): fóra do alcance, sem rede e sem
  // memoria, que a decisão é local, grátis, e a issue do flac virá limpa.
  const std::filesystem::path flac = cova.raiz() / "02 - Alheia.flac";
  { std::ofstream(flac, std::ios::binary) << "fLaC"; }
  CHECK(nu::caca_uma_faixa(faixa_de(flac), &memoria, rede.consulta(), &artes,
                           &sem_capa) == nu::CacaDeCapa::ForaDoAlcance);
  CHECK_FALSE(memoria.ja_procurada(flac.string()));
  // Sem duração no índice: duvidosa SEM rede, e LEMBRADA (SPEC C4).
  nu::Faixa muda = faixa_de(poe_mp3(cova, "03 - Muda.mp3"));
  muda.duracao = 0;
  CHECK(nu::caca_uma_faixa(muda, &memoria, rede.consulta(), &artes, &sem_capa)
        == nu::CacaDeCapa::SemMetadado);
  CHECK(memoria.ja_procurada(muda.caminho));
  // E a capa AO LADO tambem é «já tinha»: é a que o operador governa.
  const Cova outra;
  const std::filesystem::path lado = poe_mp3(outra, "01 - Lado.mp3");
  { std::ofstream(outra.raiz() / "cover.jpg") << "arte ao lado"; }
  CHECK(nu::caca_uma_faixa(faixa_de(lado), &memoria, rede.consulta(), &artes,
                           &sem_capa) == nu::CacaDeCapa::JaTinha);
  CHECK(rede.gastas == 0);  // nenhuma das quatro tocou a rede
}

TEST_CASE("a corrida reusa a arte da release e conta a somma") {
  const Cova cova;
  nu::MemoriaDeCapas memoria(cova.banco());
  RedeDeMentira rede;
  std::vector<nu::Faixa> fila;
  fila.push_back(faixa_de(poe_mp3(cova, "01 - Uma.mp3")));
  fila.push_back(faixa_de(poe_mp3(cova, "02 - Outra.mp3")));
  const std::filesystem::path com = poe_mp3(cova, "03 - Com.mp3");
  REQUIRE(nu::embute_arte(com, rede.capa));
  fila.push_back(faixa_de(com));
  std::size_t linhas = 0;
  const nu::SommaDaCaca somma = nu::caca_capas(
      fila, &memoria, rede.consulta(),
      [&linhas](const nu::Faixa&, nu::CacaDeCapa) { ++linhas; });
  CHECK(somma.embutidas == 2);
  CHECK(somma.ja_tinham == 1);
  CHECK(linhas == 3);  // o relator viu a fila inteira
  // As irmãs casam na MESMA release e a arte baixou UMA vez: duas buscas,
  // duas fichas (o casamento é por faixa) e uma arte fazem cinco.
  CHECK(rede.gastas == 5);
  CHECK(nu::arte_embutida(fila[0].caminho) == rede.capa);
  CHECK(nu::arte_embutida(fila[1].caminho) == rede.capa);
}

TEST_CASE("o recuo a meio da corrida deixa as restantes virgens") {
  const Cova cova;
  nu::MemoriaDeCapas memoria(cova.banco());
  RedeDeMentira rede;
  rede.desfecho = nu::DesfechoMB::Recuo;
  rede.estado = 503;
  std::vector<nu::Faixa> fila;
  fila.push_back(faixa_de(poe_mp3(cova, "01 - Uma.mp3")));
  fila.push_back(faixa_de(poe_mp3(cova, "02 - Outra.mp3")));
  std::size_t linhas = 0;
  const nu::SommaDaCaca somma = nu::caca_capas(
      fila, &memoria, rede.consulta(),
      [&linhas](const nu::Faixa&, nu::CacaDeCapa) { ++linhas; });
  CHECK(somma.parou_por_recuo);
  CHECK(linhas == 1);      // a segunda nem entrou no relato
  CHECK(rede.gastas == 1);  // e rede alguma se gastou por ella
  CHECK(memoria.quantas() == 0);  // nada se lembrou: amanhã tenta-se
}

TEST_CASE("a memoria que recusa o assento apparece no relato") {
  const Cova cova;
  nu::MemoriaDeCapas memoria(cova.banco());
  REQUIRE(memoria.aberta());
  RedeDeMentira rede;
  rede.desfecho_capa = nu::DesfechoMB::Falhou;
  rede.estado_capa = 404;
  std::map<std::string, std::string> artes;
  std::set<std::string> sem_capa;
  const std::filesystem::path faixa = poe_mp3(cova, "01 - Never.mp3");
  // O directorio vira SÓ-LEITURA depois de tudo aberto: o INSERT precisa
  // criar o journal ao lado, e é ahi que o SQLite recusa, como recusaria
  // n'um disco cheio. É a falha que d'antes passava calada.
  namespace fs = std::filesystem;
  fs::permissions(cova.raiz(), fs::perms::owner_read | fs::perms::owner_exec,
                  fs::perm_options::replace);
  CHECK(nu::caca_uma_faixa(faixa_de(faixa), &memoria, rede.consulta(), &artes,
                           &sem_capa) == nu::CacaDeCapa::MemoriaNaoGravou);
  fs::permissions(cova.raiz(), fs::perms::owner_all,
                  fs::perm_options::replace);
  CHECK_FALSE(memoria.ja_procurada(faixa.string()));  // nada persistiu
  // Com o directorio de volta, o MESMO caminho assenta e sahe SemCapa.
  CHECK(nu::caca_uma_faixa(faixa_de(faixa), &memoria, rede.consulta(), &artes,
                           &sem_capa) == nu::CacaDeCapa::SemCapa);
  CHECK(memoria.ja_procurada(faixa.string()));
}

//   Da lavra do eminente Doutor BRAGA US. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
