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
#include <ftxui/screen/string.hpp>

#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "nucleo/biblioteca.hpp"
#include "nucleo/rol.hpp"
#include "tui/navegador.hpp"
#include "tui/tabella.hpp"
#include "tui/tokens.hpp"

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

nu::Achado achado(const std::string& titulo, const std::string& canal,
                  int duracao) {
  nu::Achado feito;
  feito.titulo = titulo;
  feito.canal = canal;
  feito.duracao = duracao;
  feito.url = "https://y/" + titulo;
  return feito;
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
  // O banco das LISTAS, á parte do índice, para a barra da bibliotheca as ler.
  std::filesystem::path listas() const { return caminho_ / "rol.sqlite3"; }

 private:
  std::filesystem::path caminho_;
};

}  // namespace

TEST_CASE("o corte da pauta conta CELLAS, e o glypho largo vale duas") {
  // Cabendo, sahe INTEIRO e enchido: a columna promette largura fixa.
  CHECK(tui::apara_collunhas("Fuga", 8) == "Fuga    ");
  CHECK(ftxui::string_width(tui::apara_collunhas("Fuga", 8)) == 8);
  // Não cabendo, a ultima cella leva a reticencia, e a conta bate na cella.
  CHECK(tui::apara_collunhas("Toccata e Fuga", 8) == "Toccata\u2026");
  CHECK(ftxui::string_width(tui::apara_collunhas("Toccata e Fuga", 8)) == 8);
  // O CJK toma DUAS cellas por glypho: cinco glyphos pedem dez, e em oito
  // cabem tres (seis cellas) mais a reticencia. Contando por codepoint, sete
  // d'elles «cabiam» em oito e a linha sahia com seis cellas a mais.
  const std::string cjk = "\u6771\u4eac\u97f3\u697d\u796d";  // 5 glyphos, 10 cellas
  CHECK(ftxui::string_width(cjk) == 10);
  const std::string cortado = tui::apara_collunhas(cjk, 8);
  CHECK(ftxui::string_width(cortado) == 8);
  CHECK(cortado == "\u6771\u4eac\u97f3\u2026 ");
  // Largura IMPAR deixa uma cella orphã antes do «…»: enche-se de espaço, e a
  // conta continua exacta. Sem o enchimento a columna encolhia uma cella.
  CHECK(ftxui::string_width(tui::apara_collunhas(cjk, 7)) == 7);
  CHECK(ftxui::string_width(tui::apara_collunhas(cjk, 3)) == 3);
  CHECK(tui::apara_collunhas("Fuga", 0).empty());
}

namespace {

// cellas_dos — a somma das larguras dos pedaços. É o invariante da pauta: os
// vãos e as margens tambem são pedaços, d'onde a somma HA DE dar a largura.
std::size_t cellas_dos(const std::vector<tui::Pedaco>& pedacos) {
  std::size_t total = 0;
  for (const tui::Pedaco& pedaco : pedacos)
    total += static_cast<std::size_t>(ftxui::string_width(pedaco.texto));
  return total;
}

std::string dito(const std::vector<tui::Pedaco>& pedacos) {
  std::string feita;
  for (const tui::Pedaco& pedaco : pedacos) feita += pedaco.texto;
  return feita;
}

tui::Linha faixa_de(const std::string& texto, const std::string& autor,
                    int numero, int duracao) {
  tui::Linha feita;
  feita.texto = texto;
  feita.autor = autor;
  feita.numero = numero;
  feita.duracao = duracao;
  return feita;
}

}  // namespace

TEST_CASE("a somma das columnas da linha É a largura da pauta") {
  const tui::Linha curta = faixa_de("Fuga", "A", 4, 96);
  // Titulo de cento e vinte caracteres, e titulo em CJK: os dous casos que
  // empurravam as columnas. A somma ha de dar a largura em todos.
  const tui::Linha comprida = faixa_de(std::string(120, 'x'), "Canal", 12, 542);
  const tui::Linha larga =
      faixa_de("\u6771\u4eac\u97f3\u697d\u796d", "\u4e2d\u6587", 7, 200);
  for (const std::size_t quanto : {10u, 12u, 20u, 30u, 40u, 59u, 83u, 167u}) {
    for (const tui::Linha& qual : {curta, comprida, larga}) {
      const tui::Medidas faixas = tui::medidas_da_pauta(quanto, true, false);
      CHECK(cellas_dos(tui::pedacos_da_linha(qual, faixas, 542, true)) == quanto);
      const tui::Medidas nomes = tui::medidas_da_pauta(quanto, true, true);
      CHECK(cellas_dos(tui::pedacos_da_linha(qual, nomes, 12, false)) == quanto);
    }
  }
}

TEST_CASE("a linha da pauta diz numero, titulo, artista, régua e tempo") {
  const tui::Medidas medidas = tui::medidas_da_pauta(59, true, false);
  const tui::Linha qual =
      faixa_de("Montagem Lunar Celestia 1.0", "TOKYOPHILE", 4, 96);
  const std::string linha = dito(tui::pedacos_da_linha(qual, medidas, 267, false));
  CHECK(linha.substr(0, 5) == "    4");  // a cella do ▶ vazia, e o № á direita
  // O titulo de vinte e sete cellas n'uma columna de vinte e quatro: corta-se,
  // e a reticencia diz que se cortou.
  CHECK(linha.find("Montagem Lunar Celestia\u2026") != std::string::npos);
  CHECK(linha.find("TOKYOPHILE") != std::string::npos);
  // Noventa e seis segundos de duzentos e sessenta e sete: duas cellas de seis.
  CHECK(linha.find("\u25b0\u25b0\u25b1\u25b1\u25b1\u25b1") != std::string::npos);
  CHECK(linha.find("01:36") != std::string::npos);
  // A que SÔA leva o «▶» na cella d'elle, e NÃO perde o numero.
  const std::vector<tui::Pedaco> soando =
      tui::pedacos_da_linha(qual, medidas, 267, true);
  CHECK(dito(soando).substr(0, 7) == " \u25b6  4");
  namespace tk = mysong::tui::tokens;
  for (const tui::Pedaco& pedaco : soando)
    if (pedaco.negrito) CHECK(pedaco.tinta == tk::glow_soft);
  // A vista que CONTA nomes: o nome, a conta de faixas, e a régua pela conta.
  const tui::Medidas nomes = tui::medidas_da_pauta(40, false, true);
  const std::string nome =
      dito(tui::pedacos_da_linha(faixa_de("MXZI", "", 12, 0), nomes, 24, false));
  CHECK(nome.find("MXZI") != std::string::npos);
  CHECK(nome.find("  12") != std::string::npos);
  CHECK(nome.find("\u25b0\u25b0\u25b0\u25b1\u25b1\u25b1") != std::string::npos);
}

TEST_CASE("as columnas da pauta cedem por ordem de serviço") {
  // A metade esquerda de uma tela de 167 collunhas: abrem-se todas.
  const tui::Medidas larga = tui::medidas_da_pauta(83, true, false);
  CHECK(larga.marcador == 1);
  CHECK(larga.numero == 3);
  CHECK(larga.regua == 6);
  CHECK(larga.conta == 5);  // MM:SS
  CHECK(larga.artista == 19);
  CHECK(larga.titulo == 40);
  CHECK(larga.titulo >= 2 * larga.artista);  // dous terços contra um
  // A ESTREITA cede o ARTISTA, e mais nada: elle é o primeiro a ceder.
  const tui::Medidas media = tui::medidas_da_pauta(40, true, false);
  CHECK(media.artista == 0);
  CHECK(media.regua == 6);
  CHECK(media.titulo == 18);
  // Depois d'elle cede a RÉGUA, e depois o TEMPO.
  CHECK(tui::medidas_da_pauta(30, true, false).regua == 6);
  CHECK(tui::medidas_da_pauta(29, true, false).regua == 0);
  CHECK(tui::medidas_da_pauta(20, true, false).conta == 0);
  // A pauta MINIMA: as duas margens e o titulo. Nem o marcador cabe.
  const tui::Medidas minima = tui::medidas_da_pauta(10, true, false);
  CHECK(minima.marcador == 0);
  CHECK(minima.titulo == 8);
  // A vista que CONTA nomes não tem № nem artista: o que ella conta vae na
  // columna da direita, em quatro cellas, e a régua mede-se por elle.
  const tui::Medidas conta = tui::medidas_da_pauta(83, true, true);
  CHECK(conta.numero == 0);
  CHECK(conta.artista == 0);
  CHECK(conta.conta == 4);
  CHECK(conta.titulo == 67);
}

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
  navegador.mostra_rede({achado(comprido, {}, 100)});
  const std::vector<std::string> sem = pintar(navegador, 1, 60);
  navegador.mostra_rede({achado(comprido, "Canal", 100)});
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
  navegador.mostra_rede({achado(comprido, {}, 100),
                         achado("Fuga", "Canal do Orgao", 65)});
  const std::vector<std::string> linhas = pintar(navegador, 2, 60);
  REQUIRE(linhas.size() == 2);
  CHECK(linhas[1].find("Canal do Orgao") != std::string::npos);
  CHECK(escriptas(linhas[0]) == escriptas(linhas[1]));
}

TEST_CASE("o recado do vazio é por SECÇÃO, e não um para todas") {
  Cova cova;
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);  // acervo vazio, e sem roleiro
  REQUIRE(navegador.vai_para(tui::Secao::Artistas));
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

  // Nas MINHAS MÚSICAS o recado depende do TERMO: sem elle, quem está vazio é o
  // acervo, e mandar procurar erro de escripta seria mandar ao logar errado.
  REQUIRE(navegador.vai_para(tui::Secao::Busca));
  CHECK(pintar(navegador, 1, 70)[0].find("varra o acervo") != std::string::npos);
  navegador.filtra("zzz");
  CHECK(pintar(navegador, 1, 70)[0].find("esse termo") != std::string::npos);
}


namespace {

// com_som — a tabella com uma faixa a sôar, no écran de papel: aqui lê-se
// tambem a CÔR da cella, que os dous signaes se distinguem pela tinta.
ftxui::Screen com_som(const tui::Navegador& navegador,
                      const std::string& tocando) {
  ftxui::Element quadro = tui::elemento_da_tabella(navegador, 0, 2, 60, tocando);
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                                              ftxui::Dimension::Fixed(2));
  ftxui::Render(ecran, quadro);
  return ecran;
}

ftxui::Color cor(std::string_view token) {
  const mysong::tui::tokens::Triade c = mysong::tui::tokens::rgb(token);
  return ftxui::Color::RGB(c.r, c.g, c.b);
}

}  // namespace

TEST_CASE("a faixa que sôa accende com signal proprio ao lado do da eleita") {
  const Cova cova;
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  navegador.mostra_rede({achado("Toccata", "Canal", 542),
                         achado("Fuga", "Outro", 65)});
  navegador.desce();  // a ELEITA passa a ser a segunda
  namespace tk = mysong::tui::tokens;
  // Sôa a PRIMEIRA, e a eleita é a segunda: dous signaes em linhas differentes.
  const ftxui::Screen dous = com_som(navegador, "https://y/Toccata");
  CHECK(dous.PixelAt(1, 0).character == "▶");
  CHECK(dous.PixelAt(0, 0).foreground_color == cor(tk::glow_core));
  CHECK(dous.PixelAt(0, 1).background_color == cor(tk::v900));
  // E o «▶» NÃO empurra o titulo: elle toma o logar do numero, e a columna do
  // titulo cahe na mesma collunha com signal e sem elle.
  CHECK(dous.PixelAt(4, 0).character == "T");
  CHECK(dous.PixelAt(4, 1).character == "F");
  // Sôa a MESMA que está eleita: o fundo é o v900 e a tinta o glow_core.
  const ftxui::Screen um = com_som(navegador, "https://y/Fuga");
  CHECK(um.PixelAt(1, 1).character == "▶");
  CHECK(um.PixelAt(0, 1).background_color == cor(tk::v900));
  CHECK(um.PixelAt(0, 1).foreground_color == cor(tk::glow_core));
  // Caminho que não casa com chave alguma não accende linha nenhuma.
  const ftxui::Screen nada = com_som(navegador, "/musica/outra.mp3");
  CHECK(nada.PixelAt(1, 0).character != "▶");
  CHECK(nada.PixelAt(1, 1).character != "▶");
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
