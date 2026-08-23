// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LAVRA DA FITA — src/tui/arrowline.cpp
// ══════════════════════════════════════════════════════════════════════════
// Lavra o que arrowline.hpp promette. A regra vae escripta lá; aqui é a obra.
// DOMÍNIO ......... os segmentos acumulados por junta(), na ordem em que se
//                   juntaram, que é a ordem em que se lêem.
// CONTRA-DOMÍNIO .. os pedaços, o degrau rebaixado, a largura exigida.
// INVARIANTE ...... o par tinta/fundo do glifo de junção sahe IMMEDIATAMENTE
//                   antes d'elle, sem repouso pelo meio: é o analogo terminal
//                   da ordem de pintura invertida que o metrics.lua impõe ao
//                   cairo, e repouso no meio abriria a emenda visivel.
// Q.E.D. .......... a largura conta-se sobre os proprios pedaços compostos, e
//                   não por conta apartada que envelheceria em silencio.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/arrowline.hpp"

#include <cstddef>
#include <string>
#include <utility>

namespace mysong::tui {

// Rebaixar é andar DOUS assentos da rampa, e não um: os degraus intermedios o
// olho não distingue da vizinhança, e a regra (c) fala dos de centena. No
// fundo da rampa satura, que sahir d'ella seria peor que não descer.
std::string_view rebaixar(std::string_view degrau) {
  static constexpr std::string_view rampa[] = {
      tokens::v50,  tokens::v100, tokens::v200, tokens::v300,
      tokens::v400, tokens::v500, tokens::v600, tokens::v700,
      tokens::v800, tokens::v900, tokens::v950, tokens::v975};
  constexpr std::size_t ultimo = (sizeof(rampa) / sizeof(rampa[0])) - 1;
  for (std::size_t i = 0; i <= ultimo; ++i) {
    if (rampa[i] != degrau) continue;
    return rampa[i + 2 > ultimo ? ultimo : i + 2];
  }
  return degrau;  // côr de fóra da rampa não se rebaixa: devolve-se intacta.
}

Fita::Fita(Sentido sentido, bool cauda)
    : glifo_(sentido == Sentido::Dextra ? std::string(kPontaDextra)
                                       : std::string(kPontaEsquerda)),
      sentido_(sentido),
      cauda_(cauda) {}

Fita& Fita::junta(Segmento segmento) {
  segmentos_.push_back(std::move(segmento));
  return *this;
}

}  // namespace mysong::tui
