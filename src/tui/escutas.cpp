#include "tui/escutas.hpp"
#include "tui/tokens.hpp"
#include <algorithm>

namespace mysong::tui {
namespace {
using namespace ftxui;
Color cor(std::string_view token) {
  const auto c = tokens::rgb(token);
  return Color::RGB(c.r, c.g, c.b);
}
std::string tempo(std::int64_t ms) {
  const auto s = std::max<std::int64_t>(0, ms / 1000);
  if (s < 60) return std::to_string(s) + "s";
  if (s < 3600) return std::to_string(s / 60) + "min " + std::to_string(s % 60) + "s";
  return std::to_string(s / 3600) + "h " + std::to_string((s % 3600) / 60) + "min";
}
Element ranking(const std::vector<nucleo::ItemDeEscuta>& itens, const std::string& titulo,
                std::size_t largura, std::size_t quantos, bool musicas) {
  Elements linhas{text(" " + titulo) | bold | color(cor(tokens::glow_ice)), separator()};
  if (itens.empty()) linhas.push_back(paragraph(" Ouça suas músicas para preencher este ranking.") | dim);
  const auto maximo = itens.empty() ? 1 : std::max<std::int64_t>(1, itens.front().milissegundos);
  for (std::size_t i = 0; i < std::min(quantos, itens.size()); ++i) {
    const auto& item = itens[i];
    const auto nome = " " + std::to_string(i + 1) + ". " + item.nome + (musicas ? " · " + item.artista : "");
    linhas.push_back(text(nome) | xflex | color(cor(i == 0 ? tokens::glow_ice : tokens::text_primary)));
    linhas.push_back(hbox({text(" "), gauge(static_cast<float>(item.milissegundos) / maximo) | xflex,
        text(" " + tempo(item.milissegundos) + " · " + std::to_string(item.reproducoes) + "x ")}) |
        color(cor(i == 0 ? tokens::ok : tokens::v400)));
  }
  return vbox(std::move(linhas)) | size(WIDTH, EQUAL, static_cast<int>(largura));
}
}  // namespace

ftxui::Element painel_de_escutas(const nucleo::ResumoDasEscutas& r, int dias,
                               std::size_t largura, std::size_t altura) {
  using namespace ftxui;
  if (largura == 0 || altura == 0) return emptyElement();
  const std::string periodo = dias == 7 ? "ÚLTIMOS 7 DIAS" : dias == 30 ? "ÚLTIMOS 30 DIAS" : "TODO O PERÍODO";
  Elements partes;
  partes.push_back(hbox({text(" SUAS ESCUTAS ") | bold | color(cor(tokens::glow_ice)),
      filler(), text(" " + periodo + " ") | dim}));
  partes.push_back(separator());
  partes.push_back(text(" " + tempo(r.milissegundos) + " ouvidos · " + std::to_string(r.reproducoes) +
      " reproduções · " + std::to_string(r.musicas) + " músicas · " + std::to_string(r.artistas) + " artistas") | xflex | bold);
  if (altura >= 10) {
    partes.push_back(text(r.atual.nome.empty() ? " Música atual: ainda sem tempo registrado"
        : " Nesta música: " + tempo(r.atual.milissegundos) + " · " + r.atual.nome + " · " + r.atual.artista) | xflex);
    partes.push_back(separator());
    const bool colunas = largura >= 100;
    const auto vagas = altura > 12 ? altura - 12 : 0;
    const auto quantidade = std::min<std::size_t>(10, colunas ? vagas / 2 : vagas / 4);
    const auto lado = colunas ? (largura - 3) / 2 : largura;
    auto musicas = ranking(r.mais_musicas, "MÚSICAS MAIS OUVIDAS", lado, quantidade, true);
    auto artistas = ranking(r.mais_artistas, "ARTISTAS MAIS OUVIDOS", lado, quantidade, false);
    partes.push_back(colunas ? hbox({musicas, separator(), artistas}) : vbox({musicas, artistas}));
  }
  partes.push_back(filler());
  if (!r.erro.empty()) partes.push_back(paragraph(" " + r.erro) | color(cor(tokens::warn)));
  if (altura >= 12) {
    partes.push_back(text(" Uma reprodução conta após 30s ou metade da faixa, se ela for mais curta.") | xflex | dim);
    partes.push_back(text(" Pausas, mudo e saltos não somam tempo. Histórico salvo neste computador.") | xflex | dim);
  }
  partes.push_back(text(" e/Esc: voltar   1: 7d   2: 30d   3: tudo   espaço: pausar") | xflex | color(cor(tokens::v300)));
  return vbox(std::move(partes)) | bgcolor(cor(tokens::base)) | color(cor(tokens::text_primary)) |
      size(WIDTH, EQUAL, static_cast<int>(largura)) | size(HEIGHT, EQUAL, static_cast<int>(altura));
}
}  // namespace mysong::tui
