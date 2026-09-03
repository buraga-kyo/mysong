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
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "nucleo/onda.hpp"

namespace nu = mysong::nucleo;

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
