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
namespace {

// A raiz do ws/2. N'uma constante para que as tres consultas digam UM endereço.
constexpr char kRaiz[] = "https://musicbrainz.org/ws/2/";

}  // namespace

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

std::string url_da_consulta_pelo_link(std::string_view id_do_track) {
  // A relação de URL: o MusicBrainz guarda o endereço do track e a gravação a
  // que elle aponta. MEDIDO em 2026-08-27: 200 com a relação quando o link está
  // lá, e 404 quando não está; o 404 é resposta, e manda quem chama á busca.
  if (id_do_track.empty()) return {};
  const std::string track =
      "https://open.spotify.com/track/" + std::string(id_do_track);
  return std::string(kRaiz) + "url?resource=" + escapa_url(track) +
         "&inc=recording-rels&fmt=json";
}

std::string url_da_ficha(std::string_view mbid) {
  // A gravação inteira: ISRCs, artistas, releases com grupo e com a numeração
  // das faixas. O mbid vem do proprio MB, e ainda assim não se interpola cru.
  if (mbid.empty()) return {};
  return std::string(kRaiz) + "recording/" + escapa_url(mbid) +
         "?inc=isrcs+artist-credits+releases+release-groups+media&fmt=json";
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
