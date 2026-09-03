// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA BIBLIOTHECA — testes/prova_biblioteca.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o ÍNDICE, e não o disco: nenhum caso d'este arquivo abre arquivo de
// audio, chama ffmpeg, ou toca taglib. As faixas entram amarradas á mão, o que
// deixa a prova das consultas independente da prova da varredura.
//
// E nenhum caso toca o índice do operador. Todo banco d'esta bateria nasce em
// directorio temporario proprio e morre com o caso; a ~/.local/share/mysong
// real não se lê nem se escreve, e é o C16 que o afere antes e depois.
//
// DOMÍNIO ......... bancos temporarios, enchidos linha a linha pelo caso.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... o alvo de cada asserção está ESCRIPTO no caso, e nunca se
//                   colhe da obra sob exame: prova que pergunta á obra o que a
//                   obra devia responder não prova cousa alguma.
#include <doctest/doctest.h>

#include <sqlite3.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#include "nucleo/biblioteca.hpp"

namespace nu = mysong::nucleo;

namespace {
// O caracter de substituição, por extenso e n'uma constante, porque colado a
// uma letra hexadecimal dentro de um literal elle seria lido como outro byte.
const std::string kTroca = "\xEF\xBF\xBD";

// Uma COVA de prova: directorio proprio, que nasce com o caso e morre com elle.
//
// É a peça que cumpre a prohibição, e não uma commodidade. Nenhum caso d'esta
// bateria toca ~/.local/share/mysong nem ~/Música: o caminho do banco entra por
// parametro, e é sómente por isso que uma corrida de prova não pode corromper o
// índice do operador. O nome leva o pid e um contador, para que duas corridas em
// paralello, ou dous casos do mesmo binario, não se pisem.
class Cova {
 public:
  Cova() {
    static int contador = 0;
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-prova-" + std::to_string(::getpid()) + "-" +
                std::to_string(++contador));
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
    std::filesystem::create_directories(caminho_, erro);
  }
  // Apaga SÓMENTE o caminho que este objecto construiu, e nunca por padrão
  // largo: apagar por padrão é como se apaga o que não era nosso.
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;

  const std::filesystem::path& raiz() const { return caminho_; }
  std::filesystem::path banco() const { return caminho_ / "indice.sqlite3"; }

 private:
  std::filesystem::path caminho_;
};

// Uma faixa armada á mão. Os campos que a prova não afere levam valores que
// variam com o numero, para que uma troca de columnas na gravação apparecesse
// como valor trocado, e não como dous zeros que se parecem.
nu::Faixa faz(const std::string& artista, const std::string& album,
              const std::string& titulo, int numero) {
  nu::Faixa faixa;
  faixa.caminho = "/acervo/" + artista + "/" + album + "/" + titulo + ".mp3";
  faixa.raiz = "/acervo";
  faixa.artista = artista;
  faixa.album = album;
  faixa.titulo = titulo;
  faixa.numero = numero;
  faixa.anno = 1800 + numero;
  faixa.duracao = 100 + numero;
  faixa.modificado = 1000 + numero;
  faixa.tamanho = 2000 + numero;
  return faixa;
}

// Lê o arquivo inteiro em bytes, para que a prova do abandono compare o índice
// antigo com elle mesmo, e não com uma contagem que a obra também produz.
std::string le_bytes(const std::filesystem::path& caminho) {
  std::ifstream fonte(caminho, std::ios::binary);
  return std::string(std::istreambuf_iterator<char>(fonte),
                     std::istreambuf_iterator<char>());
}

// Enche um banco com o mesmo acervo de quatro faixas, para que os casos das
// consultas afiram a ORDEM contra uma taboa que se lê aqui em cima.
bool enche(const std::filesystem::path& banco) {
  nu::Escriba escriba(banco);
  if (!escriba.aberto()) return false;
  return escriba.grava(faz("Ada Lovelace", "Máquina Analítica", "Tear", 3)) &&
         escriba.grava(faz("Ada Lovelace", "Máquina Analítica", "Nota G", 7)) &&
         escriba.grava(faz("Ada Lovelace", "Notas de Menabrea", "Traducção", 1)) &&
         escriba.grava(faz("Bach", "Cravo Bem Temperado", "Fuga", 2)) &&
         escriba.conclui();
}
}  // namespace

TEST_CASE("saneia_utf8 conserva o valido e troca o invalido") {
  CHECK(nu::saneia_utf8("") == "");
  CHECK(nu::saneia_utf8("Ada Lovelace") == "Ada Lovelace");
  CHECK(nu::saneia_utf8("Máquina Analítica") == "Máquina Analítica");
  CHECK(nu::saneia_utf8("\xF0\x9F\x8E\xB5") == "\xF0\x9F\x8E\xB5");
  // Um byte de arranque a que falta a continuação: cae elle, e o '(' fica.
  CHECK(nu::saneia_utf8("\xC3\x28") == kTroca + "(");
  CHECK(nu::saneia_utf8("\xFF") == kTroca);
  CHECK(nu::saneia_utf8("\xC3") == kTroca);
  // Latin-1 mal etiquetado: o 0xE9 do «é» não é UTF-8, e o resto sobrevive.
  CHECK(nu::saneia_utf8("Ada\xE9Lovelace") == "Ada" + kTroca + "Lovelace");
  // Sobrelongo, e metade de par substituto: forma boa, valor proibido.
  CHECK(nu::saneia_utf8("\xC0\xAF") == kTroca + kTroca);
  CHECK(nu::saneia_utf8("\xED\xA0\x80") == kTroca + kTroca + kTroca);
}

TEST_CASE("o escriba grava e a bibliotheca conta o que se gravou") {
  const Cova cova;
  {
    nu::Escriba escriba(cova.banco());
    REQUIRE(escriba.aberto());
    // O temporario existe, e o banco AINDA NÃO: é o coração da promessa.
    CHECK(std::filesystem::exists(escriba.temporario()));
    CHECK_FALSE(std::filesystem::exists(cova.banco()));
    CHECK(escriba.grava(faz("Ada Lovelace", "Máquina Analítica", "Tear", 3)));
    CHECK(escriba.grava(faz("Ada Lovelace", "Máquina Analítica", "Nota G", 7)));
    CHECK(escriba.grava(faz("Ada Lovelace", "Notas de Menabrea", "Traducção", 1)));
    CHECK(escriba.grava(faz("Bach", "Cravo Bem Temperado", "Fuga", 2)));
    REQUIRE(escriba.conclui());
    CHECK_FALSE(std::filesystem::exists(escriba.temporario()));
  }
  CHECK(std::filesystem::exists(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  REQUIRE(livraria.aberta());
  CHECK(livraria.versao() == nu::kVersaoDoEsquema);
  CHECK(livraria.total() == 4);
}

TEST_CASE("artistas e albuns sahem sem repetição e em ordem") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  // Duas faixas de Ada no mesmo album, e o nome d'ella sahe UMA vez.
  CHECK(livraria.artistas() ==
        std::vector<std::string>{"Ada Lovelace", "Bach"});
  CHECK(livraria.albuns("Ada Lovelace") ==
        std::vector<std::string>{"Máquina Analítica", "Notas de Menabrea"});
  CHECK(livraria.albuns("Bach") ==
        std::vector<std::string>{"Cravo Bem Temperado"});
  CHECK(livraria.albuns("Quem Não Existe").empty());
}

TEST_CASE("as faixas de um album sahem em ordem de numero") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  const std::vector<nu::Faixa> album =
      livraria.faixas_do_album("Ada Lovelace", "Máquina Analítica");
  REQUIRE(album.size() == 2);
  // Gravou-se «Tear» com o numero 3 e «Nota G» com o 7, e nesta ordem sahem.
  CHECK(album[0].titulo == "Tear");
  CHECK(album[0].numero == 3);
  CHECK(album[0].anno == 1803);
  CHECK(album[0].duracao == 103);
  CHECK(album[0].modificado == 1003);
  CHECK(album[0].tamanho == 2003);
  CHECK(album[0].deduzido == 0u);
  CHECK(album[1].titulo == "Nota G");
  CHECK(album[1].numero == 7);
  CHECK(livraria.faixas_do_album("Ada Lovelace", "Album Que Não Ha").empty());
}

TEST_CASE("a busca casa por pedaço de titulo") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  const std::vector<nu::Faixa> achadas = livraria.busca_faixa("ota");
  REQUIRE(achadas.size() == 1);
  CHECK(achadas[0].titulo == "Nota G");
  CHECK(achadas[0].artista == "Ada Lovelace");
  CHECK(livraria.busca_faixa("Tear").size() == 1);
  CHECK(livraria.busca_faixa("zzz").empty());
}

// O curinga do LIKE vindo do operador. Amarrar o valor não o neutraliza, donde
// esta prova enche um acervo em que UMA faixa traz por cento e sublinhado no
// titulo, e exige que buscar por elles ache essa e sómente essa. Sem o escape,
// «%» casa com tudo e a busca devolve o acervo inteiro.
TEST_CASE("o curinga do operador vale por si, e não por padrão") {
  const Cova cova;
  {
    nu::Escriba escriba(cova.banco());
    REQUIRE(escriba.aberto());
    REQUIRE(escriba.grava(faz("Ada Lovelace", "Máquina Analítica", "Tear", 3)));
    REQUIRE(escriba.grava(faz("Bach", "Cravo Bem Temperado", "Fuga", 2)));
    REQUIRE(escriba.grava(faz("Cauchy", "Cours", "100% Rigor_Puro", 1)));
    REQUIRE(escriba.conclui());
  }
  const nu::Biblioteca livraria(cova.banco());
  REQUIRE(livraria.total() == 3);
  const std::vector<nu::Faixa> por_cento = livraria.busca_faixa("%");
  REQUIRE(por_cento.size() == 1);
  CHECK(por_cento[0].titulo == "100% Rigor_Puro");
  const std::vector<nu::Faixa> sublinhado = livraria.busca_faixa("_");
  REQUIRE(sublinhado.size() == 1);
  CHECK(sublinhado[0].titulo == "100% Rigor_Puro");
  CHECK(livraria.busca_faixa("% Rigor_").size() == 1);
  CHECK(livraria.busca_faixa("%Rigor").empty());
  CHECK(livraria.busca_faixa("\\").empty());
}

// Nome de verdade: acento, aspa, apóstrofo, ponto e virgula e por cento. Tudo
// atravessa a gravação e as quatro consultas intacto, e a cadeia que se parece
// com commando não vira commando.
TEST_CASE("nome com acento, aspas e espaço atravessa intacto") {
  const Cova cova;
  const std::string artista = "Ária \"Ré\" à Noite";
  const std::string album = "L'Été; DROP TABLE faixas--";
  const std::string titulo = "Nº 1 «Prélude» 100%";
  {
    nu::Escriba escriba(cova.banco());
    REQUIRE(escriba.aberto());
    REQUIRE(escriba.grava(faz(artista, album, titulo, 5)));
    REQUIRE(escriba.conclui());
  }
  const nu::Biblioteca livraria(cova.banco());
  CHECK(livraria.total() == 1);
  CHECK(livraria.artistas() == std::vector<std::string>{artista});
  CHECK(livraria.albuns(artista) == std::vector<std::string>{album});
  const std::vector<nu::Faixa> faixas =
      livraria.faixas_do_album(artista, album);
  REQUIRE(faixas.size() == 1);
  CHECK(faixas[0].titulo == titulo);
  // O por cento do termo é TEXTO a procurar: acha esta faixa por ella o traser
  // no titulo, e não por curinga. O sublinhado não está no titulo, e não acha.
  CHECK(livraria.busca_faixa("100%").size() == 1);
  CHECK(livraria.busca_faixa("%").size() == 1);
  CHECK(livraria.busca_faixa("_").empty());
  CHECK(livraria.busca_faixa("N_").empty());
}

// Banco que ainda não existe NÃO é avaria: é o acervo que ainda não se varreu,
// e é o estado em que o operador acha o programma na primeira vez que o abre.
// Toda consulta responde vazio, e nenhuma lança pela borda.
TEST_CASE("banco ausente responde vazio, e não erro") {
  const Cova cova;
  REQUIRE_FALSE(std::filesystem::exists(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  CHECK_FALSE(livraria.aberta());
  CHECK(livraria.versao() == 0);
  CHECK(livraria.total() == 0u);
  CHECK(livraria.artistas().empty());
  CHECK(livraria.albuns("Ada Lovelace").empty());
  CHECK(livraria.faixas_do_album("Ada Lovelace", "Máquina Analítica").empty());
  CHECK(livraria.busca_faixa("Tear").empty());
  nu::Faixa achada;
  CHECK_FALSE(livraria.acha_por_caminho("/acervo/qualquer.mp3", achada));
  // E a consulta não CRIA o banco por consultar: o disco fica como estava.
  CHECK_FALSE(std::filesystem::exists(cova.banco()));
}

// Abandonar a meio. O índice antigo fica byte a byte como estava, e resíduo
// algum sobra: o destructor do Escriba desfaz o temporario que não se concluiu.
TEST_CASE("abandonar a meio conserva o índice antigo, e nada sobra") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const std::string antes = le_bytes(cova.banco());
  REQUIRE_FALSE(antes.empty());
  std::filesystem::path residuo;
  {
    nu::Escriba escriba(cova.banco());
    REQUIRE(escriba.aberto());
    residuo = escriba.temporario();
    CHECK(std::filesystem::exists(residuo));
    CHECK(escriba.grava(faz("Cauchy", "Cours", "Analyse", 1)));
    // e sahe-se do escopo SEM concluir
  }
  CHECK_FALSE(std::filesystem::exists(residuo));
  CHECK(le_bytes(cova.banco()) == antes);
  const nu::Biblioteca livraria(cova.banco());
  CHECK(livraria.total() == 4u);
  CHECK(livraria.busca_faixa("Analyse").empty());
}

// A prova dos DOUS FIOS da issue #69. O socket de commando lê a bibliotheca do
// fio do relogio, e o reabre() troca o punho do fio da tela: sem tranca, um fio
// consulta o banco que o outro acabou de fechar. As asserções correm DEPOIS do
// join, no fio da prova; durante a tormenta o juiz é o sanitizador de fios.
TEST_CASE("ler a bibliotheca emquanto outro fio a reabre não a parte") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  REQUIRE(livraria.aberta());

  std::atomic<bool> pare{false};
  std::atomic<long> lidas{0};
  std::thread leitor([&] {
    while (!pare.load()) {
      // QUATRO portas, e não uma: a tranca ha de valer para todas, e a que
      // ficasse de fóra appareceria aqui e sómente aqui.
      livraria.total();
      livraria.artistas();
      livraria.busca_faixa("a");
      if (livraria.aberta()) lidas.fetch_add(1);
    }
  });
  for (int volta = 0; volta < 1000; ++volta) livraria.reabre();
  pare.store(true);
  leitor.join();

  CHECK(lidas.load() > 0);   // o fio leitor correu de facto, e não passou ao lado
  CHECK(livraria.aberta());  // e o punho ficou de pé ao cabo de mil trocas
  CHECK(livraria.total() == 4);
}

TEST_CASE("mudar o titulo troca a linha, e a busca acha-a pelo nome novo") {
  Cova cova;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  const std::string qual = "/acervo/Bach/Cravo Bem Temperado/Fuga.mp3";
  CHECK(livraria.muda_o_titulo(qual, "Fuga em ré menor"));
  nu::Faixa depois;
  REQUIRE(livraria.acha_por_caminho(qual, depois));
  CHECK(depois.titulo == "Fuga em ré menor");
  // A busca lê a mesma columna, e é ella que a pauta mostra.
  REQUIRE(livraria.busca_faixa("ré menor").size() == 1u);
  CHECK(livraria.busca_faixa("Fuga em").front().caminho == qual);
  // Titulo vazio e caminho que não existe recusam-se, e nada se perde. Aparar
  // o branco não é d'aqui: quem apara é o renomeia_titulo, que é o unico logar
  // onde mora a regra do que é titulo.
  CHECK_FALSE(livraria.muda_o_titulo(qual, ""));
  CHECK_FALSE(livraria.muda_o_titulo("/acervo/nunca houve.mp3", "Fuga"));
  CHECK(livraria.total() == 4);
}

TEST_CASE("esquecer tira a faixa do índice, e sómente aquella") {
  Cova cova;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  const std::string qual = "/acervo/Ada Lovelace/Máquina Analítica/Tear.mp3";
  CHECK(livraria.esquece(qual));
  CHECK(livraria.total() == 3);
  nu::Faixa nenhuma;
  CHECK_FALSE(livraria.acha_por_caminho(qual, nenhuma));
  // O album fica, com a outra faixa d'elle: apagar uma não apaga a visinha.
  CHECK(livraria.faixas_do_album("Ada Lovelace", "Máquina Analítica").size() == 1u);
  // Esquecer o que já se esqueceu é falso, e não segunda baixa na conta.
  CHECK_FALSE(livraria.esquece(qual));
  CHECK(livraria.total() == 3);
}

// ─── A ORDEM PROPRIA DO ACERVO (issue #152) ─────────────────────────────────

namespace {
// Lavra á mão um índice do ESQUEMA VELHO, o da versão um, que não tem collunha
// de ordem. É o unico modo honesto de provar a migração: pedi-lo á obra de hoje
// daria o esquema de hoje, e a migração não teria o que migrar.
void banco_de_hontem(const std::filesystem::path& banco,
                     const std::vector<nu::Faixa>& faixas) {
  sqlite3* punho = nullptr;
  REQUIRE(sqlite3_open_v2(banco.c_str(), &punho,
                          SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                          nullptr) == SQLITE_OK);
  REQUIRE(sqlite3_exec(punho,
                       "CREATE TABLE esquema (versao INTEGER NOT NULL);"
                       "INSERT INTO esquema VALUES (1);"
                       "CREATE TABLE faixas (caminho TEXT PRIMARY KEY,"
                       " raiz TEXT NOT NULL, artista TEXT NOT NULL,"
                       " album TEXT NOT NULL, titulo TEXT NOT NULL,"
                       " numero INTEGER NOT NULL, anno INTEGER NOT NULL,"
                       " duracao INTEGER NOT NULL, modificado INTEGER NOT NULL,"
                       " tamanho INTEGER NOT NULL, deduzido INTEGER NOT NULL);",
                       nullptr, nullptr, nullptr) == SQLITE_OK);
  for (const nu::Faixa& faixa : faixas) {
    const std::string sql =
        "INSERT INTO faixas VALUES ('" + faixa.caminho + "','/acervo','" +
        faixa.artista + "','" + faixa.album + "','" + faixa.titulo + "'," +
        std::to_string(faixa.numero) + ",0,0,0,0,0);";
    REQUIRE(sqlite3_exec(punho, sql.c_str(), nullptr, nullptr, nullptr) ==
            SQLITE_OK);
  }
  sqlite3_close(punho);
}

// As ordens gravadas, na sequencia em que a obra as devolve. Contigua quer dizer
// que isto sahe 0, 1, 2, ... e o caso escreve o alvo á mão.
std::vector<std::int64_t> ordens(const nu::Biblioteca& livraria) {
  std::vector<std::int64_t> quaes;
  for (const std::string& caminho : livraria.ordem_das_faixas()) {
    nu::Faixa faixa;
    if (livraria.acha_por_caminho(caminho, faixa)) quaes.push_back(faixa.ordem);
  }
  return quaes;
}
}  // namespace

TEST_CASE("o banco de hontem migra e a ordem sahe como hontem se via") {
  const Cova cova;
  banco_de_hontem(cova.banco(),
                  {faz("Bach", "Cravo Bem Temperado", "Fuga", 2),
                   faz("Ada Lovelace", "Notas de Menabrea", "Traducção", 1),
                   faz("Ada Lovelace", "Máquina Analítica", "Tear", 3),
                   faz("Ada Lovelace", "Máquina Analítica", "Nota G", 7)});
  const nu::Biblioteca livraria(cova.banco());
  REQUIRE(livraria.aberta());
  CHECK(livraria.versao() == nu::kVersaoDoEsquema);
  // Artista, album, numero e titulo: a ordem que a tela mostrava hontem, e que
  // o acervo de quem já nos usa não ha de ver baralhada ao abrir.
  const std::vector<std::string> alvo = {
      "/acervo/Ada Lovelace/Máquina Analítica/Tear.mp3",
      "/acervo/Ada Lovelace/Máquina Analítica/Nota G.mp3",
      "/acervo/Ada Lovelace/Notas de Menabrea/Traducção.mp3",
      "/acervo/Bach/Cravo Bem Temperado/Fuga.mp3"};
  CHECK(livraria.ordem_das_faixas() == alvo);
  CHECK(ordens(livraria) == std::vector<std::int64_t>{0, 1, 2, 3});
}

TEST_CASE("mover ao principio, ao fim e ao meio deixa a ordem contigua") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  const std::vector<std::string> entrada = livraria.ordem_das_faixas();
  REQUIRE(entrada.size() == 4u);
  const std::vector<std::int64_t> contigua = {0, 1, 2, 3};
  const std::string fuga = entrada[3], tear = entrada[0];
  CHECK(livraria.move_faixa(fuga, 0));
  CHECK(livraria.ordem_das_faixas()[0] == fuga);
  CHECK(ordens(livraria) == contigua);
  CHECK(livraria.move_faixa(fuga, 3));
  CHECK(livraria.ordem_das_faixas()[3] == fuga);
  CHECK(ordens(livraria) == contigua);
  CHECK(livraria.move_faixa(tear, 2));
  CHECK(livraria.ordem_das_faixas()[2] == tear);
  CHECK(ordens(livraria) == contigua);
  // Mover para o logar em que já está é verdadeiro, e não mexe em nada.
  const std::vector<std::string> parada = livraria.ordem_das_faixas();
  CHECK(livraria.move_faixa(tear, 2));
  CHECK(livraria.ordem_das_faixas() == parada);
  CHECK_FALSE(livraria.move_faixa("/acervo/nunca/entrou.mp3", 0));
}

TEST_CASE("a faixa nova entra ao fim, a apagada fecha o buraco, e o disco guarda") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  const std::string fuga = livraria.ordem_das_faixas()[3];
  REQUIRE(livraria.move_faixa(fuga, 0));
  // A varredura RECONSTROE o índice: as que já lá estavam entram com o logar
  // d'ellas, e a que appareceu entra sem logar. É o caminho por onde a faixa
  // nova ha de cahir no fim sem que quem a grava lh'o diga.
  std::vector<nu::Faixa> velhas;
  for (const std::string& caminho : livraria.ordem_das_faixas()) {
    nu::Faixa d_ella;
    REQUIRE(livraria.acha_por_caminho(caminho, d_ella));
    velhas.push_back(d_ella);
  }
  {
    nu::Escriba escriba(cova.banco());
    REQUIRE(escriba.aberto());
    for (const nu::Faixa& d_ella : velhas) REQUIRE(escriba.grava(d_ella));
    REQUIRE(escriba.grava(faz("Zé", "Tarde", "Chegada", 1)));
    REQUIRE(escriba.conclui());
  }
  livraria.reabre();
  const std::string chegada = "/acervo/Zé/Tarde/Chegada.mp3";
  CHECK(livraria.ordem_das_faixas()[0] == fuga);
  CHECK(livraria.ordem_das_faixas()[4] == chegada);
  CHECK(ordens(livraria) == std::vector<std::int64_t>{0, 1, 2, 3, 4});
  // A que sae fecha o buraco d'ella, e as de baixo sobem uma.
  REQUIRE(livraria.esquece(livraria.ordem_das_faixas()[2]));
  CHECK(livraria.ordem_das_faixas()[0] == fuga);
  CHECK(livraria.ordem_das_faixas()[3] == chegada);
  CHECK(ordens(livraria) == std::vector<std::int64_t>{0, 1, 2, 3});
  // Fechado e reaberto o banco, a arrumação do operador está como elle a deixou.
  const std::vector<std::string> ficou = livraria.ordem_das_faixas();
  const nu::Biblioteca outra(cova.banco());
  CHECK(outra.ordem_das_faixas() == ficou);
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
