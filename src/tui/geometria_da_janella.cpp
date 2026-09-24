#include "tui/geometria_da_janella.hpp"

#include <algorithm>

namespace mysong::tui {

GeometriaDoQuadro geometria_do_quadro(
    const PedidoDeGeometria& pedido,
    const GeometriaDoQuadro* anterior) noexcept {
  GeometriaDoQuadro quadro;
  quadro.largura = pedido.largura;
  quadro.altura = pedido.altura;
  quadro.sala = sala_da_tela(pedido.largura, pedido.altura,
                             pedido.campo_aberto);
  quadro.capa = pedido.capa;
  const auto termina = [&] {
    const auto& r = quadro.rectangulo_da_capa;
    const bool mudou = anterior == nullptr ||
        anterior->largura != quadro.largura || anterior->altura != quadro.altura ||
        anterior->capa != quadro.capa || anterior->sobreposicao != quadro.sobreposicao ||
        anterior->rectangulo_da_capa.x != r.x || anterior->rectangulo_da_capa.y != r.y ||
        anterior->rectangulo_da_capa.largura != r.largura ||
        anterior->rectangulo_da_capa.altura != r.altura;
    quadro.geracao = anterior == nullptr ? 1 : anterior->geracao + (mudou ? 1 : 0);
    return quadro;
  };

  if (pedido.capa.empty()) {
    quadro.sobreposicao = EstadoDaSobreposicao::Ausente;
    return termina();
  }
  if (!pedido.lousa_disponivel || !pedido.foco_dentro ||
      quadro.sala.capa.vazio()) {
    quadro.sobreposicao = EstadoDaSobreposicao::Oculta;
    return termina();
  }
  const nucleo::Retangulo medida = nucleo::rectangulo_da_capa(
      pedido.medida_da_capa, quadro.sala.capa.largura,
      quadro.sala.capa.altura, nucleo::CELLULA_DA_CASA);
  if (medida.collunas == 0 || medida.linhas == 0) {
    quadro.sobreposicao = EstadoDaSobreposicao::Oculta;
    return termina();
  }
  quadro.rectangulo_da_capa = {
      quadro.sala.capa.x +
          (quadro.sala.capa.largura > medida.collunas
               ? (quadro.sala.capa.largura - medida.collunas) / 2
               : 0),
      quadro.sala.capa.y + (pedido.capa_com_foco ? 1u : 0u), medida.collunas,
      medida.linhas};
  quadro.sobreposicao = EstadoDaSobreposicao::Visivel;
  return termina();
}

void ReconciliadorDaSobreposicao::deseja(
    const GeometriaDoQuadro& quadro) noexcept {
  desejada_ = std::max(desejada_, quadro.geracao);
}

void ReconciliadorDaSobreposicao::confirma(std::size_t geracao) noexcept {
  if (geracao == desejada_) confirmada_ = geracao;
}

}  // namespace mysong::tui
