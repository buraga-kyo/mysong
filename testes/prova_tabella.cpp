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
#include "nucleo/rol.hpp"
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

// cursor_da_trilha — pinta a trilha n'um écran de PAPEL e devolve o cursor que o
// FTXUI lhe pôz. Terminal algum se abre, e é o ponto: o `Render` d'elle decide o
// cursor a cada quadro pelo nó focado do documento, e é essa decisão que se afere
// aqui, sem depender de olho que abriu a tela. `com_orla` embrulha o elemento no
// `vbox` e na `border` que a tela real lhe põe á volta.
ftxui::Screen::Cursor cursor_da_trilha(const std::string& trilha, bool digitando,
                                       std::size_t largura, bool com_orla,
                                       const std::string& sufixo = "") {
  ftxui::Element quadro =
      tui::elemento_da_trilha(trilha, sufixo, digitando, largura);
  if (com_orla) quadro = ftxui::vbox({quadro}) | ftxui::border;
  ftxui::Screen ecran =
      ftxui::Screen::Create(ftxui::Dimension::Fixed(static_cast<int>(largura)),
                            ftxui::Dimension::Fixed(com_orla ? 3 : 1));
  ftxui::Render(ecran, quadro);
  return ecran.cursor();
}

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
}


TEST_CASE("a trilha parada não pede cursor algum") {
  // Não pedindo o documento foco algum, o FTXUI põe `Hidden` a cada quadro e o
  // «ESC[?25h» não sae. Hoje isso vem de graça, porque logar algum d'esta obra
  // pedia foco; este caso é quem o guarda no dia em que a etiqueta do FTXUI
  // subir e o esconder se perder calado.
  const ftxui::Screen::Cursor parada = cursor_da_trilha("ARTISTS", false, 40, false);
  CHECK(parada.shape == ftxui::Screen::Cursor::Shape::Hidden);
}

TEST_CASE("a trilha a digitar põe a barra logo a seguir ao texto") {
  // «/ção» tem quatro collunhas e cinco bytes: contando bytes, o caret cahiria
  // uma collunha á direita do que se escreveu.
  const ftxui::Screen::Cursor caret = cursor_da_trilha("/ção", true, 40, false);
  CHECK(caret.shape == ftxui::Screen::Cursor::Shape::Bar);
  CHECK(caret.x == 4);
  CHECK(caret.y == 0);
}

TEST_CASE("o caret sobrevive ao vbox e á orla que a tela lhe põe á volta") {
  // O foco propaga-se de filho para pae; a tela real embrulha a trilha, e sem
  // esta prova a propagação ficaria por conta da leitura do codigo alheio.
  const ftxui::Screen::Cursor caret = cursor_da_trilha("/ção", true, 40, true);
  CHECK(caret.shape == ftxui::Screen::Cursor::Shape::Bar);
  CHECK(caret.x == 5);
  CHECK(caret.y == 1);
}

TEST_CASE("o caret não sae da tela com termo mais comprido que ella") {
  // Caret á direita da ultima collunha faz o FTXUI mandar deslocamento
  // NEGATIVO, que é escape mal formado a sahir para o terminal do operador.
  const ftxui::Screen::Cursor caret =
      cursor_da_trilha(std::string(80, 'x'), true, 20, false);
  CHECK(caret.shape == ftxui::Screen::Cursor::Shape::Bar);
  CHECK(caret.x < 20);
}

TEST_CASE("o aviso da rede não leva o caret do prompt comsigo") {
  // O aviso escreve-se em dezoito logares da janella e não se apaga nunca;
  // colado á trilha, elle punha o caret depois de palavras que ninguem
  // digitou, em TODO prompt da sessão a partir do primeiro recado. O appenso
  // entra á parte, e digitando-se o elemento cala-o.
  const ftxui::Screen::Cursor caret =
      cursor_da_trilha("/ção", true, 40, false, "   baixada: pronta");
  CHECK(caret.shape == ftxui::Screen::Cursor::Shape::Bar);
  CHECK(caret.x == 4);
  CHECK(caret.y == 0);
}

TEST_CASE("parada a trilha ainda mostra o appenso") {
  // Calar o appenso emquanto se digita não é perdê-lo: fechado o prompt, o
  // recado torna á linha. Lê-se cella a cella, como o resto d'esta prova.
  ftxui::Element quadro =
      tui::elemento_da_trilha("ARTISTS", "   [video: a.mkv]", false, 40);
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                              ftxui::Dimension::Fixed(1));
  ftxui::Render(ecran, quadro);
  std::string linha;
  for (int x = 0; x < 40; ++x) linha += ecran.PixelAt(x, 0).character;
  CHECK(linha.find("[video: a.mkv]") != std::string::npos);
}

TEST_CASE("os dous modos nunca medidos pousam o caret na collunha certa") {
  // O «I» (a playlist do Spotify) e o «R» (renomear) nunca passaram pelo pty;
  // o papel fixa-lhes a forma e o logar, com a MESMA linha que a janella compõe
  // para cada um.
  const ftxui::Screen::Cursor lista =
      cursor_da_trilha("PLAYLIST DO SPOTIFY: beat", true, 40, false);
  CHECK(lista.shape == ftxui::Screen::Cursor::Shape::Bar);
  CHECK(lista.x == 25);
  CHECK(lista.y == 0);
  const ftxui::Screen::Cursor nome =
      cursor_da_trilha("NOME: novo", true, 40, false);
  CHECK(nome.shape == ftxui::Screen::Cursor::Shape::Bar);
  CHECK(nome.x == 10);
  CHECK(nome.y == 0);
}

TEST_CASE("o glypho de duas collunhas não leva o caret para fóra da folga") {
  // O corte conta CODEPOINTS, e o glypho largo (CJK, emoji) conta por um
  // valendo duas collunhas: trinta d'elles pedem sessenta n'uma tela de
  // quarenta. Medido no papel, o hbox espreme o caret para a orla e elle pousa
  // em `largura` em ponto, UMA collunha além da ultima; é a folga de quatro do
  // pintor (`larg` é `col - 4`) que o guarda do escape negativo no terminal
  // de verdade. Crescer d'ahi é o que este caso recusa.
  std::string larga;
  for (int i = 0; i < 30; ++i) larga += "\u65e5";
  const ftxui::Screen::Cursor caret = cursor_da_trilha(larga, true, 40, false);
  CHECK(caret.shape == ftxui::Screen::Cursor::Shape::Bar);
  CHECK(caret.x <= 40);
  CHECK(caret.y == 0);
}

// ── A BARRA COM FOCO (issue #80) ────────────────────────────────────────────

namespace {

// pintar_barra — o écran de papel da BARRA, na largura fixa d'ella e nas
// fileiras que se pedir, lido cella a cella pela mesma razão do pintar da
// tabella. `altura` é a que a barra recebe (zero é «sem limite»); `fileiras` é o
// tamanho do papel, e as duas são cousas differentes de proposito: é assim que
// se afere que a barra NÃO passa da altura que lhe deram.
std::vector<std::string> pintar_barra(const tui::Navegador& navegador,
                                      bool com_foco, std::size_t degrau,
                                      std::size_t altura = 0,
                                      std::size_t fileiras = 7) {
  const int larg = static_cast<int>(tui::LARGURA_DA_BARRA);
  ftxui::Element quadro =
      tui::elemento_da_barra(navegador, com_foco, degrau, altura);
  ftxui::Screen ecran =
      ftxui::Screen::Create(ftxui::Dimension::Fixed(larg),
                            ftxui::Dimension::Fixed(static_cast<int>(fileiras)));
  ftxui::Render(ecran, quadro);
  std::vector<std::string> linhas;
  for (int y = 0; y < static_cast<int>(fileiras); ++y) {
    std::string linha;
    for (int x = 0; x < larg; ++x) linha += ecran.PixelAt(x, y).character;
    linhas.push_back(linha);
  }
  return linhas;
}

}  // namespace

TEST_CASE("com o foco na barra o dedo pinta-se n'uma fileira só") {
  const Cova cova;
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  // Sem lista alguma: o titulo, as minhas musicas, a risca, e os quatro.
  const std::vector<std::string> linhas = pintar_barra(navegador, true, 4);
  REQUIRE(linhas.size() == 7);
  CHECK(linhas[0] == " BIBLIOTECA         ");
  CHECK(linhas[1] == " MINHAS MÚSICAS     ");
  CHECK(linhas[3] == " ARTISTAS           ");
  CHECK(linhas[4] == " ÁLBUNS             ");
  CHECK(linhas[6] == "▸SPOTIFY            ");
  for (std::size_t i = 0; i < linhas.size(); ++i)
    if (i != 6) CHECK(linhas[i].find("▸") == std::string::npos);
}

TEST_CASE("sem foco a barra não tem dedo algum, e toda fileira mede vinte") {
  const Cova cova;
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);  // nasce nas MINHAS MÚSICAS
  for (const std::string& linha : pintar_barra(navegador, false, 3)) {
    CHECK(linha.find("▸") == std::string::npos);
    CHECK(escriptas(linha + "|") == tui::LARGURA_DA_BARRA + 1);
  }
  // E o dedo sobre a fileira corrente soma os dous signaes n'uma só.
  CHECK(pintar_barra(navegador, true, 0)[1] == "▸MINHAS MÚSICAS     ");
}

TEST_CASE("as listas do operador entram na barra pelo nome, na ordem do banco") {
  const Cova cova;
  nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(cova.listas());
  tui::Navegador navegador(livraria, &roleiro);
  REQUIRE(navegador.cria_rol("da noite"));
  REQUIRE(navegador.cria_rol("da manhã"));
  const std::vector<std::string> barra = pintar_barra(navegador, false, 0, 0, 9);
  REQUIRE(barra.size() == 9);
  CHECK(barra[1] == " MINHAS MÚSICAS     ");
  CHECK(barra[2] == " da manhã           ");
  CHECK(barra[3] == " da noite           ");
  CHECK(barra[5] == " ARTISTAS           ");  // depois da risca
}

TEST_CASE("a lista apparece, muda e some da barra no mesmo quadro") {
  const Cova cova;
  nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(cova.listas());
  tui::Navegador navegador(livraria, &roleiro);
  const auto tem = [&](const std::string& nome) {
    for (const std::string& linha : pintar_barra(navegador, false, 0, 0, 9))
      if (linha.find(nome) != std::string::npos) return true;
    return false;
  };
  REQUIRE(navegador.cria_rol("da manhã"));
  CHECK(tem("da manhã"));
  REQUIRE(navegador.renomeia_rol("de tarde"));
  CHECK_FALSE(tem("da manhã"));
  CHECK(tem("de tarde"));
  REQUIRE(navegador.apaga_rol());
  CHECK_FALSE(tem("de tarde"));
}

TEST_CASE("o nome comprido corta-se com reticencias, e a barra não alarga") {
  const Cova cova;
  nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(cova.listas());
  tui::Navegador navegador(livraria, &roleiro);
  REQUIRE(navegador.cria_rol(std::string(60, 'z')));
  const std::vector<std::string> barra = pintar_barra(navegador, false, 0, 0, 8);
  CHECK(barra[2] == " " + std::string(18, 'z') + "…");
  for (const std::string& linha : barra)
    CHECK(escriptas(linha + "|") == tui::LARGURA_DA_BARRA + 1);
}

TEST_CASE("a barra não passa da altura que lhe deram, e o eleito fica á vista") {
  const Cova cova;
  nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(cova.listas());
  tui::Navegador navegador(livraria, &roleiro);
  for (int i = 10; i < 60; ++i)
    REQUIRE(navegador.cria_rol("lista " + std::to_string(i)));
  // Altura de doze: as sete fileiras fixas e cinco listas, e nada mais. O papel
  // vae de vinte, que barra que passasse da altura appareceria n'elle.
  for (const std::size_t degrau : {std::size_t{1}, std::size_t{25},
                                   std::size_t{50}}) {
    const std::vector<std::string> barra =
        pintar_barra(navegador, true, degrau, 12, 20);
    std::size_t dedos = 0;
    for (std::size_t i = 0; i < barra.size(); ++i) {
      if (barra[i].find("▸") != std::string::npos) ++dedos;
      if (i >= 12) CHECK(escriptas(barra[i] + "|") == 1);  // fileira em branco
    }
    CHECK(dedos == 1);  // o degrau eleito está sempre á vista
  }
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
