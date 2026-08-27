// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MUSICBRAINZ — src/nucleo/musicbrainz.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro (consultas e leituras), e a rede sozinha
// no fim, á maneira do catalogo: o que se prova está acima, o que não se prova
// está abaixo, e o olho vê a fronteira de um relance.
//
// DOMÍNIO ......... o id de um track, ou artista+titulo+duração; e os corpos.
// CONTRA-DOMÍNIO .. as URLs de consulta, e a ficha da gravação.
// INVARIANTE ...... o acelerador é UM para o processo inteiro: dous obreiros da
//                   fila de baixa somam UMA requisição por segundo, e não duas.
// Q.E.D. .......... as fixtures da bateria são recortes VERBATIM das respostas
//                   vivas de 2026-08-27; leitor que as lê, lê a API de verdade.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/musicbrainz.hpp"

#include <string_view>

namespace mysong::nucleo {

std::string escapa_url(std::string_view crua) {
  // O percent-encoding do RFC 3986, byte a byte. PROPRIO, e não o do curl: o
  // curl_easy_escape pede punho vivo, e punho em funcção pura é rede dentro do
  // que a bateria havia de provar sem rede.
  static constexpr char kHexa[] = "0123456789ABCDEF";
  std::string obra;
  obra.reserve(crua.size() * 3);
  for (const char bruto : crua) {
    const unsigned char byte = static_cast<unsigned char>(bruto);
    const bool livre = (byte >= 'A' && byte <= 'Z') ||
                       (byte >= 'a' && byte <= 'z') ||
                       (byte >= '0' && byte <= '9') || byte == '-' ||
                       byte == '.' || byte == '_' || byte == '~';
    if (livre) { obra += bruto; continue; }
    obra += '%';
    obra += kHexa[byte >> 4];
    obra += kHexa[byte & 0x0F];
  }
  return obra;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
