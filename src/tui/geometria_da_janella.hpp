#pragma once

#include <cstddef>
#include <filesystem>

#include "nucleo/capa.hpp"
#include "tui/sala.hpp"

namespace mysong::tui {

enum class EstadoDaSobreposicao { Ausente, Oculta, Visivel };

struct PedidoDeGeometria {
  std::size_t largura = 0;
  std::size_t altura = 0;
  bool campo_aberto = false;
  bool foco_dentro = true;
  bool capa_com_foco = false;
  bool lousa_disponivel = false;
  std::filesystem::path capa;
  nucleo::Medida medida_da_capa;
};

struct GeometriaDoQuadro {
  std::size_t largura = 0;
  std::size_t altura = 0;
  Sala sala;
  std::filesystem::path capa;
  Rectangulo rectangulo_da_capa;
  EstadoDaSobreposicao sobreposicao = EstadoDaSobreposicao::Ausente;
  std::size_t geracao = 0;
};

GeometriaDoQuadro geometria_do_quadro(
    const PedidoDeGeometria& pedido,
    const GeometriaDoQuadro* anterior = nullptr) noexcept;

class ReconciliadorDaSobreposicao {
 public:
  void deseja(const GeometriaDoQuadro& quadro) noexcept;
  void confirma(std::size_t geracao) noexcept;
  bool pendente() const noexcept { return desejada_ > confirmada_; }
  std::size_t desejada() const noexcept { return desejada_; }
  std::size_t confirmada() const noexcept { return confirmada_; }

 private:
  std::size_t desejada_ = 0;
  std::size_t confirmada_ = 0;
};

}  // namespace mysong::tui
