#include "tui/separacao.hpp"
#include "tui/tokens.hpp"
#include <algorithm>

namespace mysong::tui {
namespace {
ftxui::Color cor(std::string_view token) {
  const auto c = tokens::rgb(token); return ftxui::Color::RGB(c.r,c.g,c.b);
}
bool mesma(const std::string& caminho, const std::filesystem::path& outro) {
  std::error_code erro;
  return !caminho.empty() && std::filesystem::equivalent(caminho, outro, erro) && !erro;
}
}
bool PainelDeSeparacao::tecla(const ftxui::Event& e, nucleo::SeparadorDeAudio& separador, nucleo::Tocador& tocador) {
  using ftxui::Event;
  if (e == Event::Escape || e == Event::Character('i') || e == Event::Character('q')) { aberto = false; return true; }
  if (e == Event::Custom || e.is_mouse()) return true;
  const auto trabalho = separador.retrato();
  const auto atual = tocador.retracto();
  aviso.clear();
  if (e == Event::Character('s') || e == Event::Character('S')) {
    if (alvo.empty()) aviso = "Toque uma música local para começar.";
    else if (!separador.inicia(alvo)) aviso = "Uma separação já está em andamento.";
    return true;
  }
  if (e == Event::Character('c')) { separador.cancela(); return true; }
  if (e == Event::Character('0')) {
    if (atual.faixa && tocador.retorna_original(*atual.faixa)) aviso = "Ouvindo o arquivo original.";
    return true;
  }
  if (e == Event::ArrowUp) { selecionada = (selecionada + 3) % 4; return true; }
  if (e == Event::ArrowDown) { selecionada = (selecionada + 1) % 4; return true; }
  if (e == Event::Character(' ')) {
    if (atual.estado == nucleo::Estado::Tocando) tocador.pausar();
    else if (atual.estado == nucleo::Estado::Pausado) tocador.retomar();
    return true;
  }
  bool aplicar = true;
  auto novos = volumes;
  if (e == Event::Character('1')) novos = {100,0,0,0};
  else if (e == Event::Character('2')) novos = {0,100,100,100};
  else if (e == Event::Character('3')) novos = {0,100,0,0};
  else if (e == Event::Character('4')) novos = {0,0,100,0};
  else if (e == Event::Character('5')) novos = {0,0,0,100};
  else if (e == Event::Character('6')) novos = {100,100,100,100};
  else if (e == Event::ArrowLeft) novos[selecionada] = std::max(0,novos[selecionada]-10);
  else if (e == Event::ArrowRight) novos[selecionada] = std::min(100,novos[selecionada]+10);
  else if (e == Event::Character('m')) novos[selecionada] = novos[selecionada] ? 0 : 100;
  else if (e != Event::Return) aplicar = false;
  if (!aplicar) return true;
  if (trabalho.estado != nucleo::EstadoDaSeparacao::Pronto || !mesma(alvo,trabalho.original)) {
    aviso = "Pressione s e aguarde as quatro partes ficarem prontas."; return true;
  }
  if (e == Event::Return && (!atual.faixa || !mesma(*atual.faixa,trabalho.original))) {
    if (!tocador.tocar(alvo)) { aviso = "Não foi possível tocar esta música."; return true; }
  }
  const auto agora = tocador.retracto();
  if (!agora.faixa || !mesma(*agora.faixa,trabalho.original)) {
    aviso = "Outra música está tocando. Enter toca a música deste painel."; return true;
  }
  if (tocador.aplica_mistura(*agora.faixa,trabalho.arquivos,novos)) volumes = novos;
  else aviso = "Não foi possível aplicar a mistura. O áudio voltou ao original.";
  return true;
}
ftxui::Element painel_de_separacao(const PainelDeSeparacao& p, const nucleo::RetratoDaSeparacao& r,
                                  const nucleo::Retracto& atual, int largura, int altura) {
  using namespace ftxui;
  Elements linhas{text(" SEPARAR E MISTURAR ") | bold | color(cor(tokens::glow_ice)), separator(),
      text(" " + (p.alvo.empty() ? "Nenhuma música selecionada" : std::filesystem::path(p.alvo).filename().string())) | xflex};
  if (!r.original.empty()) linhas.push_back(text(" Partes de: " + r.original.filename().string()) | xflex | dim);
  linhas.push_back(text(" " + (r.detalhe.empty() ? "Pressione s para preparar as quatro partes desta música." : r.detalhe)) | xflex);
  if (r.porcentagem) linhas.push_back(hbox({text(" "),gauge(*r.porcentagem/100.f) | flex,
      text(" " + std::to_string(*r.porcentagem) + "% ")}) | color(cor(tokens::glow_ice)));
  if (nucleo::separacao_ocupada(r.estado)) {
    const auto segundos = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now()-r.inicio).count();
    linhas.push_back(text(" Em andamento há " + std::to_string(segundos) + "s · você pode fechar este painel e continuar ouvindo.") | xflex | dim);
  }
  linhas.push_back(separator());
  for (int i=0;i<4;++i) {
    auto linha = hbox({text(i==p.selecionada ? " > " : "   "), text(nucleo::NOMES_DAS_PARTES[i]) | size(WIDTH,EQUAL,22),
        gauge(p.volumes[i]/100.f) | flex,text(" " + std::to_string(p.volumes[i]) + "% ")});
    linhas.push_back(linha | color(cor(i==p.selecionada ? tokens::glow_ice : tokens::text_primary)));
  }
  linhas.push_back(text(atual.audio_separado ? " Ouvindo a mistura das partes" : " Ouvindo o arquivo original") | color(cor(tokens::ok)));
  linhas.push_back(filler());
  if (!p.aviso.empty()) linhas.push_back(paragraph(" " + p.aviso) | color(cor(tokens::warn)));
  linhas.push_back(text(" s: separar   c: cancelar   0: original   1: voz   2: instrumental") | xflex);
  linhas.push_back(text(" 3: bateria   4: baixo   5: demais   6: todas   Enter: tocar e aplicar") | xflex);
  linhas.push_back(text(" ↑/↓: escolher   ←/→: volume   m: silenciar   espaço: pausar   i/Esc: voltar") | xflex | color(cor(tokens::v300)));
  return vbox(std::move(linhas)) | bgcolor(cor(tokens::base)) | color(cor(tokens::text_primary)) |
      size(WIDTH,EQUAL,std::max(1,largura)) | size(HEIGHT,EQUAL,std::max(1,altura));
}
}
