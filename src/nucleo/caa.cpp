// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO COVER ART ARCHIVE, LAVRA — src/nucleo/caa.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o que o cabeçalho promette. As puras primeiro, e o que toca banco e
// rede por baixo, á maneira do musicbrainz.cpp, que é o irmão d'esta peça.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/caa.hpp"

namespace mysong::nucleo {
namespace {

// A raiz das capas por release. N'uma constante, para que haja UM endereço.
constexpr char kRaizDoCaa[] = "https://coverartarchive.org/release/";

}  // namespace

std::string url_da_capa(std::string_view release_mbid) {
  // O MBID vem do proprio MB, e ainda assim não se interpola cru: é a mesma
  // doutrina da url_da_ficha, e o escapa_url é o mesmo d'ella.
  if (release_mbid.empty()) return {};
  return std::string(kRaizDoCaa) + escapa_url(release_mbid) + "/front-500";
}

DesfechoDaCapa desfecho_da_capa(DesfechoMB desfecho, long estado_http) {
  // O Recuo e o Achado passam taes e quaes; o Falhou reparte-se pelo estado:
  // o 404 é o CAA a dizer «capa não ha», definitivo, e todo o resto (rede
  // muda com estado zero, 5xx sem recuo, outra recusa) é passageiro.
  if (desfecho == DesfechoMB::Recuo) return DesfechoDaCapa::Recuo;
  if (desfecho == DesfechoMB::Achado) return DesfechoDaCapa::Achada;
  return estado_http == 404 ? DesfechoDaCapa::SemCapa
                            : DesfechoDaCapa::Transitoria;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
