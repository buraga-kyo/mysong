// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA CAPA — src/nucleo/capa.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro; o chafa e a taglib no fim.
//
// DOMÍNIO ......... o caminho de uma faixa, e a geometria do painel.
// CONTRA-DOMÍNIO .. linhas de texto prontas a pintar, ou a ausencia declarada.
// INVARIANTE ...... funcção alguma d'aqui lança, e capa ausente não é falha.
// Q.E.D. .......... a chave do cache não conhece o chafa, donde a prova do cache
//                   corre sem imagem alguma no disco.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/capa.hpp"

#include <algorithm>
#include <cctype>

namespace mysong::nucleo {

const std::vector<std::string>& nomes_de_capa() {
  // A ordem é a convenção que os ripadores usam, e não gosto: `cover` é o nome que
  // o Picard grava, `folder` o que o Windows Media Player grava, e `front` o que os
  // acervos antigos trazem. Procura-se por essa ordem para que o acervo que tenha
  // dous mostre o que o ripador teve por principal.
  static const std::vector<std::string> kNomes = {
      "cover.jpg",  "cover.jpeg", "cover.png",  "cover.webp",
      "folder.jpg", "folder.png", "front.jpg",  "front.png",
      "album.jpg",  "album.png",  "capa.jpg",   "capa.png",
  };
  return kNomes;
}

std::filesystem::path capa_ao_lado(const std::filesystem::path& faixa) {
  const std::filesystem::path pasta = faixa.parent_path();
  std::error_code erro;
  for (const std::string& nome : nomes_de_capa()) {
    const std::filesystem::path tentativa = pasta / nome;
    if (std::filesystem::is_regular_file(tentativa, erro) && !erro)
      return tentativa;
  }
  return {};
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
