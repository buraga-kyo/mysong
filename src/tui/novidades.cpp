#include "tui/novidades.hpp"

#include <utility>

namespace mysong::tui {
namespace {

template <class Carga>
std::optional<Novidade<Carga>> colhe(CorreioDe<Carga>& correio) {
  Novidade<Carga> novidade;
  if (!correio.colhe(&novidade.carga, &novidade.recado)) return std::nullopt;
  return novidade;
}

}  // namespace

NovidadesDaJanella colhe_novidades(CorreiosDaJanella correios,
                                 bool assenta, nucleo::Fonte fonte) {
  NovidadesDaJanella novidades;
  if (assenta) {
    novidades.busca = colhe(correios.busca);
    if (novidades.busca) {
      const auto& achados = novidades.busca->carga;
      // Respostas vazias vêm da rede; Spotify responde pelo catálogo local.
      const bool casa = achados.empty() ? fonte != nucleo::Fonte::Spotify
                                       : achados.front().fonte == fonte;
      if (!casa) novidades.busca.reset();
    }
    novidades.catalogo = colhe(correios.catalogo);
    novidades.playlist = colhe(correios.playlist);
    novidades.acervo_novo = correios.acervo_novo.exchange(false);
  }
  novidades.contagem = colhe(correios.contagem);
  novidades.varredura = colhe(correios.varredura);
  return novidades;
}

}  // namespace mysong::tui
