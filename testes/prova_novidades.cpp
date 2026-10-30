#include <doctest/doctest.h>

#include "tui/novidades.hpp"

namespace {
namespace tui = mysong::tui;
namespace nucleo = mysong::nucleo;

struct Caixa {
  tui::CorreioDe<nucleo::Achado> busca;
  tui::CorreioDe<nucleo::Catalogo> catalogo;
  tui::CorreioDe<nucleo::Pedido> playlist;
  tui::CorreioDe<std::size_t> contagem;
  tui::CorreioDe<int> varredura;
  std::atomic<bool> acervo_novo{false};

  tui::NovidadesDaJanella colhe(bool assenta = true,
                               nucleo::Fonte fonte = nucleo::Fonte::YouTube) {
    return tui::colhe_novidades(
        {busca, catalogo, playlist, contagem, varredura, acervo_novo},
        assenta, fonte);
  }
};
}  // namespace

TEST_CASE("novidades congelam navegacao sem perder respostas pendentes") {
  Caixa caixa;
  nucleo::Achado achado;
  achado.fonte = nucleo::Fonte::YouTube;
  caixa.busca.poe({achado}, "busca pronta");
  caixa.catalogo.poe({nucleo::Catalogo{}}, "catalogo pronto");
  caixa.playlist.poe({nucleo::Pedido{}}, "playlist pronta");
  caixa.acervo_novo.store(true);
  caixa.contagem.poe({42}, {});
  caixa.varredura.poe({}, "aviso da varredura");

  const auto congeladas = caixa.colhe(false);
  CHECK_FALSE(congeladas.busca);
  CHECK_FALSE(congeladas.catalogo);
  CHECK_FALSE(congeladas.playlist);
  CHECK_FALSE(congeladas.acervo_novo);
  CHECK(caixa.acervo_novo.load());
  REQUIRE(congeladas.contagem);
  CHECK(congeladas.contagem->carga.front() == 42);
  REQUIRE(congeladas.varredura);
  CHECK(congeladas.varredura->recado == "aviso da varredura");

  const auto prontas = caixa.colhe();
  REQUIRE(prontas.busca);
  CHECK(prontas.busca->recado == "busca pronta");
  REQUIRE(prontas.catalogo);
  CHECK(prontas.catalogo->carga.size() == 1);
  REQUIRE(prontas.playlist);
  CHECK(prontas.playlist->carga.size() == 1);
  CHECK(prontas.acervo_novo);
  CHECK_FALSE(prontas.contagem);
  CHECK_FALSE(prontas.varredura);

  const auto consumidas = caixa.colhe();
  CHECK_FALSE(consumidas.busca);
  CHECK_FALSE(consumidas.catalogo);
  CHECK_FALSE(consumidas.playlist);
  CHECK_FALSE(consumidas.acervo_novo);
}

TEST_CASE("novidades descartam busca de fonte antiga uma unica vez") {
  Caixa caixa;
  nucleo::Achado antigo;
  antigo.fonte = nucleo::Fonte::YouTube;
  caixa.busca.poe({antigo}, "antiga");
  CHECK_FALSE(caixa.colhe(true, nucleo::Fonte::YouTubeMusic).busca);
  CHECK_FALSE(caixa.colhe().busca);

  antigo.fonte = nucleo::Fonte::YouTubeMusic;
  caixa.busca.poe({antigo}, "atual");
  const auto atual = caixa.colhe(true, nucleo::Fonte::YouTubeMusic);
  REQUIRE(atual.busca);
  CHECK(atual.busca->recado == "atual");
}

TEST_CASE("novidades distinguem busca vazia de ausencia de resposta") {
  Caixa caixa;
  caixa.busca.poe({}, "nada encontrado");
  const auto vazia = caixa.colhe();
  REQUIRE(vazia.busca);
  CHECK(vazia.busca->carga.empty());
  CHECK(vazia.busca->recado == "nada encontrado");
  CHECK_FALSE(caixa.colhe().busca);

  caixa.busca.poe({}, "resposta da rede antiga");
  CHECK_FALSE(caixa.colhe(true, nucleo::Fonte::Spotify).busca);
}
