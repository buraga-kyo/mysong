// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA ONDA — testes/prova_onda.cpp
// ══════════════════════════════════════════════════════════════════════════
// A conta da envolvente, a chave e o formato do cache, a dobra e a linha em
// écran de PAPEL. O ffmpeg não corre aqui: afere-se o argv que se HA DE
// correr. O caso VIVO dorme sem a variavel MYSONG_PROVA_MP3.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <chrono>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "nucleo/onda.hpp"
#include "tui/onda.hpp"
#include "tui/tokens.hpp"

namespace nu = mysong::nucleo;
namespace tk = mysong::tui::tokens;
namespace tui = mysong::tui;

namespace {

// seno — um seno de amplitude dada, com `por_volta` amostras por volta. A
// volta divide o balde de proposito: assim a RMS de cada balde é a mesma até
// ao ultimo bit, e a prova da normalização não depende de tolerancia larga.
std::vector<std::int16_t> seno(std::size_t quantas, double amplitude,
                               std::size_t por_volta) {
  std::vector<std::int16_t> amostras;
  amostras.reserve(quantas);
  for (std::size_t i = 0; i < quantas; ++i) {
    const double angulo = 2.0 * 3.14159265358979323846 *
                          static_cast<double>(i % por_volta) /
                          static_cast<double>(por_volta);
    amostras.push_back(static_cast<std::int16_t>(
        std::lround(amplitude * 32767.0 * std::sin(angulo))));
  }
  return amostras;
}

}  // namespace

// A RMS de um seno inteiro é a amplitude a dividir pela raiz de dous, e é a
// MESMA em todo balde: a onda de um tom constante sahe chata, e o máximo dá um.
TEST_CASE("o seno constante dá pontos eguaes, e o máximo dá um") {
  const std::vector<std::int16_t> amostras = seno(8192, 0.5, 8);
  const nu::Onda onda = nu::onda_das_amostras(amostras.data(), amostras.size(), 64);
  REQUIRE(onda.pronta());
  REQUIRE(onda.pontos.size() == 64);
  for (const float ponto : onda.pontos) CHECK(ponto == doctest::Approx(1.0));
}

// O silencio no MEIO ha de sahir no meio, e não espalhado: é o que faz a onda
// dizer onde a faixa cala. Metade calada, e a outra metade em pé.
TEST_CASE("o silencio no meio da faixa dá zeros no meio da onda") {
  std::vector<std::int16_t> amostras = seno(8192, 1.0, 8);
  for (std::size_t i = 2048; i < 6144; ++i) amostras[i] = 0;
  const nu::Onda onda = nu::onda_das_amostras(amostras.data(), amostras.size(), 64);
  REQUIRE(onda.pontos.size() == 64);
  for (std::size_t p = 0; p < 16; ++p) CHECK(onda.pontos[p] == doctest::Approx(1.0));
  for (std::size_t p = 16; p < 48; ++p) CHECK(onda.pontos[p] == 0.0f);
  for (std::size_t p = 48; p < 64; ++p) CHECK(onda.pontos[p] == doctest::Approx(1.0));
}

TEST_CASE("a linha de commando do ffmpeg sahe exacta, argumento a argumento") {
  CHECK(nu::linha_de_commando_da_onda("/casa/Musica/faixa.mp3") ==
        std::vector<std::string>{"ffmpeg", "-v", "error", "-nostdin", "-i",
                                 "/casa/Musica/faixa.mp3", "-vn", "-ac", "1",
                                 "-ar", "8000", "-f", "s16le", "pipe:1"});
}

// Amostra nenhuma NÃO é silencio: é ausencia, e a ausencia diz-se com a onda
// vazia, que é o que manda a fita pintar a barra chata em logar de mil zeros.
TEST_CASE("amostras nenhumas dão a onda vazia") {
  const std::int16_t nada[1] = {0};
  CHECK_FALSE(nu::onda_das_amostras(nada, 0).pronta());
  CHECK_FALSE(nu::onda_das_amostras(nullptr, 8).pronta());
  CHECK_FALSE(nu::onda_das_amostras(nada, 1, 0).pronta());
}

// A faixa de meio segundo tem menos amostras que baldes, e ainda assim ha de
// dar a conta INTEIRA de pontos: a fita conta collunhas sobre ella e vector
// curto faria a onda sahir pela metade da fita.
TEST_CASE("menos amostras que baldes ainda dá a conta inteira de pontos") {
  const std::vector<std::int16_t> poucas = {0, 16384, 32767, 16384, 0};
  const nu::Onda onda = nu::onda_das_amostras(poucas.data(), poucas.size(), 20);
  REQUIRE(onda.pontos.size() == 20);
  // Quatro baldes por amostra, que é o esticar: os quatro primeiros vêm da
  // primeira amostra, e o balde sem amostra propria repete o vizinho de traz.
  for (std::size_t p = 0; p < 4; ++p) CHECK(onda.pontos[p] == 0.0f);
  for (std::size_t p = 8; p < 12; ++p) CHECK(onda.pontos[p] == doctest::Approx(1.0));
  CHECK(onda.pontos[4] == onda.pontos[7]);
}

// Faixa calada dá zeros, e nunca divisão por zero nem NaN na tela.
TEST_CASE("a faixa toda calada dá zeros, e não divisão por zero") {
  const std::vector<std::int16_t> mudas(4096, 0);
  const nu::Onda onda = nu::onda_das_amostras(mudas.data(), mudas.size(), 32);
  REQUIRE(onda.pontos.size() == 32);
  for (const float ponto : onda.pontos) CHECK(ponto == 0.0f);
}

// A onda inteira, na conta que a Casa promette: mil e vinte e quatro pontos.
TEST_CASE("sem se pedir conta, a onda sahe com os mil e vinte e quatro pontos") {
  const std::vector<std::int16_t> amostras = seno(65536, 0.25, 8);
  const nu::Onda onda = nu::onda_das_amostras(amostras.data(), amostras.size());
  REQUIRE(onda.pontos.size() == nu::PONTOS_DA_ONDA);
  for (const float ponto : onda.pontos) {
    CHECK(ponto >= 0.0f);
    CHECK(ponto <= 1.0f);
  }
}

namespace {

// A COVA: um directorio temporario proprio, que morre com o caso. A bateria
// não escreve no acervo nem no cache do operador, e é esta classe que o
// garante: a prova da capa e a da lousa usam a mesma.
class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-onda-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  const std::filesystem::path& raiz() const { return caminho_; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

}  // namespace

// O mtime entra na chave porque o operador troca faixa no mesmo nome quando
// torna a baixar uma que sahiu cortada. Sem elle, a Casa mostraria para sempre
// a fórma da gravação velha.
TEST_CASE("a chave da onda muda quando o mtime muda") {
  const Cova cova;
  const std::filesystem::path faixa = cova.raiz() / "faixa.mp3";
  std::ofstream(faixa) << "isto não é mp3, mas tem tamanho e mtime";
  const std::string antes = nu::chave_da_onda(faixa);
  CHECK(antes.size() == 16);
  std::filesystem::last_write_time(
      faixa, std::filesystem::last_write_time(faixa) + std::chrono::hours(1));
  CHECK(nu::chave_da_onda(faixa) != antes);
}

namespace {

// A CASA DO CACHE trocada por uma cova, e devolvida no fim. A prova nunca
// escreve em `~/.cache/mysong`: o que ella lá deixasse ficaria depois da
// bateria, e cache do operador não é logar de lixo de prova.
class CachePostiço {
 public:
  explicit CachePostiço(const std::filesystem::path& onde) {
    const char* const antes = std::getenv("XDG_CACHE_HOME");
    havia_ = antes != nullptr;
    if (havia_) guardado_ = antes;
    ::setenv("XDG_CACHE_HOME", onde.c_str(), 1);
  }
  ~CachePostiço() {
    if (havia_)
      ::setenv("XDG_CACHE_HOME", guardado_.c_str(), 1);
    else
      ::unsetenv("XDG_CACHE_HOME");
  }
  CachePostiço(const CachePostiço&) = delete;
  CachePostiço& operator=(const CachePostiço&) = delete;

 private:
  bool havia_ = false;
  std::string guardado_;
};

}  // namespace

// `ondas/` ao lado de `capas/` e de `letreiro/`: apagar uma pasta não ha de
// levar a outra pelo caminho.
TEST_CASE("o caminho do cache pende do XDG_CACHE_HOME") {
  const Cova cova;
  const CachePostiço postiço(cova.raiz());
  CHECK(nu::caminho_da_onda_em_cache("abc123") ==
        cova.raiz() / "mysong" / "ondas" / "abc123.onda");
  // Chave vazia não dá caminho: sahiria o arquivo escondido «.onda», e duas
  // faixas sem chave cahiriam n'elle uma por cima da outra.
  CHECK(nu::caminho_da_onda_em_cache("").empty());
}

// O cyclo fechado: o que se escreve é o que se lê. A tolerancia é a da escala
// em que a onda se guarda, um octeto por ponto, e não folga arbitraria.
TEST_CASE("escrever e ler fecham o cyclo, ponto a ponto") {
  const Cova cova;
  nu::Onda onda;
  onda.pontos = {0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 0.125f};
  const std::filesystem::path onde = cova.raiz() / "fundo" / "faixa.onda";
  REQUIRE(nu::escreve_onda(onde, onda));
  // O directorio nasce com a escripta: quem escreve é quem cria.
  REQUIRE(std::filesystem::is_regular_file(onde));
  nu::Onda lida;
  REQUIRE(nu::le_onda(onde, &lida));
  REQUIRE(lida.pontos.size() == onda.pontos.size());
  for (std::size_t p = 0; p < onda.pontos.size(); ++p)
    CHECK(lida.pontos[p] == doctest::Approx(onda.pontos[p]).epsilon(0.004));
  // Temporario algum fica para traz: o rename leva-o inteiro.
  int quantos = 0;
  for (const auto& achado :
       std::filesystem::directory_iterator(onde.parent_path()))
    ++quantos, (void)achado;
  CHECK(quantos == 1);
}

// Onda vazia RECUSA-SE: guardar o nada faria a falha pegajosa, que a colheita
// seguinte acharia o arquivo, daria-o por bom e nunca mais chamaria o ffmpeg.
TEST_CASE("a onda vazia não se guarda, nem se lê de arquivo que não ha") {
  const Cova cova;
  CHECK_FALSE(nu::escreve_onda(cova.raiz() / "vazia.onda", nu::Onda{}));
  CHECK_FALSE(std::filesystem::exists(cova.raiz() / "vazia.onda"));
  nu::Onda lida;
  CHECK_FALSE(nu::le_onda(cova.raiz() / "não-ha.onda", &lida));
  CHECK_FALSE(nu::le_onda({}, &lida));
}

namespace {

// escreve_cru — o arquivo do cache lavrado Á MÃO, que é o que deixa aferir a
// RECUSA sem se correr o ffmpeg: cada maneira de o corromper põe-se de
// propósito, octeto por octeto.
std::filesystem::path escreve_cru(const Cova& cova, const std::string& nome,
                                  const std::string& conteudo) {
  const std::filesystem::path onde = cova.raiz() / nome;
  std::ofstream(onde) << conteudo;
  return onde;
}

}  // namespace

// Arquivo cortado ao meio ha de sahir como AUSENCIA, e nunca como onda pela
// metade: essa a tela pintaria sem desconfiar, e a fórma sahiria mentirosa.
TEST_CASE("o formato estranho é recusado, e não lido pela metade") {
  const Cova cova;
  nu::Onda lida;
  // A marca de outrem, e a versão que esta geração não escreveu.
  CHECK_FALSE(nu::le_onda(escreve_cru(cova, "a", "mysong-vento 1 2\n10 20\n"), &lida));
  CHECK_FALSE(nu::le_onda(escreve_cru(cova, "b", "mysong-onda 2 2\n10 20\n"), &lida));
  // A conta declarada que não bate: de menos e de mais.
  CHECK_FALSE(nu::le_onda(escreve_cru(cova, "c", "mysong-onda 1 3\n10 20\n"), &lida));
  CHECK_FALSE(nu::le_onda(escreve_cru(cova, "d", "mysong-onda 1 2\n10 20 30\n"), &lida));
  // Valor fóra da escala de um octeto, e cabeçalho que declara nada.
  CHECK_FALSE(nu::le_onda(escreve_cru(cova, "e", "mysong-onda 1 2\n10 300\n"), &lida));
  CHECK_FALSE(nu::le_onda(escreve_cru(cova, "f", "mysong-onda 1 0\n\n"), &lida));
  CHECK_FALSE(nu::le_onda(escreve_cru(cova, "g", ""), &lida));
  // E o que ESTÁ certo passa, senão o caso provaria sómente que tudo recusa.
  CHECK(nu::le_onda(escreve_cru(cova, "h", "mysong-onda 1 2\n0 255\n"), &lida));
  REQUIRE(lida.pontos.size() == 2);
  CHECK(lida.pontos[0] == 0.0f);
  CHECK(lida.pontos[1] == doctest::Approx(1.0));
}

// ── A PEÇA DA TELA. Écran de PAPEL, lido cella a cella: terminal algum se
// abre, e a onda arma-se á mão.

TEST_CASE("dobrar funde por máximo, e nunca amostra") {
  const std::vector<float> pontos = {0.1f, 0.9f, 0.2f, 0.8f};
  CHECK(tui::dobrar(pontos, 2) == std::vector<float>{0.9f, 0.8f});
  CHECK(tui::dobrar(pontos, 1) == std::vector<float>{0.9f});
  // Esticar é o máximo a degenerar em copia: quatro pontos em oito collunhas.
  CHECK(tui::dobrar(pontos, 8) ==
        std::vector<float>{0.1f, 0.1f, 0.9f, 0.9f, 0.2f, 0.2f, 0.8f, 0.8f});
  CHECK(tui::dobrar({}, 8).empty());
  CHECK(tui::dobrar(pontos, 0).empty());
}

// A largura EXACTA, de uma a duzentas: a fita conta collunhas sobre o que a
// dobra devolve, e um valor a menos deixaria a linha curta n'aquella largura
// só, que é o defeito que ninguem repara até o terminal ter aquelle tamanho.
TEST_CASE("dobrar fecha a largura exacta de uma a duzentas collunhas") {
  std::vector<float> pontos(nu::PONTOS_DA_ONDA);
  for (std::size_t p = 0; p < pontos.size(); ++p)
    pontos[p] = static_cast<float>(p % 17) / 16.0f;
  const float maior = 1.0f;
  for (std::size_t largura = 1; largura <= 200; ++largura) {
    const std::vector<float> dobrados = tui::dobrar(pontos, largura);
    REQUIRE(dobrados.size() == largura);
    // Pico algum some ao estreitar: o máximo da faixa sobrevive a toda dobra.
    CHECK(*std::max_element(dobrados.begin(), dobrados.end()) ==
          doctest::Approx(maior));
  }
}

namespace {

// papel — o écran de mentira, da largura pedida e de UMA fileira.
ftxui::Screen papel(ftxui::Element quadro, int largura) {
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(largura),
                                              ftxui::Dimension::Fixed(1));
  ftxui::Render(ecran, quadro);
  return ecran;
}

// pedaco — as cellas por GLIFO, e não por octeto: `substr` n'uma cadeia UTF-8
// contaria bytes, e o bloco de tres desalinharia todo indice depois do
// primeiro.
std::string pedaco(const ftxui::Screen& ecran, int x, int quantas) {
  std::string dita;
  for (int i = x; i < x + quantas && i < ecran.dimx(); ++i) {
    const std::string& glifo = ecran.PixelAt(i, 0).character;
    dita += glifo.empty() ? " " : glifo;
  }
  return dita;
}

// A onda dos oito degraus, do cheio ao raso, para se ler bloco a bloco.
const std::vector<float>& escada() {
  static const std::vector<float> kEscada = {1.0f,   0.875f, 0.75f, 0.5f,
                                             0.25f,  0.125f, 0.0f,  1.0f};
  return kEscada;
}

}  // namespace

// A LINHA INTEIRA contra uma cadeia escripta á mão: diz de uma vez a escada
// dos blocos, o piso de um oitavo e a conta das cellas.
TEST_CASE("a onda em écran de papel dá um bloco por cella, do cheio ao raso") {
  const ftxui::Screen tela =
      papel(tui::elemento_da_onda(escada(), 3.0, 8.0, 8, false), 8);
  CHECK(pedaco(tela, 0, 8) == "█▇▆▄▂▁▁█");
}

namespace {

ftxui::Color cor(std::string_view token) {
  const tk::Triade c = tk::rgb(token);
  return ftxui::Color::RGB(c.r, c.g, c.b);
}

}  // namespace

// A côr é o que diz por onde a faixa vae: v600 no andado, line_dim no que
// falta, e o fundo do painel por baixo de tudo.
TEST_CASE("a onda veste v600 no andado, e line_dim no que falta") {
  const ftxui::Screen tela =
      papel(tui::elemento_da_onda(escada(), 3.0, 8.0, 8, false), 8);
  for (int c = 0; c < 3; ++c) {
    CHECK(tela.PixelAt(c, 0).foreground_color == cor(tk::v600));
    CHECK(tela.PixelAt(c, 0).background_color == cor(tk::panel));
  }
  for (int c = 3; c < 8; ++c)
    CHECK(tela.PixelAt(c, 0).foreground_color == cor(tk::line_dim));
}

// O foco accende o ANDADO, e não a linha inteira: accendendo tudo, a onda
// deixaria de dizer por onde a faixa vae, que é o officio primeiro d'ella.
TEST_CASE("com foco, o andado accende em glow_core") {
  const ftxui::Screen tela =
      papel(tui::elemento_da_onda(escada(), 3.0, 8.0, 8, true), 8);
  CHECK(tela.PixelAt(0, 0).foreground_color == cor(tk::glow_core));
  CHECK(tela.PixelAt(2, 0).foreground_color == cor(tk::glow_core));
  CHECK(tela.PixelAt(3, 0).foreground_color == cor(tk::line_dim));
}

// Sem pontos, a barra chata do trilho, nas MESMAS côres: o progresso e o
// clique nunca hão de pender do ffmpeg, e machina que o não tenha vê a fita
// exactamente como via antes da onda.
TEST_CASE("sem pontos, a onda mostra a barra chata do trilho") {
  const ftxui::Screen tela = papel(tui::elemento_da_onda({}, 3.0, 8.0, 8, false), 8);
  CHECK(pedaco(tela, 0, 8) == "━━━━━━━━");
  CHECK(tela.PixelAt(0, 0).foreground_color == cor(tk::v600));
  CHECK(tela.PixelAt(7, 0).foreground_color == cor(tk::line_dim));
}

// Largura zero dá elemento VAZIO, e não `text("")`: o `text` do FTXUI pede
// sempre UMA linha, e a fita que pedisse meio de largura zero ganharia uma
// fileira do nada, que empurraria a lista uma linha para baixo.
TEST_CASE("largura zero dá elemento vazio, de altura zero") {
  ftxui::Element vazio = tui::elemento_da_onda(escada(), 3.0, 8.0, 0, false);
  vazio->ComputeRequirement();
  CHECK(vazio->requirement().min_x == 0);
  CHECK(vazio->requirement().min_y == 0);
}

// A caixa por pintar nasce VAZIA, e não contendo o canto (0,0): a caixa de
// fabrica do FTXUI contém-o, e clique no canto da tela cahiria na onda que
// ainda se não pintou.
TEST_CASE("a caixa da onda nasce vazia, e recebe o reflect ao pintar") {
  ftxui::Box caixa = {0, 0, 0, 0};
  ftxui::Element nada = tui::elemento_da_onda(escada(), 0.0, 8.0, 0, false, &caixa);
  CHECK(caixa.x_max < caixa.x_min);
  (void)nada;
  papel(tui::elemento_da_onda(escada(), 3.0, 8.0, 8, false, &caixa), 8);
  CHECK(caixa.x_min == 0);
  CHECK(caixa.x_max == 7);
  CHECK(caixa.y_min == 0);
  CHECK(caixa.y_max == 0);
}

// Duração que não é tempo não enche cousa alguma: o andado cinge-se em zero e
// a onda sahe toda apagada, sem divisão por zero pelo caminho.
TEST_CASE("sem duração, a onda sahe toda por andar") {
  const ftxui::Screen tela =
      papel(tui::elemento_da_onda(escada(), 0.0, 0.0, 8, false), 8);
  for (int c = 0; c < 8; ++c)
    CHECK(tela.PixelAt(c, 0).foreground_color == cor(tk::line_dim));
}

// ── O CASO VIVO. Corre o ffmpeg de verdade sobre um mp3 de verdade, e por
// isso DORME sem `MYSONG_PROVA_MP3`: a bateria da Casa não ha de pender de
// programa alheio nem de arquivo que nem toda machina tem. `MYSONG_PROVA_DUMP`
// diz o directorio em que se deixa o cache postiço e o dump da linha pintada.
TEST_CASE("com um mp3 de verdade, a onda nasce em cache e dispensa o ffmpeg") {
  const char* const faixa = std::getenv("MYSONG_PROVA_MP3");
  if (faixa == nullptr || faixa[0] == '\0') return;
  const char* const posto = std::getenv("MYSONG_PROVA_DUMP");
  const Cova cova;
  const std::filesystem::path casa =
      posto != nullptr && posto[0] != '\0' ? std::filesystem::path(posto) / "cache"
                                           : cova.raiz();
  const CachePostiço postiço(casa);

  std::string razao;
  const nu::Onda primeira = nu::colhe_onda(faixa, &razao);
  INFO("razão: " << razao);
  REQUIRE(primeira.pronta());
  CHECK(razao.empty());
  CHECK(primeira.pontos.size() == nu::PONTOS_DA_ONDA);
  for (const float ponto : primeira.pontos) {
    CHECK(ponto >= 0.0f);
    CHECK(ponto <= 1.0f);
  }
  const std::filesystem::path guardada =
      nu::caminho_da_onda_em_cache(nu::chave_da_onda(faixa));
  CHECK(std::filesystem::is_regular_file(guardada));

  // A PROVA de que a segunda colheita não corre o ffmpeg, e não é o relogio
  // que a dá: tira-se o programa do PATH. Vindo a onda á mesma, ella veio do
  // cache, que é o unico logar que sobrou. A tolerancia é a da escala em que
  // o cache guarda, um octeto por ponto: a primeira vem da conta em float, a
  // segunda vem do arquivo, e egualdade exacta seria promessa falsa.
  const std::string caminho_de_antes =
      std::getenv("PATH") != nullptr ? std::getenv("PATH") : "";
  ::setenv("PATH", (cova.raiz() / "sem-programa").c_str(), 1);
  const nu::Onda segunda = nu::colhe_onda(faixa, &razao);
  ::setenv("PATH", caminho_de_antes.c_str(), 1);
  REQUIRE(segunda.pronta());
  CHECK(razao.empty());
  REQUIRE(segunda.pontos.size() == primeira.pontos.size());
  for (std::size_t p = 0; p < primeira.pontos.size(); ++p)
    CHECK(segunda.pontos[p] == doctest::Approx(primeira.pontos[p]).epsilon(0.004));

  if (posto == nullptr || posto[0] == '\0') return;
  // O DUMP da linha a oitenta collunhas, cru, com os escapes dentro: é o que o
  // olho ha de julgar, e o que a limpeza por `sed` torna legivel.
  const ftxui::Screen tela =
      papel(tui::elemento_da_onda(segunda.pontos, 19.0, 189.0, 80, false), 80);
  std::ofstream(std::filesystem::path(posto) / "onda-80.cru") << tela.ToString();
}
