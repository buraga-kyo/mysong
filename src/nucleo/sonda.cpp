// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SONDA — src/nucleo/sonda.cpp
// ══════════════════════════════════════════════════════════════════════════
// A obra da sonda, em duas partes que não se misturam: a TABOA dos requisitos
// com a laçada que a percorre, que nada sabem do systema; e as tres consultas
// que interrogam o systema de verdade, que nada sabem da taboa.
//
// DOMÍNIO ......... a taboa d'este arquivo, e o inquerito que o chamador traz.
// CONTRA-DOMÍNIO .. o relatorio, com um estado por requisito da taboa.
// INVARIANTE ...... a taboa tem duração estática e chave UNICA por requisito;
//                   duas invocações de requisitos() devolvem a mesma referencia.
// Q.E.D. .......... requisito novo é LINHA nova na taboa, e nunca ramo novo na
//                   laçada; donde a sonda cresce sem que a laçada cresça.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/sonda.hpp"

namespace mysong::nucleo {

// A TABOA. Os IMPEDIMENTOS primeiro, que é a ordem em que a tela os mostra: a
// fonte, sem a qual a estetica da casa vira quadrículo, e a libmpv, que é a
// machina de som e sem a qual não ha tocador algum, sómente moldura.
// O remedio cabe em UMA linha, e o mais remette-se ao README.
const std::vector<Requisito>& requisitos() {
  static const std::vector<Requisito> taboa = {
      {"fonte", "fonte com glifos de seta (Nerd Font)", Gravidade::Impedimento,
       Especie::FamiliaDeFonte, "nerd",
       "baixe uma Nerd Font de nerdfonts.com para ~/.local/share/fonts e rode "
       "fc-cache -fv"},
      {"libmpv", "libmpv (a machina de som)", Gravidade::Impedimento,
       Especie::Bibliotheca, "libmpv.so.2",
       "installe a libmpv: sudo apt install libmpv2 (ou libmpv-dev, para "
       "compilar)"},
  };
  return taboa;
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
