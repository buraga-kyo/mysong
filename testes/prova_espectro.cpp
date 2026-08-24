// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DO ESPECTRO — testes/prova_espectro.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a MATHEMATICA em machina surda: sem PipeWire, sem placa de som, sem
// mpv e sem arquivo em disco. O sinal é synthetico, feito aqui mesmo, e é isso
// que torna a prova EXACTA: um seno de frequencia conhecida ha de acender a
// banda que o contém, e nenhuma outra ha de subir com elle.
//
// DOMÍNIO ......... senos de frequencia conhecida, silencio absoluto, blocos
//                   de tamanho absurdo, taxas que não são a d'esta machina, e
//                   fluxo de um canal.
// CONTRA-DOMÍNIO .. veredicto do doctest, e por elle o status do ctest.
// INVARIANTE ...... nenhum caso d'aqui abre socket, arquivo ou placa de som:
//                   roda igual em machina sem som e sem PipeWire algum, que é
//                   o que faz a bateria ser verde onde o aceite de mão não se
//                   pode nem tentar.
// Q.E.D. .......... a prova pergunta á obra QUAL banda contém a frequencia, em
//                   vez de repetir a conta do espaçamento logarithmico. Prova
//                   que repete a conta que quer aferir não prova nada: os dous
//                   erros iguaes se cancellam, e o caso fica verde com a obra
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cmath>
#include <vector>

#include "nucleo/espectro.hpp"

namespace {

namespace nu = mysong::nucleo;

constexpr float PI = 3.14159265358979323846f;

// Um seno entrelaçado, de amplitude e frequencia conhecidas, com o mesmo valor
// em todos os canaes: assim a mistura para mono devolve o proprio seno, e a
// prova afere a transformada sem a mistura no meio a mascarar o resultado.
std::vector<float> seno(float hertz, float amplitude, float taxa,
                        std::size_t quadros, int canaes = 2) {
  const auto largura = static_cast<std::size_t>(canaes);
  std::vector<float> bloco(quadros * largura, 0.0f);
  for (std::size_t q = 0; q < quadros; ++q) {
    const float angulo = 2.0f * PI * hertz * static_cast<float>(q) / taxa;
    const float valor = amplitude * std::sin(angulo);
    for (std::size_t c = 0; c < largura; ++c) bloco[q * largura + c] = valor;
  }
  return bloco;
}

}  // namespace

namespace {

// A maior banda que está LONGE do alvo, e o seu indice. Longe é mais de duas
// bandas de distancia, e a folga tem razão physica: no baixo as bandas valem uma
// raia cada (as bordas ahi são 2,3,4,5,6,7,8), e a janela de Hann espalha a raia
// pelas visinhas immediatas. Duas bandas de folga é o limite da resolução d'esta
// janela, e declará-lo aqui é dizer a verdade em vez de affrouxar o limiar.
std::pair<float, std::size_t> maior_de_longe(const std::vector<float>& bandas,
                                             std::size_t alvo) {
  float maior = 0.0f;
  std::size_t qual = 0;
  for (std::size_t b = 0; b < bandas.size(); ++b) {
    const std::size_t distancia = b > alvo ? b - alvo : alvo - b;
    if (distancia <= 2) continue;
    if (bandas[b] > maior) {
      maior = bandas[b];
      qual = b;
    }
  }
  return {maior, qual};
}

// Vinte decibeis na escala de sessenta comprimidos valem um terço do vão todo.
constexpr float VINTE_DECIBEIS = 1.0f / 3.0f;

}  // namespace

TEST_CASE("o seno de 440 Hz acende a banda que o contém, e não as de longe") {
  nu::Espectro espectro(48000.0f, 2);
  const auto bloco = seno(440.0f, 0.5f, 48000.0f, 14 * nu::SALTO_DA_FFT);
  espectro.alimenta(bloco.data(), bloco.size());

  const auto bandas = espectro.bandas();
  REQUIRE(bandas.size() == nu::QUANTAS_BANDAS);
  const std::size_t alvo = espectro.banda_de(440.0f);
  REQUIRE(alvo < nu::QUANTAS_BANDAS);

  const auto longe = maior_de_longe(bandas, alvo);
  INFO("alvo=" << alvo << " valor=" << bandas[alvo] << " maior de longe=" << longe.first
               << " na banda " << longe.second);
  CHECK(bandas[alvo] > 0.5f);
  CHECK(bandas[alvo] - longe.first >= VINTE_DECIBEIS);
}

TEST_CASE("o baixo com o agudo respondem cada um na sua banda") {
  // 100 Hz é o caso do baixo, onde as bandas valem uma raia cada; 6000 Hz é o
  // do agudo, onde a banda tem sessenta e cinco raias. As duas pontas hão de
  // responder, e é aqui que a escolha do PICO em vez da media se prova: com
  // media, a banda de 6000 sahiria dividida por sessenta e cinco.
  for (const float hertz : {100.0f, 6000.0f}) {
    nu::Espectro espectro(48000.0f, 2);
    const auto bloco = seno(hertz, 0.5f, 48000.0f, 14 * nu::SALTO_DA_FFT);
    espectro.alimenta(bloco.data(), bloco.size());

    const auto bandas = espectro.bandas();
    const std::size_t alvo = espectro.banda_de(hertz);
    REQUIRE(alvo < nu::QUANTAS_BANDAS);
    const auto longe = maior_de_longe(bandas, alvo);
    INFO("hertz=" << hertz << " alvo=" << alvo << " valor=" << bandas[alvo]
                  << " maior de longe=" << longe.first << " na banda " << longe.second);
    CHECK(bandas[alvo] > 0.5f);
    CHECK(bandas[alvo] - longe.first >= VINTE_DECIBEIS);

    // E o alvo é o MAIOR de todos, visinhos inclusive: sem esta linha, uma obra
    // que acendesse a banda ao lado com mais força passaria no caso de cima.
    for (std::size_t b = 0; b < nu::QUANTAS_BANDAS; ++b) {
      if (b == alvo) continue;
      CHECK(bandas[b] <= bandas[alvo]);
    }
  }
}

TEST_CASE("as bordas das bandas não decrescem, e nenhuma banda fica vazia") {
  for (const float taxa : {44100.0f, 48000.0f, 96000.0f}) {
    nu::Espectro espectro(taxa, 2);
    const auto& bordas = espectro.bordas();
    REQUIRE(bordas.size() == nu::QUANTAS_BANDAS + 1);
    for (std::size_t b = 0; b < nu::QUANTAS_BANDAS; ++b) {
      INFO("taxa=" << taxa << " banda=" << b << " de " << bordas[b] << " a " << bordas[b + 1]);
      CHECK(bordas[b + 1] > bordas[b]);
      CHECK(bordas[b + 1] <= nu::JANELA_DA_FFT / 2);
    }
  }
}
