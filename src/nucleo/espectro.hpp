// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ESPECTRO — src/nucleo/espectro.hpp
// ══════════════════════════════════════════════════════════════════════════
// A MATHEMATICA, e nada mais: recebe amostras, devolve bandas. Não sabe de
// PipeWire, não sabe de mpv, não abre linha de execução propria. É de
// proposito, e é a decisão que faz esta tarefa provavel: a colheita fala com o
// mundo e não se prova, porque não ha PipeWire determinístico; a transformação é
// CONTA, e conta se prova com sinal synthetico em machina surda.
//
// DOMÍNIO ......... amostras flotantes ENTRELAÇADAS, na taxa e no numero de
//                   canaes que o formato confirmado disser, em blocos de
//                   tamanho QUALQUER, inclusive zero e maiores que a janela: o
//                   quantum d'esta machina vae de 32 a 2048, e muda em voo.
// CONTRA-DOMÍNIO .. QUANTAS_BANDAS magnitudes em [0,1], suavizadas, promptas
//                   para barra de terminal.
// INVARIANTE ...... silencio absoluto dá zero EXACTO, e não erro. As bandas
//                   nunca sahem de [0,1], nunca sahem NaN e nunca sahem em
//                   numero differente de QUANTAS_BANDAS. As bordas se calculam
//                   da taxa CONFIRMADA, jamais de 48000 chumbado: placa de
//                   44100 muda a largura da raia, e as bordas com ella.
// Q.E.D. .......... um seno de frequencia conhecida acende a banda que o
//                   contém, e não as que estão longe d'ella. É prova exacta, e
//                   não depende de machina nem de placa de som. E a fftw3 não
//                   entra por este cabeçalho, que declara o plano adiante, do
//                   mesmo modo que o motor declara o punho da libmpv.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <vector>

#include "nucleo/analisador.hpp"  // de onde vem QUANTAS_BANDAS, que é do CONTRACTO

// O plano da fftw3, declarado adiante e não incluido: assim fftw3.h não entra
// por este cabeçalho em unidade alguma que não precise d'ella, e a bateria da
// mathematica compila sem os directorios de inclusão da fftw.
extern "C" {
struct fftwf_plan_s;
}

namespace mysong::nucleo {

// ── A PARAMETRIZAÇÃO DA TRANSFORMADA, com o porquê, que é o que a distingue de
// numero solto. A taxa d'esta machina é 48000 (pw-metadata: clock.rate=48000).
//
// JANELA de 2048 amostras = 42,7 ms, e raia de 48000/2048 = 23,44 Hz. A raia ha
// de ser mais FINA que o intervallo que o ouvido separa: o semitom em torno de
// 440 Hz vale cerca de 26 Hz, e 23,44 cabe dentro d'elle. Com 1024 a raia daria
// 46,9 Hz e o baixo virava um borrão; com 4096 a latencia subiria a 85 ms, que o
// olho já lê como atraso entre o som e a barra.
inline constexpr std::size_t JANELA_DA_FFT = 2048;

// SALTO de 1024 = metade da janela, e não por gosto: 1024 é EXACTAMENTE o
// quantum d'esta machina (pw-metadata: clock.quantum=1024), donde cada buffer
// que o PipeWire entrega produz um quadro, e o que sobeja não se accumula de
// quadro em quadro. Dá 46,9 quadros por segundo, folgado acima da redesenha de
// um terminal.
inline constexpr std::size_t SALTO_DA_FFT = 1024;
static_assert(SALTO_DA_FFT > 0 && SALTO_DA_FFT <= JANELA_DA_FFT,
              "o salto ha de caber na janela");

// ── A FAIXA que se pinta. Abaixo de 40 Hz quasi nenhum fone reproduz, e acima
// de 16 kHz quasi nenhum ouvido de adulto escuta. Entre as duas o espaçamento é
// LOGARITHMICO, porque é assim que o ouvido escuta: de 40 a 80 Hz vae uma
// octava, de 8000 a 16000 vae outra, e as duas hão de occupar a mesma largura na
// tela. Espaçamento linear daria vinte bandas de agudo e nenhuma de baixo.
inline constexpr float HERTZ_MINIMO = 40.0f;
inline constexpr float HERTZ_MAXIMO = 16000.0f;

// O PISO. Magnitude crua é linear, e em linear tudo o que não é o pico fica
// rente ao chão: a barra parece morta com musica a tocar. Comprime-se em
// decibeis, com o piso a valer zero e a escala cheia a valer um. Sessenta
// decibeis é a faixa que um alto-falante de mesa entrega e o olho distingue.
inline constexpr float PISO_EM_DECIBEIS = -60.0f;

// Abaixo d'este limiar a banda vale ZERO EXACTO, e não um resto de arredondar.
// É o que faz o silencio ser silencio de verdade na tela, e não 0,003 a tremer;
// e é o que faz o aceite poder pedir «bandas em zero» e receber zero.
//
// Tres centesimos, e os dous motivos são medidos. O primeiro é do olho: uma
// barra de terminal desenha-se com oito glyphos de bloco, donde o menor passo
// que ella mostra vale um oitavo, e tres centesimos são um quarto d'esse menor
// passo. O segundo é do prazo: com queda de 250 ms, uma banda cheia leva 876 ms
// para descer a tres centesimos, e cabe no segundo que o aceite dá ao nó que
// morre; com um centesimo levaria 1151 ms, e o aceite estouraria por 151.
inline constexpr float LIMIAR_DE_ZERO = 0.03f;

// A taxa que se presume ANTES de o formato ser confirmado. Presumir é legitimo,
// chumbar não é: assenta_formato() recalcula tudo quando o mundo responde.
inline constexpr float TAXA_PRESUMIDA = 48000.0f;

// ── A SUAVIZAÇÃO, em TEMPO e não em coefficiente. Coefficiente é numero solto:
// muda-se o salto e a suavização desafina sem que ninguem note. Tempo é cousa
// physica, e o coefficiente sahe d'elle com a duração REAL do salto.
//
// ATAQUE de 40 ms: a batida ha de chegar á tela quasi junto com o som, e 40 ms é
// o limite em que o olho ainda casa o movimento com o que ouve.
// QUEDA de 250 ms: o olho lê queda mais devagar do que ataque, e barra que desce
// tão rapido quanto sobe PISCA. Um quarto de segundo desce com graça e ainda
// acompanha a musica.
inline constexpr double TEMPO_DE_ATAQUE_MS = 40.0;
inline constexpr double TEMPO_DE_QUEDA_MS = 250.0;
static_assert(TEMPO_DE_QUEDA_MS > TEMPO_DE_ATAQUE_MS,
              "a queda ha de ser mais lenta que o ataque, ou a barra pisca");

// Passado este prazo sem buffer novo, declara-se o silencio. O nó do mpv morre
// SEM AVISO: o callback de processo simplesmente deixa de correr, e sem este
// prazo a ultima janela ficaria congelada na tela para sempre. Cento e vinte
// millesimos são cinco quantos de folga, que basta para não confundir um
// tropeço do systema com faixa que acabou.
inline constexpr double PRAZO_DE_SILENCIO_MS = 120.0;

// O ESPECTRO. Uma linha de execução só: quem o alimenta e quem o lê é o mesmo,
// ou então quem os coordena põe fechadura de fóra. A conta não conhece linha de
// execução, e é justamente por isso que ella se prova.
class Espectro {
 public:
  explicit Espectro(float taxa = TAXA_PRESUMIDA, int canaes = 2,
                    std::size_t quantas = QUANTAS_BANDAS);
  ~Espectro();

  Espectro(const Espectro&) = delete;
  Espectro& operator=(const Espectro&) = delete;

  // Assenta o formato CONFIRMADO pelo mundo. Recalcula as bordas e esquece o
  // que sobejava, porque amostra de taxa velha não se mistura com a nova.
  void assenta_formato(float taxa, int canaes);

  // Engole amostras ENTRELAÇADAS: quantas é o numero de FLOTANTES, não de
  // quadros. Bloco de tamanho qualquer, inclusive zero e inclusive maior que a
  // janela; o que não completa quadro fica de sobejo para o bloco seguinte.
  void alimenta(const float* amostras, std::size_t quantas);

  // O PEDIDO de quem desenha: recalcula as bordas e redimensiona o estado. O
  // numero cinge-se por cinge_bandas, e o estado ZERA quando elle MUDA:
  // interpolar bandas velhas em bordas novas mentiria por um quadro, e um
  // quadro são 22 millesimos. Pedir o MESMO numero nada faz, e é o que permitte
  // ao desenho pedir a cada quadro sem zerar quarenta e seis vezes por segundo.
  void quer_bandas(std::size_t quantas);
  std::size_t quantas_bandas() const noexcept;

  // Esmorece as bandas pelo tempo passado, sem amostra alguma. É o que o relogio
  // de guarda chama quando o nó morre.
  void esmorece(double millesimos);

  // O retracto: sempre QUANTAS_BANDAS valores, sempre em [0,1].
  std::vector<float> bandas() const;

  float taxa() const noexcept;
  int canaes() const noexcept;

  // As bordas em RAIAS, abertas á prova: é assim que ella sabe QUAL banda
  // contém 440 Hz sem repetir a conta que ella mesma quer aferir. Ha
  // QUANTAS_BANDAS + 1 bordas, não decrescentes, e a banda b vae de bordas()[b]
  // inclusive a bordas()[b + 1] exclusive.
  const std::vector<std::size_t>& bordas() const noexcept;
  std::size_t banda_de(float hertz) const;

 private:
  void um_quadro();
  void assenta_bordas();
  void suaviza(const std::vector<float>& alvo, double millesimos);

  float taxa_ = TAXA_PRESUMIDA;
  int canaes_ = 2;

  // A entrada e a sahida da fftw, alocadas por ella para que o alinhamento seja
  // o que ella pede. A sahida é REAL de propósito: são 2 * (JANELA/2 + 1)
  // flotantes, a parte real e a imaginaria alternadas, que é exactamente a
  // memoria do fftwf_complex sem que este cabeçalho o precise conhecer.
  float* entrada_ = nullptr;
  float* sahida_ = nullptr;
  ::fftwf_plan_s* plano_ = nullptr;

  std::vector<float> hann_;    // a janela, computada uma vez
  std::vector<float> sobejo_;  // amostras mono que ainda não formaram quadro
  std::vector<float> bandas_;  // o estado suavizado, que é o que se lê
  std::vector<std::size_t> bordas_;
};

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
