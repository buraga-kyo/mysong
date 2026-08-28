// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA TABELLA — testes/prova_tabella.cpp
// ══════════════════════════════════════════════════════════════════════════
// Pinta a tabella num écran de PAPEL, de largura escolhida, e lê os bytes que
// sahiram. Terminal algum se abre. O que se afere é o alinhamento das columnas,
// que é o defeito que a issue #37 ensinou a temer: a linha do transporte
// transbordava, e o olho de quem a abriu não o accusou.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "nucleo/biblioteca.hpp"
#include "tui/navegador.hpp"
#include "tui/tabella.hpp"

namespace nu = mysong::nucleo;
namespace tui = mysong::tui;

namespace {

// pintar — o écran de papel, lido CELLA A CELLA. Não se lê o `ToString`, e é de
// proposito: elle mette as sequencias de escape no meio dos bytes, e contar bytes
// daria comprimentos differentes para linhas de egual largura, que a linha eleita
// traz mais tinta que as outras. Contar byte seria contar a tinta. Perguntando ao
// écran cella por cella, o que se conta é collunha, que é o que a tabella promette.
std::vector<std::string> pintar(const tui::Navegador& navegador,
                                std::size_t altura, std::size_t largura) {
  ftxui::Element quadro = tui::elemento_da_tabella(navegador, 0, altura, largura);
  ftxui::Screen ecran =
      ftxui::Screen::Create(ftxui::Dimension::Fixed(static_cast<int>(largura)),
                            ftxui::Dimension::Fixed(static_cast<int>(altura)));
  ftxui::Render(ecran, quadro);
  std::vector<std::string> linhas;
  for (int y = 0; y < static_cast<int>(altura); ++y) {
    std::string linha;
    for (int x = 0; x < static_cast<int>(largura); ++x)
      linha += ecran.PixelAt(x, y).character;
    linhas.push_back(linha);
  }
  return linhas;
}

// aparadas — as collunhas de uma linha sem o enchimento de espaços da direita. É o
// que diz onde a tabella deixou de escrever, e é isso que se compara entre linhas.
std::size_t escriptas(const std::string& linha) {
  std::size_t fim = linha.size();
  while (fim > 0 && linha[fim - 1] == ' ') --fim;
  std::size_t conta = 0;
  for (std::size_t i = 0; i < fim; ++i)
    if ((static_cast<unsigned char>(linha[i]) & 0xC0) != 0x80) ++conta;
  return conta;
}

tui::Linha achado(const std::string& titulo, const std::string& canal,
                  int duracao) {
  return {titulo, "https://y/" + titulo, 0, duracao, canal};
}

// A cova e o índice: a tabella pede um Navegador, e o Navegador pede uma
// Bibliotheca. O acervo fica VAZIO de proposito: os casos abaixo põem a vista por
// mostra_rede, e acervo enchido só juntaria ruido ao que se lê.
class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-tab-" + std::to_string(::getpid()));
    std::filesystem::create_directories(caminho_);
    nu::Escriba escriba(caminho_ / "indice.sqlite3");
    escriba.conclui();
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  std::filesystem::path banco() const { return caminho_ / "indice.sqlite3"; }

 private:
  std::filesystem::path caminho_;
};

}  // namespace

TEST_CASE("a columna do canal apparece havendo autor, e o tempo fica á direita") {
  Cova cova;
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  navegador.mostra_rede({achado("Toccata", "Canal do Orgao", 542),
                         achado("Fuga", "Outro Canal", 65)});
  const std::vector<std::string> linhas = pintar(navegador, 2, 60);
  REQUIRE(linhas.size() == 2);
  CHECK(linhas[0].find("Toccata") != std::string::npos);
  CHECK(linhas[0].find("Canal do Orgao") != std::string::npos);
  CHECK(linhas[0].find("09:02") != std::string::npos);
  CHECK(linhas[1].find("01:05") != std::string::npos);
}

TEST_CASE("as duas linhas sahem com o MESMO comprimento, e não transbordam") {
  Cova cova;
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  // Titulo comprido e canal comprido na primeira, curtos na segunda: é o par que
  // desalinha as columnas quando a largura da columna se decide por LINHA.
  navegador.mostra_rede(
      {achado("Toccata e Fuga em Re menor BWV 565 completa ao vivo",
              "Canal do Orgao de Tubos de Freiberg", 542),
       achado("Fuga", "A", 65)});
  const std::vector<std::string> linhas = pintar(navegador, 2, 60);
  REQUIRE(linhas.size() == 2);
  // O TEMPO acaba as duas linhas, e por isso a ultima collunha escripta é a mesma.
  // Sem isto, a columna do tempo da linha comprida cahia depois da da linha curta.
  CHECK(escriptas(linhas[0]) == escriptas(linhas[1]));
  CHECK(escriptas(linhas[0]) <= 60);
}

TEST_CASE("sem autor algum a columna do canal não se abre, e o titulo fica largo") {
  Cova cova;
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  // Titulo de setenta e cinco caracteres. Sem columna de canal, o que sobra para
  // elle é maior, e por isso corta-se mais tarde: é o que este caso lê.
  const std::string comprido(75, 'x');
  navegador.mostra_rede({{comprido, "https://y/1", 0, 100, {}}});
  const std::vector<std::string> sem = pintar(navegador, 1, 60);
  navegador.mostra_rede({{comprido, "https://y/1", 0, 100, "Canal"}});
  const std::vector<std::string> com = pintar(navegador, 1, 60);
  const auto quantos_x = [](const std::vector<std::string>& onde) {
    std::size_t conta = 0;
    for (const std::string& linha : onde)
      for (const char c : linha)
        if (c == 'x') ++conta;
    return conta;
  };
  CHECK(quantos_x(sem) > quantos_x(com));
}

TEST_CASE("basta UMA linha com autor na fatia para a columna se abrir") {
  Cova cova;
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  // A PRIMEIRA linha não tem autor, a segunda tem. Decidindo-se pela primeira, a
  // columna ficava fechada e o canal da segunda não apparecia; decidindo-se pela
  // FATIA, ella abre-se para as duas e as columnas alinham.
  const std::string comprido(75, 'x');
  navegador.mostra_rede({{comprido, "https://y/1", 0, 100, {}},
                         {"Fuga", "https://y/2", 0, 65, "Canal do Orgao"}});
  const std::vector<std::string> linhas = pintar(navegador, 2, 60);
  REQUIRE(linhas.size() == 2);
  CHECK(linhas[1].find("Canal do Orgao") != std::string::npos);
  CHECK(escriptas(linhas[0]) == escriptas(linhas[1]));
}

TEST_CASE("o recado do vazio é por SECÇÃO, e não um para todas") {
  Cova cova;
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);  // acervo vazio, e sem roleiro
  // No acervo, o recado manda varrer. Dentro de uma lista de faixas escolhidas á
  // mão, mandar varrer o acervo seria mandar o operador ao logar errado.
  const std::vector<std::string> acervo = pintar(navegador, 1, 70);
  CHECK(acervo[0].find("varra o acervo") != std::string::npos);

  navegador.mostra_rois();
  const std::vector<std::string> listas = pintar(navegador, 1, 70);
  CHECK(listas[0].find("cria uma") != std::string::npos);
  CHECK(listas[0].find("varra o acervo") == std::string::npos);

  navegador.mostra_rede(std::vector<nu::Achado>{});
  const std::vector<std::string> rede = pintar(navegador, 1, 70);
  CHECK(rede[0].find("pergunta outra vez") != std::string::npos);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
