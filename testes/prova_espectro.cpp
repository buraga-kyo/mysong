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
#include "nucleo/tocador.hpp"

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

TEST_CASE("silencio absoluto dá zero EXACTO, e não erro") {
  nu::Espectro espectro(48000.0f, 2);
  const std::vector<float> mudo(8192, 0.0f);
  espectro.alimenta(mudo.data(), mudo.size());
  for (const float valor : espectro.bandas()) {
    CHECK(valor == 0.0f);
  }
  // E de novo, para que se veja que o silencio não se accumula em cousa alguma.
  espectro.alimenta(mudo.data(), mudo.size());
  for (const float valor : espectro.bandas()) {
    CHECK(valor == 0.0f);
  }
}

TEST_CASE("a queda é mais lenta que o ataque, e desce sem saltar") {
  static_assert(nu::TEMPO_DE_QUEDA_MS > nu::TEMPO_DE_ATAQUE_MS,
                "a queda ha de ser mais lenta que o ataque");
  nu::Espectro espectro(48000.0f, 2);
  const auto forte = seno(440.0f, 1.0f, 48000.0f, 14 * nu::SALTO_DA_FFT);
  espectro.alimenta(forte.data(), forte.size());
  const std::size_t alvo = espectro.banda_de(440.0f);
  const float alto = espectro.bandas()[alvo];
  REQUIRE(alto > 0.5f);

  // Corta-se o som, e a banda ha de DESCER, quadro por quadro, e não saltar a
  // zero: saltar é o que na tela se lê como falha do programa.
  float anterior = alto;
  for (int quadro = 0; quadro < 5; ++quadro) {
    espectro.esmorece(1000.0 * nu::SALTO_DA_FFT / 48000.0);
    const float agora = espectro.bandas()[alvo];
    INFO("quadro=" << quadro << " anterior=" << anterior << " agora=" << agora);
    CHECK(agora < anterior);
    CHECK(agora > 0.0f);
    anterior = agora;
  }

  // E ao fim de um segundo de silencio ella chega a zero, que é o que o aceite
  // pede quando o nó morre.
  espectro.esmorece(1000.0);
  CHECK(espectro.bandas()[alvo] == 0.0f);
}

TEST_CASE("bloco de tamanho absurdo não estoura nem erra") {
  nu::Espectro espectro(48000.0f, 2);
  const std::vector<float> um(1, 0.5f);
  const auto tres_janelas = seno(440.0f, 0.5f, 48000.0f, 3 * nu::JANELA_DA_FFT);

  // Ponteiro nullo, zero amostras, e uma amostra só num fluxo de dous canaes:
  // as tres cousas o PipeWire faz, e nenhuma d'ellas é erro.
  espectro.alimenta(nullptr, 128);
  espectro.alimenta(um.data(), 0);
  espectro.alimenta(um.data(), 1);
  for (const float valor : espectro.bandas()) CHECK(valor == 0.0f);

  // Bloco de tres janelas produz varios quadros de uma vez, que é o caso do
  // quantum grande. O alvo ha de acender do mesmo modo.
  espectro.alimenta(tres_janelas.data(), tres_janelas.size());
  const std::size_t alvo = espectro.banda_de(440.0f);
  CHECK(espectro.bandas()[alvo] > 0.0f);
}

TEST_CASE("fluxo de um canal se mistura sem adiantar a musica") {
  nu::Espectro espectro(48000.0f, 1);
  REQUIRE(espectro.canaes() == 1);
  const auto bloco = seno(440.0f, 0.5f, 48000.0f, 14 * nu::SALTO_DA_FFT, 1);
  espectro.alimenta(bloco.data(), bloco.size());
  const std::size_t alvo = espectro.banda_de(440.0f);
  const auto bandas = espectro.bandas();
  const auto longe = maior_de_longe(bandas, alvo);
  INFO("mono: alvo=" << alvo << " valor=" << bandas[alvo] << " longe=" << longe.first);
  CHECK(bandas[alvo] > 0.5f);
  CHECK(bandas[alvo] - longe.first >= VINTE_DECIBEIS);
}

TEST_CASE("taxa differente põe a mesma frequencia na mesma banda") {
  // A prova de que a taxa não está chumbada: em 44100 a raia vale 21,5 Hz e em
  // 96000 vale 46,9, e ainda assim 440 Hz ha de cahir na banda de 440 Hz.
  for (const float taxa : {44100.0f, 96000.0f}) {
    nu::Espectro espectro(taxa, 2);
    const auto bloco = seno(440.0f, 0.5f, taxa, 14 * nu::SALTO_DA_FFT);
    espectro.alimenta(bloco.data(), bloco.size());
    const std::size_t alvo = espectro.banda_de(440.0f);
    REQUIRE(alvo < nu::QUANTAS_BANDAS);
    const auto bandas = espectro.bandas();
    const auto longe = maior_de_longe(bandas, alvo);
    INFO("taxa=" << taxa << " alvo=" << alvo << " valor=" << bandas[alvo]);
    CHECK(bandas[alvo] > 0.5f);
    CHECK(bandas[alvo] - longe.first >= VINTE_DECIBEIS);
  }
}

namespace {

// O motor SURDO: aceita toda ordem e nada toca. Existe aqui, e não na prova do
// tocador, porque esta lavra não toca em arquivo de lavra alheia.
class MotorSurdo final : public nu::Motor {
 public:
  bool tocar(const std::string&) override { return true; }
  bool pausar() override { return true; }
  bool retomar() override { return true; }
  bool buscar(double) override { return true; }
  bool volume(int) override { return true; }
  double posicao() const override { return 0.0; }
  double duracao() const override { return 0.0; }
  nu::Estado estado() const override { return nu::Estado::Parado; }
  void bombear() override {}
};

// A fonte FINGIDA: entrega o que se lhe puser, e conta as batidas que recebe.
class FonteFingida final : public nu::FonteDeBandas {
 public:
  std::vector<float> bandas() const override { return valores; }
  void pulsa() override { ++batidas; }

  std::vector<float> valores = std::vector<float>(nu::QUANTAS_BANDAS, 0.5f);
  mutable int batidas = 0;
};

}  // namespace

TEST_CASE("o tocador sem fonte entrega zeros, e com fonte entrega o que ella diz") {
  MotorSurdo motor;
  nu::Tocador tocador(motor);

  // Sem fonte: vinte e quatro zeros, e não vector vazio nem excepção. É o
  // contracto que a issue pede enquanto não houver nó.
  const auto sem = tocador.bandas();
  CHECK(sem.size() == nu::QUANTAS_BANDAS);
  for (const float valor : sem) CHECK(valor == 0.0f);

  FonteFingida fonte;
  tocador.observa(fonte);
  const auto com = tocador.bandas();
  REQUIRE(com.size() == nu::QUANTAS_BANDAS);
  CHECK(com[0] == doctest::Approx(0.5f));

  // E a batida do tocador chega á fonte: é o relogio de guarda a ser bobinado.
  CHECK(fonte.batidas == 0);
  tocador.pulsa();
  CHECK(fonte.batidas == 1);
}
