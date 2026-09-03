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

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
