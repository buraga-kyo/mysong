// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SALA — src/tui/sala.cpp
// ══════════════════════════════════════════════════════════════════════════
// A lavra das peças que o sala.hpp declara. Compõe, e sahe.
//
// DOMÍNIO ......... os retractos e a geometria.
// CONTRA-DOMÍNIO .. cadeias e elementos do FTXUI.
// INVARIANTE ...... funcção alguma d'aqui lê o mundo: nem banco, nem relogio.
// Q.E.D. .......... a bateria arma os retractos á mão e afere-a em papel.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/sala.hpp"

namespace mysong::tui {
namespace {
// substantivo_da — o que se conta, em caixa alta. Singular SEM o `s`.
const char* substantivo_da(Especie especie, bool um) {
  switch (especie) {
    case Especie::Artistas: return um ? "ARTISTA" : "ARTISTAS";
    case Especie::Albuns: return um ? "ÁLBUM" : "ÁLBUNS";
    case Especie::Listas: return um ? "LISTA" : "LISTAS";
    case Especie::Achados: return um ? "ACHADO" : "ACHADOS";
    case Especie::Faixas: break;
  }
  return um ? "FAIXA" : "FAIXAS";
}
}  // namespace

std::string texto_da_duracao(int segundos) {
  if (segundos <= 0) return {};
  if (segundos < 60) return std::to_string(segundos) + "s";
  const int minutos = segundos / 60;
  if (minutos < 60) return std::to_string(minutos) + "min";
  // Zero á esquerda nos minutos: «1h5» lê-se hora e cinco HORAS.
  const int resto = minutos % 60;
  return std::to_string(minutos / 60) + "h" + (resto < 10 ? "0" : "") +
         std::to_string(resto);
}

std::string texto_da_conta(std::size_t quantas, Especie especie, int duracao) {
  std::string feita =
      std::to_string(quantas) + " " + substantivo_da(especie, quantas == 1);
  const std::string tempo = texto_da_duracao(duracao);
  if (!tempo.empty()) feita += ", " + tempo;
  return feita;
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
