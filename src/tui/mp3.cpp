#include "tui/mp3.hpp"
#include "tui/tokens.hpp"
#include <filesystem>

namespace mysong::tui {
namespace {
ftxui::Color cor(std::string_view token) {
  const auto c=tokens::rgb(token);return ftxui::Color::RGB(c.r,c.g,c.b);
}
}
bool usa_mp3(std::size_t largura, std::size_t altura) { return largura < 76 || altura < 9; }
ftxui::Element controles_do_mp3(const Retracto& r,std::size_t largura,CaixasDoCabecalho* caixas) {
  using namespace ftxui;
  const bool toca=r.video ? !r.video_pausada : r.estado==nucleo::Estado::Tocando;
  const auto botao=[&](std::string rotulo,Box* caixa) {
    auto e=text(std::move(rotulo)) | bold | color(cor(tokens::glow_ice)) | bgcolor(cor(tokens::panel_hi));
    return caixa ? e | reflect(*caixa) : e;
  };
  Elements botoes;
  if (largura >= 3) {
    botoes.push_back(botao(largura >= 14 ? " |< " : "‹",caixas ? &caixas->botao_anterior : nullptr));
    botoes.push_back(botao(largura >= 14 ? (toca ? " || " : " ▶  ") : (toca ? "Ⅱ" : "▶"),
        caixas ? &caixas->botao_tocar : nullptr));
    botoes.push_back(botao(largura >= 14 ? " >| " : "›",caixas ? &caixas->botao_seguinte : nullptr));
  } else botoes.push_back(botao(toca ? "Ⅱ" : "▶",caixas ? &caixas->botao_tocar : nullptr));
  return hbox({filler(),hbox(std::move(botoes)),filler()}) | size(WIDTH,EQUAL,static_cast<int>(largura));
}
ftxui::Element elemento_do_mp3(const Retracto& r,std::size_t largura,std::size_t altura,CaixasDoCabecalho* caixas) {
  using namespace ftxui;
  if (caixas) *caixas={};
  if (!largura || !altura) return emptyElement();
  Elements partes;
  if (altura >= 3) {
    const auto rotulo = largura >= 20 ? " MY " + std::to_string(r.acervo) + " SONG's" : " " + std::to_string(r.acervo) + " músicas";
    partes.push_back(text(rotulo) | bold | color(cor(tokens::glow_ice)));
    const auto titulo=r.titulo.empty() ? "Escolha uma música" : std::filesystem::path(r.titulo).stem().string();
    partes.push_back(text(" " + titulo) | xflex | color(cor(tokens::ok)));
  }
  if (altura >= 6) {
    partes.push_back(text(" " + mm_ss(r.posicao) + " / " + mm_ss(r.duracao)) | color(cor(tokens::ok)));
    auto barra=gauge(r.duracao>0 ? static_cast<float>(r.posicao/r.duracao) : 0.f) | color(cor(tokens::ok));
    if (caixas) barra=barra | reflect(caixas->trilho);
    partes.push_back(barra);
  }
  if (altura >= 8) partes.push_back(text(std::string(" ") + (r.embaralhado ? "ALEATÓRIO " : "") +
      (r.repeticao==nucleo::Repeticao::Uma ? "REPETE UMA" : r.repeticao==nucleo::Repeticao::Todas ? "REPETE TODAS" : "")) | dim);
  partes.push_back(filler());
  partes.push_back(controles_do_mp3(r,largura,caixas));
  if (altura >= 10) partes.push_back(text(" espaço: pausa  n/p: faixa  ?: ajuda") | xflex | dim);
  return vbox(std::move(partes)) | bgcolor(cor(tokens::base)) | color(cor(tokens::text_primary)) |
      size(WIDTH,EQUAL,static_cast<int>(largura)) | size(HEIGHT,EQUAL,static_cast<int>(altura));
}
}
