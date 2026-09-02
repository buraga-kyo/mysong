// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO RATO — src/tui/rato.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação das duas taboadas. Vale aqui a regra do cabeçalho: cousa
// alguma d'este arquivo sabe o que é um Navegador, um Menu ou um Tocador.
//
// DOMÍNIO ......... as caixas, o ponto, o botão, o movimento e o estado.
// CONTRA-DOMÍNIO .. o Alvo e o Gesto, deterministicos.
// INVARIANTE ...... funcção alguma d'aqui lança, nem toca em estado que viva
//                   fóra dos seus parametros.
// Q.E.D. .......... sendo tudo funcção de valores, a bateria arma a tela em
//                   caixas escriptas á mão e afere o alvo contra alvo escripto.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/rato.hpp"

namespace mysong::tui {

ftxui::Box CaixasDoTransporte::progresso() const noexcept {
  // A união das metades, e sómente das que se pintaram. O `Box::Union` com uma
  // caixa VAZIA arrastaria o canto para a origem, e a barra passaria a cobrir
  // meia tela; no principio e no fim da faixa é isso que uma das metades é.
  if (barra_cheia.IsEmpty()) return barra_vazia;
  if (barra_vazia.IsEmpty()) return barra_cheia;
  return ftxui::Box::Union(barra_cheia, barra_vazia);
}

namespace {

// fracao_na — onde, entre zero e um, o ponto cahiu dentro da caixa. A primeira
// collunha vale zero e a ultima vale um, e não meia collunha em cada ponta:
// quem clica na ultima quer o fim da faixa, e não noventa e tantos por cento
// d'ella. Caixa de uma collunha só, ou vazia, dá o principio.
double fracao_na(const ftxui::Box& caixa, int x) noexcept {
  const int largura = caixa.x_max - caixa.x_min;
  if (largura <= 0) return 0.0;
  const double razao = static_cast<double>(x - caixa.x_min) / largura;
  if (razao < 0.0) return 0.0;
  return razao > 1.0 ? 1.0 : razao;
}

}  // namespace

Alvo alvo_do_ponto(const CaixasDaTela& caixas, int x, int y) noexcept {
  // A ordem não é preferencia: caixa alguma se sobrepõe a outra, e a primeira
  // que contiver o ponto é a UNICA que o contem. Percorre-se pela ordem em que
  // a tela se lê, do alto para baixo, que é o que torna a lista revisavel.
  for (std::size_t i = 0; i < caixas.degraus.size(); ++i)
    if (caixas.degraus[i].Contain(x, y)) return {Peca::Degrau, i, 0.0};
  // O indice sahe ABSOLUTO: a caixa é da linha VISIVEL, e a rolagem somma-se
  // aqui, uma vez só, no logar que sabe quanto ella vale.
  for (std::size_t i = 0; i < caixas.linhas.size(); ++i)
    if (caixas.linhas[i].Contain(x, y))
      return {Peca::Linha, caixas.primeira_linha + i, 0.0};
  if (caixas.capa.Contain(x, y)) return {Peca::Capa, 0, 0.0};
  const CaixasDoTransporte& baixo = caixas.transporte;
  if (baixo.anterior.Contain(x, y)) return {Peca::Anterior, 0, 0.0};
  if (baixo.pausa.Contain(x, y)) return {Peca::Pausa, 0, 0.0};
  if (baixo.proxima.Contain(x, y)) return {Peca::Proxima, 0, 0.0};
  const ftxui::Box barra = baixo.progresso();
  if (barra.Contain(x, y)) return {Peca::Progresso, 0, fracao_na(barra, x)};
  return {};  // a orla, o rodapé e o espectro não respondem ao rato
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
