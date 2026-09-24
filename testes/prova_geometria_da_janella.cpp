#include <doctest/doctest.h>

#include "tui/geometria_da_janella.hpp"
#include "nucleo/lousa.hpp"
#include <cerrno>

namespace tui = mysong::tui;

namespace {

tui::PedidoDeGeometria pedido(std::size_t largura, std::size_t altura) {
  return {largura, altura, false, true, false, true, "/tmp/capa.jpg",
          {1280, 720}};
}

}  // namespace

TEST_CASE("redimensionamento ocupado entrega somente a última geometria") {
  bool ocupado = true;
  std::string recebido;
  mysong::nucleo::EscoadouroDaLousa saida([&](std::string_view bytes) -> ssize_t {
    if (ocupado) { errno = EAGAIN; return -1; }
    recebido += bytes;
    return static_cast<ssize_t>(bytes.size());
  });
  std::string ultima;
  for (const auto largura : {167u, 130u, 100u, 99u, 120u}) {
    const auto quadro = tui::geometria_do_quadro(pedido(largura, 45));
    const auto& r = quadro.rectangulo_da_capa;
    const bool visivel = quadro.sobreposicao == tui::EstadoDaSobreposicao::Visivel;
    ultima = visivel ? mysong::nucleo::ordem_de_por("capa", "/tmp/capa.jpg",
        r.x, r.y, r.largura, r.altura) : mysong::nucleo::ordem_de_tirar("capa");
    saida.deseja("capa", ultima, visivel);
  }
  CHECK(saida.pendente());
  ocupado = false;
  CHECK(saida.drena());
  CHECK(recebido == ultima);
  CHECK_FALSE(saida.pendente());
}

TEST_CASE("a geometria do quadro nasce inteira da mesma medida da tela") {
  const tui::GeometriaDoQuadro quadro =
      tui::geometria_do_quadro(pedido(167, 67));
  CHECK(quadro.largura == 167);
  CHECK(quadro.altura == 67);
  CHECK(quadro.sala.painel.x == 84);
  CHECK(quadro.rectangulo_da_capa.x == 84);
  CHECK(quadro.rectangulo_da_capa.y == 1);
  CHECK(quadro.rectangulo_da_capa.largura == 83);
  CHECK(quadro.rectangulo_da_capa.altura == 22);
  CHECK(quadro.sobreposicao == tui::EstadoDaSobreposicao::Visivel);
}

TEST_CASE("a sequencia de tamanhos nunca produz ordem de dimensão zero") {
  tui::GeometriaDoQuadro anterior;
  bool ha_anterior = false;
  for (const auto& [largura, altura] :
       {std::pair<std::size_t, std::size_t>{167, 67}, {130, 50}, {100, 40},
        {99, 40}, {0, 0}, {120, 45}}) {
    const tui::GeometriaDoQuadro quadro = tui::geometria_do_quadro(
        pedido(largura, altura), ha_anterior ? &anterior : nullptr);
    if (quadro.sobreposicao == tui::EstadoDaSobreposicao::Visivel) {
      CHECK(quadro.rectangulo_da_capa.largura > 0);
      CHECK(quadro.rectangulo_da_capa.altura > 0);
    } else {
      CHECK(quadro.rectangulo_da_capa.vazio());
    }
    anterior = quadro;
    ha_anterior = true;
  }
  CHECK(anterior.geracao == 6);
  CHECK(anterior.sobreposicao == tui::EstadoDaSobreposicao::Visivel);
}

TEST_CASE("a geração só cresce quando a medida da tela muda") {
  const tui::GeometriaDoQuadro primeira =
      tui::geometria_do_quadro(pedido(100, 40));
  const tui::GeometriaDoQuadro repetida =
      tui::geometria_do_quadro(pedido(100, 40), &primeira);
  const tui::GeometriaDoQuadro nova =
      tui::geometria_do_quadro(pedido(120, 45), &repetida);
  CHECK(primeira.geracao == 1);
  CHECK(repetida.geracao == 1);
  CHECK(nova.geracao == 2);
}

TEST_CASE("capa ausente oculta e visivel são estados distintos") {
  auto qual = pedido(120, 45);
  qual.capa.clear();
  CHECK(tui::geometria_do_quadro(qual).sobreposicao ==
        tui::EstadoDaSobreposicao::Ausente);
  qual.capa = "/tmp/capa.jpg";
  qual.lousa_disponivel = false;
  CHECK(tui::geometria_do_quadro(qual).sobreposicao ==
        tui::EstadoDaSobreposicao::Oculta);
  qual.lousa_disponivel = true;
  CHECK(tui::geometria_do_quadro(qual).sobreposicao ==
        tui::EstadoDaSobreposicao::Visivel);
}

TEST_CASE("confirmação tardia não apaga a geração mais nova") {
  tui::ReconciliadorDaSobreposicao reconciliador;
  const tui::GeometriaDoQuadro velha =
      tui::geometria_do_quadro(pedido(100, 40));
  const tui::GeometriaDoQuadro nova =
      tui::geometria_do_quadro(pedido(120, 45), &velha);
  reconciliador.deseja(velha);
  reconciliador.deseja(nova);
  reconciliador.confirma(velha.geracao);
  CHECK(reconciliador.pendente());
  CHECK(reconciliador.confirmada() == 0);
  reconciliador.confirma(nova.geracao);
  CHECK_FALSE(reconciliador.pendente());
  CHECK(reconciliador.confirmada() == nova.geracao);
}
