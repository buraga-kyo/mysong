#pragma once

#include <atomic>
#include <optional>
#include <string>
#include <vector>

#include "nucleo/aquisicao.hpp"
#include "nucleo/catalogo.hpp"
#include "tui/correio.hpp"

namespace mysong::tui {

// Consumidos somente no fio da interface, antes da renderização. Prompts e
// menus congelam a navegação, mas não a contagem nem os avisos da varredura.
struct CorreiosDaJanella {
  CorreioDe<nucleo::Achado>& busca;
  CorreioDe<nucleo::Catalogo>& catalogo;
  CorreioDe<nucleo::Pedido>& playlist;
  CorreioDe<std::size_t>& contagem;
  CorreioDe<int>& varredura;
  std::atomic<bool>& acervo_novo;
};

template <class Carga>
struct Novidade {
  std::vector<Carga> carga;
  std::string recado;
};

struct NovidadesDaJanella {
  std::optional<Novidade<nucleo::Achado>> busca;
  std::optional<Novidade<nucleo::Catalogo>> catalogo;
  std::optional<Novidade<nucleo::Pedido>> playlist;
  std::optional<Novidade<std::size_t>> contagem;
  std::optional<Novidade<int>> varredura;
  bool acervo_novo = false;
};

NovidadesDaJanella colhe_novidades(CorreiosDaJanella correios,
                                 bool assenta, nucleo::Fonte fonte);

}  // namespace mysong::tui
