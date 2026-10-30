#include "tui/miniaturas.hpp"

#include <algorithm>

#include "tui/tabella.hpp"

namespace mysong::tui {

std::vector<MiniaturaDaLista> miniaturas_da_lista(
    const std::vector<Linha>& vista, std::size_t primeira, Rectangulo pauta,
    bool habilitadas,
    const std::function<std::filesystem::path(const std::string&)>& imagem) {
  std::vector<MiniaturaDaLista> miniaturas;
  if (!habilitadas || pauta.largura < LARGURA_DA_MINIATURA + 1 || primeira >= vista.size() || !imagem)
    return miniaturas;
  const auto quantas = std::min(pauta.altura / ALTURA_DA_FAIXA, vista.size() - primeira);
  miniaturas.reserve(quantas);
  for (std::size_t posicao = 0; posicao < quantas; ++posicao) {
    const auto indice = primeira + posicao;
    if (vista[indice].chave.empty()) continue;
    auto caminho = imagem(vista[indice].chave);
    if (caminho.empty()) continue;
    miniaturas.push_back({"miniatura_lista_" + std::to_string(posicao), std::move(caminho),
        {pauta.x, pauta.y + posicao * ALTURA_DA_FAIXA, LARGURA_DA_MINIATURA, ALTURA_DA_FAIXA}, indice});
  }
  return miniaturas;
}

std::vector<AlteracaoDaMiniatura> ReconciliadorDeMiniaturas::atualiza(
    const std::vector<MiniaturaDaLista>& desejadas) {
  std::map<std::string, MiniaturaDaLista> novas;
  for (const auto& miniatura : desejadas) novas.emplace(miniatura.identidade, miniatura);
  std::vector<AlteracaoDaMiniatura> alteracoes;
  for (const auto& [identidade, anterior] : atuais_)
    if (novas.find(identidade) == novas.end()) alteracoes.push_back({anterior, true});
  for (const auto& [identidade, atual] : novas) {
    const auto antes = atuais_.find(identidade);
    if (antes != atuais_.end()) {
      const auto& a = antes->second;
      const auto& b = atual.caixa;
      if (a.imagem == atual.imagem && a.caixa.x == b.x && a.caixa.y == b.y &&
          a.caixa.largura == b.largura && a.caixa.altura == b.altura) continue;
    }
    alteracoes.push_back({atual, false});
  }
  atuais_ = std::move(novas);
  return alteracoes;
}

}  // namespace mysong::tui
