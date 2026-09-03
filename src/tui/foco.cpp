// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO FOCO — src/tui/foco.cpp
// ══════════════════════════════════════════════════════════════════════════
// A lavra do que foco.hpp promette: a taboada das setas, a caixa de cada peça,
// e o salto pela geometria dos centros.
//
// DOMÍNIO ......... as caixas, a peça corrente e a direcção.
// CONTRA-DOMÍNIO .. a peça que fica com o foco, e o Alvo do rato.
// INVARIANTE ...... funcção alguma d'aqui lê o mundo nem lança. Os `switch`
//                   não levam `default`, para que peça nova deixe de compilar
//                   em vez de sahir sem caixa e sem gesto.
// Q.E.D. .......... sendo o salto funcção de valores, a bateria arma a tela em
//                   caixas escriptas á mão e afere cada seta de cada peça.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/foco.hpp"

#include <cstdlib>
#include <utility>

#include "tui/tokens.hpp"

namespace mysong::tui {

namespace {

// TODAS as peças, na ordem do enum, que é a ordem em que a tela se lê. É ella
// que desempata dous candidatos á mesma distancia: sem ordem declarada, o
// desempate ficaria por conta de como o compilador arrumou o laço.
constexpr Focavel kTodas[] = {
    Focavel::Pauta,       Focavel::AbaMySong, Focavel::AbaPlaylists,
    Focavel::AbaDownload, Focavel::Tocar,     Focavel::Anterior,
    Focavel::Seguinte,    Focavel::Volume,    Focavel::Embaralhar,
    Focavel::Repetir,     Focavel::Trilho,    Focavel::Capa,
};

// O centro em DOBRO, para que a conta corra em inteiros: caixa de largura par
// tem centro em meia collunha, e arredondá-lo trocaria desempates por engano.
int centro_x2(const ftxui::Box& caixa) noexcept {
  return caixa.x_min + caixa.x_max;
}
int centro_y2(const ftxui::Box& caixa) noexcept {
  return caixa.y_min + caixa.y_max;
}

}  // namespace

Direcao rumo_da_tecla(const ftxui::Event& tecla) noexcept {
  namespace f = ftxui;
  if (tecla == f::Event::ArrowUp) return Direcao::Cima;
  if (tecla == f::Event::ArrowDown) return Direcao::Baixo;
  if (tecla == f::Event::ArrowLeft) return Direcao::Esquerda;
  if (tecla == f::Event::ArrowRight) return Direcao::Dextra;
  return Direcao::Nenhuma;
}

ftxui::Box caixa_da_peca(const CaixasDaTela& caixas, Focavel qual) noexcept {
  const CaixasDoCabecalho& alto = caixas.cabecalho;
  switch (qual) {
    case Focavel::Pauta: return caixas.pauta;
    case Focavel::AbaMySong: return alto.aba_mysong;
    case Focavel::AbaPlaylists: return alto.aba_playlists;
    case Focavel::AbaDownload: return alto.aba_download;
    case Focavel::Tocar: return alto.botao_tocar;
    case Focavel::Anterior: return alto.botao_anterior;
    case Focavel::Seguinte: return alto.botao_seguinte;
    case Focavel::Volume: return alto.volume;
    case Focavel::Embaralhar: return alto.embaralhar;
    case Focavel::Repetir: return alto.repetir;
    case Focavel::Trilho: return alto.trilho;
    case Focavel::Capa: return caixas.capa;
  }
  return caixa_por_pintar();
}

Focavel salto(const CaixasDaTela& caixas, Focavel corrente,
              Direcao rumo) noexcept {
  if (rumo == Direcao::Nenhuma) return corrente;
  const ftxui::Box d_onde = caixa_da_peca(caixas, corrente);
  if (d_onde.IsEmpty()) return corrente;  // peça por pintar não salta
  const bool vertical = rumo == Direcao::Cima || rumo == Direcao::Baixo;
  const int sentido =
      (rumo == Direcao::Cima || rumo == Direcao::Esquerda) ? -1 : 1;
  const int meu_eixo = vertical ? centro_y2(d_onde) : centro_x2(d_onde);
  const int meu_traves = vertical ? centro_x2(d_onde) : centro_y2(d_onde);
  Focavel eleita = corrente;
  long melhor = 0;
  for (const Focavel qual : kTodas) {
    if (qual == corrente) continue;
    const ftxui::Box outra = caixa_da_peca(caixas, qual);
    if (outra.IsEmpty()) continue;
    const int eixo = vertical ? centro_y2(outra) : centro_x2(outra);
    // Á RÉ, ou no mesmo degrau, não é candidata: a seta anda para onde ella
    // aponta, e peça de centro egual ficaria a disputar com a corrente.
    const int adeante = (eixo - meu_eixo) * sentido;
    if (adeante <= 0) continue;
    const int traves = vertical ? centro_x2(outra) : centro_y2(outra);
    const long custo = adeante + static_cast<long>(PESO_DE_TRAVES) *
                                     std::abs(traves - meu_traves);
    if (eleita == corrente || custo < melhor) {
      eleita = qual;
      melhor = custo;
    }
  }
  return eleita;
}

Alvo alvo_do_foco(Focavel qual) noexcept {
  switch (qual) {
    case Focavel::AbaMySong: return {Peca::Aba, 0, 0.0};
    case Focavel::AbaPlaylists: return {Peca::Aba, 1, 0.0};
    case Focavel::AbaDownload: return {Peca::Aba, 2, 0.0};
    case Focavel::Tocar: return {Peca::Pausa, 0, 0.0};
    case Focavel::Anterior: return {Peca::Anterior, 0, 0.0};
    case Focavel::Seguinte: return {Peca::Proxima, 0, 0.0};
    case Focavel::Volume: return {Peca::Volume, 0, 0.0};
    case Focavel::Embaralhar: return {Peca::Embaralhar, 0, 0.0};
    case Focavel::Repetir: return {Peca::Repetir, 0, 0.0};
    case Focavel::Capa: return {Peca::Capa, 0, 0.0};
    // As duas que a tecla NÃO aperta, e o porquê está no cabeçalho d'este
    // tractado: a pauta tem taboada propria, e o trilho pede collunha.
    case Focavel::Trilho:
    case Focavel::Pauta: break;
  }
  return {};
}

ftxui::Element orla_do_foco(ftxui::Element dentro) {
  const tokens::Triade c = tokens::rgb(tokens::glow_core);
  // A tinta veste-se por FÓRA da orla, e o que a arte pinta por dentro vae por
  // cima: o `FgColor` do FTXUI assenta a côr na caixa toda e SÓMENTE depois
  // desce ao filho, donde a capa não sahe d'aqui tingida de violeta.
  return ftxui::border(std::move(dentro)) |
         ftxui::color(ftxui::Color::RGB(c.r, c.g, c.b));
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
