// ══════════════════════════════════════════════════════════════════════════
//   A CARNE DO ESPECTRO — src/nucleo/espectro.cpp
// ══════════════════════════════════════════════════════════════════════════
// O TRACTADO vive no cabeçalho. Aqui mora a fftw3, e sómente aqui: é esta a
// unica unidade de traducção da Casa que inclue fftw3.h.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/espectro.hpp"

#include <fftw3.h>

#include <algorithm>
#include <cmath>

namespace mysong::nucleo {
namespace {

constexpr float PI = 3.14159265358979323846f;

// O coefficiente de um filtro de primeira ordem, tirado do TEMPO e do passo.
// Passo maior que o tempo dá coefficiente um, que é chegar de uma vez: é o que
// se quer, e não um coefficiente maior que um a estourar o valor por cima.
float coeficiente(double tempo_ms, double passo_ms) {
  if (tempo_ms <= 0.0 || passo_ms <= 0.0) return 1.0f;
  const double crua = 1.0 - std::exp(-passo_ms / tempo_ms);
  return static_cast<float>(std::min(1.0, std::max(0.0, crua)));
}

// A compressão em decibeis, aparada ao piso. Magnitude não positiva vale zero,
// e a comparação por maior-que trata o NaN de graça: NaN não é maior que zero,
// donde NaN nunca sahe d'esta função, e a barra nunca recebe o que não pinta.
float em_decibeis(float magnitude) {
  if (!(magnitude > 0.0f)) return 0.0f;
  const float db = 20.0f * std::log10(magnitude);
  if (db <= PISO_EM_DECIBEIS) return 0.0f;
  if (db >= 0.0f) return 1.0f;
  return (db - PISO_EM_DECIBEIS) / (0.0f - PISO_EM_DECIBEIS);
}

}  // namespace

Espectro::Espectro(float taxa, int canaes) {
  // A janela de HANN, computada uma vez. Sem janela alguma (que é a janela
  // rectangular) um seno puro espalha a sua raia por toda a tela, e o aceite de
  // que «o seno de 440 acende a banda de 440 e não as outras» morreria por isso
  // e por nada mais.
  hann_.resize(JANELA_DA_FFT);
  for (std::size_t i = 0; i < JANELA_DA_FFT; ++i) {
    const float parte = static_cast<float>(i) / static_cast<float>(JANELA_DA_FFT - 1);
    hann_[i] = 0.5f - 0.5f * std::cos(2.0f * PI * parte);
  }

  entrada_ = fftwf_alloc_real(JANELA_DA_FFT);
  sahida_ = fftwf_alloc_real(2 * (JANELA_DA_FFT / 2 + 1));
  // O plano nasce UMA VEZ, na construcção, e nunca dentro do callback de
  // processo: planejar custa, e custar dentro da linha de tempo real do
  // PipeWire seria pagar em falha de audio o que aqui se paga uma só vez.
  plano_ = fftwf_plan_dft_r2c_1d(static_cast<int>(JANELA_DA_FFT), entrada_,
                                 reinterpret_cast<fftwf_complex*>(sahida_),
                                 FFTW_MEASURE);
  bandas_.assign(QUANTAS_BANDAS, 0.0f);
  assenta_formato(taxa, canaes);
}

Espectro::~Espectro() {
  if (plano_ != nullptr) fftwf_destroy_plan(plano_);
  if (entrada_ != nullptr) fftwf_free(entrada_);
  if (sahida_ != nullptr) fftwf_free(sahida_);
}

void Espectro::assenta_formato(float taxa, int canaes) {
  const float nova = taxa > 0.0f ? taxa : TAXA_PRESUMIDA;
  const int quantos = canaes > 0 ? canaes : 1;
  if (nova == taxa_ && quantos == canaes_ && !bordas_.empty()) return;
  taxa_ = nova;
  canaes_ = quantos;
  // Amostra de taxa velha não se mistura com a nova: o que sobejava sahe fóra.
  sobejo_.clear();
  assenta_bordas();
}

void Espectro::assenta_bordas() {
  // As bordas em RAIAS, de espaçamento logarithmico em HERTZ. A largura da raia
  // sahe da taxa CONFIRMADA: em 44100 a raia vale 21,5 Hz e em 96000 vale 46,9,
  // donde chumbar 48000 aqui poria as bandas no logar errado em qualquer placa
  // que não fosse esta.
  const float largura_da_raia = taxa_ / static_cast<float>(JANELA_DA_FFT);
  const std::size_t ultima_raia = JANELA_DA_FFT / 2;
  const float razao = HERTZ_MAXIMO / HERTZ_MINIMO;
  // O numero de bandas lê-se do proprio estado, e não da constante: é elle a
  // unica verdade depois que o desenho pode pedir outro.
  const std::size_t quantas = bandas_.size();
  bordas_.assign(quantas + 1, 0);
  for (std::size_t b = 0; b <= quantas; ++b) {
    const float parte = static_cast<float>(b) / static_cast<float>(quantas);
    const float hertz = HERTZ_MINIMO * std::pow(razao, parte);
    std::size_t raia = static_cast<std::size_t>(hertz / largura_da_raia + 0.5f);
    // Nenhuma banda fica VAZIA no baixo: em 40 Hz duas bordas seguidas cahiriam
    // na mesma raia, e banda sem raia alguma seria columna morta na tela.
    if (b > 0 && raia <= bordas_[b - 1]) raia = bordas_[b - 1] + 1;
    if (raia > ultima_raia) raia = ultima_raia;
    bordas_[b] = raia;
  }
}

void Espectro::alimenta(const float* amostras, std::size_t quantas) {
  // Bloco vazio, ou ponteiro nullo, não é erro: é rotina. O PipeWire entrega
  // buffer de tamanho zero quando nada tem a dizer, e quem tratasse isso como
  // falha registraria falha o dia inteiro.
  if (amostras == nullptr || quantas == 0) return;

  const std::size_t passo = static_cast<std::size_t>(canaes_);
  for (std::size_t i = 0; i + passo <= quantas; i += passo) {
    // A mistura para mono pela MEDIA dos canaes que o formato disse ter, e não
    // dos dous que se presumiria: fluxo mono existe, e presumir dous leria a
    // amostra do quadro seguinte como se fosse o canal direito.
    float somma = 0.0f;
    for (std::size_t c = 0; c < passo; ++c) somma += amostras[i + c];
    sobejo_.push_back(somma / static_cast<float>(passo));
  }

  // Bloco maior que a janela produz VARIOS quadros, e bloco menor que o salto
  // produz nenhum. Os dous casos são do mundo: o quantum muda em voo.
  while (sobejo_.size() >= JANELA_DA_FFT) {
    um_quadro();
    sobejo_.erase(sobejo_.begin(),
                  sobejo_.begin() + static_cast<std::ptrdiff_t>(SALTO_DA_FFT));
  }
}

void Espectro::um_quadro() {
  // Sem plano não ha quadro, e não ha erro: as bandas ficam onde estavam e
  // esmorecem. A fftw só devolve plano nullo por falta de memoria na
  // construcção, o que é raro; mas executar plano nullo é comportamento
  // indefinido, e comportamento indefinido não é caso que se possa provar nem
  // relatar. Aparar aqui custa uma linha e fecha o caminho.
  if (plano_ == nullptr || entrada_ == nullptr || sahida_ == nullptr) return;
  for (std::size_t i = 0; i < JANELA_DA_FFT; ++i) entrada_[i] = sobejo_[i] * hann_[i];
  fftwf_execute(plano_);

  // A ESCALA: um seno de amplitude um, sob janela de Hann, dá pico de JANELA/4
  // na raia que o contém. A somma da Hann vale JANELA/2, e o espectro de um só
  // lado parte isso ao meio. Dividir por ella põe a escala cheia em 1,0, donde
  // um seno de amplitude 0,5 lê seis decibeis abaixo do cheio, como ha de ser.
  const float escala = static_cast<float>(JANELA_DA_FFT) / 4.0f;
  std::vector<float> alvo(bandas_.size(), 0.0f);
  for (std::size_t b = 0; b < bandas_.size(); ++b) {
    float pico = 0.0f;
    for (std::size_t r = bordas_[b]; r < bordas_[b + 1]; ++r) {
      const float real = sahida_[2 * r];
      const float imaginaria = sahida_[2 * r + 1];
      pico = std::max(pico, std::sqrt(real * real + imaginaria * imaginaria));
    }
    // O PICO da banda, e não a media. As bandas altas têm dezenas de raias e as
    // baixas uma só: a media apagaria o agudo por LARGURA, e não por falta de
    // som, e a tela mostraria uma musica que não é a que toca.
    alvo[b] = em_decibeis(pico / escala);
  }
  suaviza(alvo, 1000.0 * static_cast<double>(SALTO_DA_FFT) / taxa_);
}

void Espectro::suaviza(const std::vector<float>& alvo, double millesimos) {
  const float sobe = coeficiente(TEMPO_DE_ATAQUE_MS, millesimos);
  const float desce = coeficiente(TEMPO_DE_QUEDA_MS, millesimos);
  for (std::size_t b = 0; b < bandas_.size(); ++b) {
    const float agora = bandas_[b];
    const float destino = alvo[b];
    // Dous tempos, e o que decide qual usar é o SENTIDO do movimento: subir é
    // ataque, descer é queda. Um tempo só faria a barra piscar (se rapido) ou
    // arrastar-se atraz da musica (se lento), e a issue nomeia o piscar.
    const float coef = destino > agora ? sobe : desce;
    float novo = agora + (destino - agora) * coef;
    if (novo < LIMIAR_DE_ZERO) novo = 0.0f;
    if (novo > 1.0f) novo = 1.0f;
    bandas_[b] = novo;
  }
}

void Espectro::esmorece(double millesimos) {
  if (millesimos <= 0.0) return;
  // O que sobejava é de ANTES do silencio: guardar aquellas amostras faria a
  // barra ressuscitar com som velho quando o nó voltasse.
  sobejo_.clear();
  const std::vector<float> zero(bandas_.size(), 0.0f);
  suaviza(zero, millesimos);
}

std::vector<float> Espectro::bandas() const { return bandas_; }
float Espectro::taxa() const noexcept { return taxa_; }
int Espectro::canaes() const noexcept { return canaes_; }
const std::vector<std::size_t>& Espectro::bordas() const noexcept { return bordas_; }

std::size_t Espectro::banda_de(float hertz) const {
  // Acha a banda pela RAIA em que a frequencia cahe, e não pelo intervallo em
  // hertz: é a mesma unidade em que o agrupamento trabalha, donde a prova e a
  // obra não podem discordar por arredondamento de meia raia.
  const float largura_da_raia = taxa_ / static_cast<float>(JANELA_DA_FFT);
  const std::size_t raia = static_cast<std::size_t>(hertz / largura_da_raia + 0.5f);
  for (std::size_t b = 0; b < bandas_.size(); ++b) {
    if (raia >= bordas_[b] && raia < bordas_[b + 1]) return b;
  }
  // Fóra da faixa que se pinta. Devolve as bandas que ha, que não é indice
  // valido: quem pergunta por 20 kHz ha de topar com isso, e não com zero, que
  // seria a banda do baixo a responder por um agudo que não existe.
  return bandas_.size();
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
