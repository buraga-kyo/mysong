#pragma once

#include <filesystem>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "tui/navegador.hpp"
#include "tui/sala.hpp"

namespace mysong::tui {

struct MiniaturaDaLista {
  std::string identidade;
  std::filesystem::path imagem;
  Rectangulo caixa;
  std::size_t indice = 0;
};

// Planeja somente linhas completas e visíveis. O carregador pode usar cache;
// não se abre imagem de uma faixa que esteja fora da fatia mostrada.
std::vector<MiniaturaDaLista> miniaturas_da_lista(
    const std::vector<Linha>& vista, std::size_t primeira, Rectangulo pauta,
    bool habilitadas,
    const std::function<std::filesystem::path(const std::string&)>& imagem);

struct AlteracaoDaMiniatura {
  MiniaturaDaLista miniatura;
  bool remover = false;
};

// Quadros idênticos não produzem ordens, inclusive com o espectro animado.
class ReconciliadorDeMiniaturas {
 public:
  std::vector<AlteracaoDaMiniatura> atualiza(
      const std::vector<MiniaturaDaLista>& desejadas);

 private:
  std::map<std::string, MiniaturaDaLista> atuais_;
};

}  // namespace mysong::tui
