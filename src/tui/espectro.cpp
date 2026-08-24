// ══════════════════════════════════════════════════════════════════════════
//   O CORPO DO DESENHO DO ESPECTRO — src/tui/espectro.cpp
// ══════════════════════════════════════════════════════════════════════════
// A conta que o tractado promette. Nada aqui abre terminal, lê ambiente ou
// consulta relogio: d'onde toda affirmação d'este manuscripto se prova em
// machina surda, e a fita deixa de depender do olho de quem a abriu.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/espectro.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace mysong::tui {

namespace {

// cingido — a magnitude reduzida ao intervallo [0,1] que o contracto promette,
// e que esta Casa não confia. A ORDEM das duas guardas é o que importa: a
// finitude PRIMEIRO, porque toda comparação com NaN é falsa, e um cingir
// escripto na ordem natural (`m < 0 ? 0 : m > 1 ? 1 : m`) devolveria o NaN
// intacto ao floor, d'onde sahiria conta indefinida e indice fóra de limite.
float cingido(float magnitude) {
  if (!std::isfinite(magnitude)) return 0.0f;
  if (magnitude < 0.0f) return 0.0f;
  if (magnitude > 1.0f) return 1.0f;
  return magnitude;
}

}  // namespace

int oitavos(float magnitude, std::size_t altura) {
  const int teto = static_cast<int>(altura) * DEGRAUS_POR_CELULA;
  if (teto <= 0) return 0;
  // O floor, e não o arredondamento: enche-se o degrau que a magnitude JÁ
  // conquistou, e não o que ella quasi conquistou. D'onde a magnitude cheia dá o
  // teto EXACTO (floor de 1 * teto é teto) e o silencio dá zero exacto, que são
  // os dous extremos que o aceite cobra por nome.
  const float degraus = cingido(magnitude) * static_cast<float>(teto);
  const int conquistados = static_cast<int>(std::floor(degraus));
  return conquistados < 0 ? 0 : (conquistados > teto ? teto : conquistados);
}

std::string glifo_do_degrau(int degrau) {
  if (degrau <= 0) return std::string(kCelulaVazia);
  const int k = degrau > DEGRAUS_POR_CELULA ? DEGRAUS_POR_CELULA : degrau;
  // U+2580 + k, em UTF-8 de tres octetos. Escreve-se por ARITHMETICA do ponto
  // de codigo, e não por taboada de oito glifos crus, porque a taboada
  // permittiria um glifo fóra de ordem passar calado; a arithmetica não. O
  // terceiro octeto de U+2580 é 0x80, d'onde o de U+2580 + k é 0x80 + k, e k
  // vae de 1 a 8, que é U+2581 (um oitavo) a U+2588 (o bloco cheio).
  return std::string{'\xe2', '\x96', static_cast<char>('\x80' + k)};
}

const Celula& Quadro::em(std::size_t linha, std::size_t collunha) const {
  // A célulla de fóra, uma só e immutavel: devolve-se referencia a ella em vez
  // de estourar, conforme o cabeçalho promette.
  static const Celula de_fora;
  if (linha >= altura || collunha >= largura) return de_fora;
  return celulas[linha * largura + collunha];
}

namespace {

// valor_da_columna — A REPARTIÇÃO, e é UMA funcção para os DOUS regimes.
//
// A collunha `c` de `largura` cobre o intervallo SEMI-ABERTO de bandas
// [c * n / largura, (c + 1) * n / largura), e toma o MÁXIMO d'ellas.
//
// Sendo a largura MENOR que n, o intervallo tem duas bandas ou mais, e o máximo
// FUNDE. Funde e não amostra, de propósito: amostrar faria um pico desapparecer
// só porque o operador estreitou a janella, e barra que apaga ao redimensionar
// lê-se como defeito. Sendo a largura MAIOR que n, o intervallo teria comprimento
// menor que um e sahiria VAZIO por truncamento; alarga-se ao minimo de uma banda,
// e então o máximo degenera em copia, que é o esticar.
//
// D'aqui sahe de graça o invariante que o aceite cobra: os intervallos partem
// [0, n) sem sobra e sem vão, d'onde banda alguma se perde em largura alguma.
float valor_da_columna(const std::vector<float>& bandas, std::size_t c,
                       std::size_t largura) {
  const std::size_t n = bandas.size();
  if (n == 0 || largura == 0) return 0.0f;

  std::size_t principio = (c * n) / largura;
  if (principio >= n) principio = n - 1;
  std::size_t fim = ((c + 1) * n) / largura;
  if (fim <= principio) fim = principio + 1;  // o intervallo nunca é vazio
  if (fim > n) fim = n;

  float pico = 0.0f;
  for (std::size_t b = principio; b < fim; ++b)
    pico = std::max(pico, cingido(bandas[b]));
  return pico;
}

}  // namespace

tokens::Triade tinta_da_linha(std::size_t desde_a_base, std::size_t altura) {
  // Painel de uma célulla só: a rampa degenera, e vale a BASE. A §7.4.9 ancora
  // a rampa na base («v700 na base»), e painel de uma célulla é todo base; o
  // meio da rampa seria côr que a spec não nomeia em logar algum. E o desvio
  // por zero fica excluido antes de se chegar á divisão.
  if (altura <= 1) return tokens::rgb(tokens::v700);

  const std::size_t alto = desde_a_base >= altura ? altura - 1 : desde_a_base;
  const double t = static_cast<double>(alto) / static_cast<double>(altura - 1);

  // A interpolação vae por tokens::mistura, e NÃO por arithmetica de côr nova.
  // Ella compõe a frente sobre o fundo com o peso dado, que é exactamente a
  // interpolação linear que se quer, e a bateria da issue #2 já a prova. D'onde
  // t = 0 dá v700 EXACTO e t = 1 dá v400 EXACTO, sem arredondamento a explicar.
  // Escrever aqui uma segunda conta de côr seria abrir um segundo caminho para o
  // mesmo resultado, e dous caminhos divergem sem avisar.
  return tokens::mistura(tokens::v400, tokens::v700, t);
}
