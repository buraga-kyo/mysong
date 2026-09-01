// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO COVER ART ARCHIVE — src/nucleo/caa.hpp
// ══════════════════════════════════════════════════════════════════════════
// A capa que falta ao acervo que JÁ está no disco (issue #83). A issue #81
// consertou a baixa nova: o yt-dlp embute a miniatura. Esta peça cuida do
// resto: por ORDEM do operador, casa a faixa sem capa no MusicBrainz pelos
// metadados que ella tem (faixa antiga não traz ISRC), pede a arte ao Cover
// Art Archive, que é o irmão do MB indexado pela RELEASE, e embute-a na
// etiqueta pelo mesmo quadro APIC da #81, que sobrevive a mover o arquivo.
//
// DOMÍNIO ......... faixas do índice, e os corpos e octetos que a rede dá.
// CONTRA-DOMÍNIO .. arte embutida, e desfechos NOMEADOS, faixa a faixa.
// INVARIANTE ...... o acervo não se toca sem ordem; negativo DEFINITIVO
//                   lembra-se fóra do índice (que a varredura reconstroe);
//                   o transitorio não se lembra; e o recuo pára a caça toda.
// Q.E.D. .......... a rede entra por parametro (a Consulta da casa), donde a
//                   bateria afere a caça inteira com corpos de mentira.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <string>
#include <string_view>

#include "nucleo/musicbrainz.hpp"

namespace mysong::nucleo {

// url_da_capa — a capa da FRENTE da release, na miniatura de quinhentos
// pixels (RULINGS R4 da issue). O front-500, e não o /front original: o
// original pode ser PNG de varios mega-octetos, e embutir isso em cada MP3
// incha o acervo para um painel que pinta meio-bloco de vinte collunas.
// A miniatura do CAA sahe JPEG, e o 307 que ella responde o curl já segue.
std::string url_da_capa(std::string_view release_mbid);

// O DESFECHO da resposta do CAA. QUATRO, e não os tres do MB, porque aqui o
// 404 tem sentido PROPRIO: é o CAA a dizer que a release não tem capa, que é
// resposta definitiva e se LEMBRA. A rede muda e o 5xx são Transitoria, que
// amanhã podem responder e por isso não se lembram; e o Recuo pára a caça.
enum class DesfechoDaCapa { Achada, SemCapa, Transitoria, Recuo };

// desfecho_da_capa — a leitura PURA do que consulta_mb_com_estado devolveu,
// para que a bateria a afira sem rede alguma.
DesfechoDaCapa desfecho_da_capa(DesfechoMB desfecho, long estado_http);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
