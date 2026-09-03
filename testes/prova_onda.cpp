// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA ONDA — testes/prova_onda.cpp
// ══════════════════════════════════════════════════════════════════════════
// A conta da envolvente, a chave e o formato do cache, a dobra e a linha em
// écran de PAPEL. O ffmpeg não corre aqui: afere-se o argv que se HA DE
// correr. O caso VIVO dorme sem a variavel MYSONG_PROVA_MP3.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cmath>
#include <cstdint>
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
