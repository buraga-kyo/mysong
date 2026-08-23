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
//                   cairo, e repouso no meio abriria a emenda visivel. E a
//                   eleição do glifo mora n'UM só logar, dentro de compor():
//                   espalhada por dous ramos, a unicidade que o cabeçalho
//                   promette deixaria de ter guarda que a prova possa matar.
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

Fita::Fita(Sentido sentido, bool cauda) : sentido_(sentido), cauda_(cauda) {}

Fita& Fita::junta(Segmento segmento) {
  segmentos_.push_back(std::move(segmento));
  return *this;
}

// compor — Regra (b): a junção herda a tinta do segmento que ella SEGUE e toma
// por fundo o que a segue; na fita que aponta á ESQUERDA vale o espelho. Um
// sentido por fita: os dous lavrariam o losango que a regra (a) proscreve.
std::vector<Pedaco> Fita::compor() const {
  std::vector<Pedaco> fita;
  if (segmentos_.empty()) return fita;  // fita vazia não tem sequer cauda.
  const std::size_t quantos = segmentos_.size();
  fita.reserve(quantos * 2 + 1);
  // A ELEIÇÃO, e ella sózinha: um glifo por fita, tirado do sentido com que a
  // fita nasceu. Não ha campo que o guarde, nem porta que o troque; d'onde o
  // losango de duas pontas não tem por onde se exprimir.
  const std::string_view eleito =
      sentido_ == Sentido::Dextra ? kPontaDextra : kPontaEsquerda;
  const auto rotulo = [&](std::size_t i) {
    fita.push_back({segmentos_[i].rotulo, segmentos_[i].fundo,
                    segmentos_[i].tinta, false, false});
  };
  const auto juncao = [&](std::string_view cama, std::string_view herdada,
                          bool remate) {
    fita.push_back({std::string(eleito), cama, herdada, true, remate});
  };
  if (sentido_ == Sentido::Esquerda) {
    if (cauda_) juncao(tokens::transparent, segmentos_.front().fundo, true);
    for (std::size_t i = 0; i < quantos; ++i) {
      if (i > 0) juncao(segmentos_[i - 1].fundo, segmentos_[i].fundo, false);
      rotulo(i);
    }
    return fita;
  }
  for (std::size_t i = 0; i < quantos; ++i) {
    rotulo(i);
    if (i + 1 < quantos) juncao(segmentos_[i + 1].fundo, segmentos_[i].fundo, false);
  }
  if (cauda_) juncao(tokens::transparent, segmentos_.back().fundo, true);
  return fita;
}

// Conta-se em CODEPOINTS: para o CJK e o emoji, de duas collunas, SUBESTIMA.
std::size_t Fita::largura_exigida() const {
  std::size_t collunas = 0;
  for (const Pedaco& pedaco : compor())
    for (const unsigned char byte : pedaco.texto)
      if ((byte & 0xC0) != 0x80) ++collunas;  // conta-se so o byte lider
  return collunas;
}

}  // namespace mysong::tui

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
