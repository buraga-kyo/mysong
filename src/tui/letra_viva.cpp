// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LETRA VIVA — src/tui/letra_viva.cpp
// ══════════════════════════════════════════════════════════════════════════
// A lavra do que src/tui/letra_viva.hpp declara. O contracto, o dominio e os
// invariantes moram lá, e não se repetem aqui.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/letra_viva.hpp"

#include <cmath>
#include <cstdint>
#include <utility>

namespace mysong::tui {
namespace {

// cingido — o valor no intervallo fechado. Mora aqui, e não em tokens: aquelle
// cinge alfa, e alfa é caso d'este, e não o contrario.
double cingido(double valor, double baixo, double alto) {
  return valor < baixo ? baixo : (valor > alto ? alto : valor);
}

// e_branco — o glypho que o embaralho CONSERVA. É o espaço, e é elle que deixa
// a fórma das palavras a ler-se antes de as letras se resolverem: mexido, a
// linha viraria uma barra de lixo e o olho não veria verso nenhum a chegar.
bool e_branco(const std::string& glifo) { return glifo == " "; }

}  // namespace

std::size_t linha_de_leitura(std::size_t altura) {
  return altura == 0 ? 0 : altura / 3;
}

double nascimento_da_linha(const std::vector<nucleo::LinhaDaLetra>& linhas,
                           std::size_t qual) {
  if (qual >= linhas.size()) return NASCIMENTO_MAXIMO;
  const double anterior = qual == 0 ? 0.0 : linhas[qual - 1].tempo;
  return cingido(linhas[qual].tempo - anterior, NASCIMENTO_MINIMO,
                 NASCIMENTO_MAXIMO);
}

std::vector<std::string> glifos_da_linha(std::string_view texto) {
  std::vector<std::string> saida;
  for (std::size_t i = 0; i < texto.size();) {
    const unsigned char oct = static_cast<unsigned char>(texto[i]);
    // A continuação SOLTA (o octeto 10xxxxxx sem cabeça) vale por um glypho de
    // um octeto: cadeia mal fórmada não ha de fazer o laço andar para traz nem
    // ler fóra do fim, e letra que veio rota mostra-se rota.
    std::size_t quantos = 1;
    if (oct >= 0xf0) quantos = 4;
    else if (oct >= 0xe0) quantos = 3;
    else if (oct >= 0xc0) quantos = 2;
    if (i + quantos > texto.size()) quantos = 1;
    saida.emplace_back(texto.substr(i, quantos));
    i += quantos;
  }
  return saida;
}

namespace {

// mistura_do_acaso — a mistura de bits do splitmix64. Gerador do systema NÃO ha
// n'esta obra, e por isso se escreve: `rand()` daria fita differente a cada
// corrida, e prova alguma se poderia fazer d'ella. Da mesma semente sahe sempre
// o mesmo numero, e a semente é a POSIÇÃO, que é o que a pureza exige.
std::uint64_t mistura_do_acaso(std::uint64_t semente) {
  std::uint64_t x = semente + 0x9e3779b97f4a7c15ull;
  x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
  x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
  return x ^ (x >> 31);
}

}  // namespace

std::string embaralha(const std::vector<std::string>& glifos, double resolvida,
                      std::size_t qual, long long quadro) {
  const std::size_t quantos = glifos.size();
  if (quantos == 0) return {};
  // A FONTE do embaralho são as proprias letras da linha, e sómente ellas: sahe
  // fita que se lê como a mesma lingua, com os mesmos acentos, e não ruido.
  std::vector<std::size_t> fonte;
  for (std::size_t k = 0; k < quantos; ++k)
    if (!e_branco(glifos[k])) fonte.push_back(k);

  // Os RESOLVIDOS são os do MEIO: a janella abre-se do centro para as pontas, e
  // é d'ahi que sahe o «ganhar fórma» que a issue pede. Contada das pontas para
  // o meio, a linha resolver-se-hia pelo fim, que é onde o olho não a lê.
  const std::size_t resolvidos = static_cast<std::size_t>(std::llround(
      cingido(resolvida, 0.0, 1.0) * static_cast<double>(quantos)));
  const std::size_t inicio = (quantos - resolvidos) / 2;

  std::string saida;
  for (std::size_t k = 0; k < quantos; ++k) {
    if ((k >= inicio && k < inicio + resolvidos) || fonte.empty() ||
        e_branco(glifos[k])) {
      saida += glifos[k];
      continue;
    }
    const std::uint64_t semente =
        static_cast<std::uint64_t>(qual) * 1000003ull +
        static_cast<std::uint64_t>(quadro) * 8191ull + k;
    saida += glifos[fonte[mistura_do_acaso(semente) % fonte.size()]];
  }
  return saida;
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
