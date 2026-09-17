#pragma once

#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include <ftxui/screen/box.hpp>

#include "nucleo/letra.hpp"
#include "tui/ajuda.hpp"
#include "tui/dinamica_do_espectro.hpp"
#include "tui/foco.hpp"
#include "tui/geometria_da_janella.hpp"
#include "tui/letra_viva.hpp"
#include "tui/menu_contexto.hpp"
#include "tui/rato.hpp"
#include "tui/sala.hpp"

namespace mysong::tui {

// Estado puramente visual que atravessa quadros. Não possui motor, banco,
// socket, rede nem processo externo.
struct EstadoDaJanella {
  std::size_t primeira_linha = 0;
  CaixasDaTela caixas;
  Focavel foco = Focavel::Pauta;
  MenuDeContexto menu;
  Ajuda ajuda;
  Arrasto arrasto;
  AssignaturaDaChapa assignatura_posta;
  bool chapa_posta = false;
  ftxui::Box caixa_da_ajuda = caixa_por_pintar();
  std::vector<nucleo::LinhaDaLetra> letra;
  std::string letra_de_qual;
  Ficha ficha;
  std::vector<float> onda_da_faixa;
  std::atomic<bool> mostra_letra{true};
  std::optional<GeometriaDoQuadro> geometria_anterior;
  ReconciliadorDaSobreposicao reconciliador_da_capa;
  DinamicaDoEspectro dinamica_do_espectro;
};

}  // namespace mysong::tui
