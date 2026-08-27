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

#include "api/jsonzinho.hpp"

namespace mysong::nucleo {
namespace {

// A raiz do ws/2. N'uma constante para que as tres consultas digam UM endereço.
constexpr char kRaiz[] = "https://musicbrainz.org/ws/2/";

// A janella da busca por duração, em milesimos: os MESMOS doze segundos da
// TOLERANCIA_DO_CASAMENTO, e pela mesma medida (silencio nas pontas fica dentro,
// versão ao vivo e estendida ficam fóra).
constexpr int kJanellaMs = 12000;

// aspas_seguras — o texto dentro de aspas da consulta Lucene. Aspa e
// contra-barra escapam-se; sem isto, um titulo com aspa partiria a frase e o
// resto do titulo viraria operador de busca.
std::string aspas_seguras(std::string_view crua) {
  std::string obra;
  obra.reserve(crua.size() + 4);
  for (const char letra : crua) {
    if (letra == '"' || letra == '\\') obra += '\\';
    obra += letra;
  }
  return obra;
}

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

std::string url_da_consulta_pela_busca(std::string_view artista,
                                       std::string_view titulo, int duracao_ms) {
  // O caminho segundo: gravação por artista, titulo e duração. Duração zero
  // («não disse») busca sem janella; e a de baixo apara-se no zero, que
  // janella negativa o Lucene recusa inteira.
  if (titulo.empty()) return {};
  std::string consulta = "recording:\"" + aspas_seguras(titulo) + "\"";
  if (!artista.empty())
    consulta += " AND artist:\"" + aspas_seguras(artista) + "\"";
  if (duracao_ms > 0) {
    const int piso = duracao_ms > kJanellaMs ? duracao_ms - kJanellaMs : 0;
    consulta += " AND dur:[" + std::to_string(piso) + " TO " +
                std::to_string(duracao_ms + kJanellaMs) + "]";
  }
  return std::string(kRaiz) + "recording?query=" + escapa_url(consulta) +
         "&limit=8&fmt=json";
}

std::string url_da_ficha(std::string_view mbid) {
  // A gravação inteira: ISRCs, artistas, releases com grupo e com a numeração
  // das faixas. O mbid vem do proprio MB, e ainda assim não se interpola cru.
  if (mbid.empty()) return {};
  return std::string(kRaiz) + "recording/" + escapa_url(mbid) +
         "?inc=isrcs+artist-credits+releases+release-groups+media&fmt=json";
}

std::string le_gravacao_da_url(std::string_view corpo) {
  // O corpo do url-lookup: `relations[]`, e em cada relação a gravação a que o
  // link aponta. Toma-se a PRIMEIRA cujo alvo é gravação; havendo mais de uma,
  // qualquer d'ellas é casamento que o proprio MB declarou. Corpo de 404,
  // truncado ou alheio dá vazio, que é resposta e não erro.
  const std::string arranjo = api::recorta_arranjo(corpo, "relations");
  if (arranjo.empty()) return {};
  for (const std::string& relacao : api::objectos_do_arranjo(arranjo)) {
    if (api::texto_de_chave(relacao, "target-type") != "recording") continue;
    const std::string gravacao = api::recorta_objecto(relacao, "recording");
    if (gravacao.empty()) continue;
    const std::string mbid = api::texto_de_chave(gravacao, "id");
    if (!mbid.empty()) return mbid;
  }
  return {};
}

FichaMB le_ficha_da_gravacao(std::string_view corpo) {
  // O corpo do lookup da gravação. Titulo e duração são os de fundo UM: os das
  // releases moram mais fundo, e os leitores da Casa não os confundem. O
  // credito de artista toma-se do primeiro arranjo `artist-credit` do corpo,
  // que numa gravação é o d'ella; e a ficha é «melhor esforço»: campo que o
  // corpo não traga fica vazio, e quem chama decide o que fazer com o vazio.
  FichaMB ficha;
  ficha.titulo = api::texto_de_chave(corpo, "title");
  double valor = 0.0;
  if (api::numero_de_chave(corpo, "length", &valor))
    ficha.duracao_ms = static_cast<int>(valor);
  ficha.isrcs = api::textos_do_arranjo(api::recorta_arranjo(corpo, "isrcs"));
  const std::vector<std::string> creditos =
      api::objectos_do_arranjo(api::recorta_arranjo(corpo, "artist-credit"));
  if (!creditos.empty()) ficha.artista = api::texto_de_chave(creditos[0], "name");
  return ficha;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
