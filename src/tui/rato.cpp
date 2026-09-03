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

#include <cmath>

namespace mysong::tui {

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
  const CaixasDoCabecalho& alto = caixas.cabecalho;
  if (alto.aba_mysong.Contain(x, y)) return {Peca::Aba, 0, 0.0};
  if (alto.aba_playlists.Contain(x, y)) return {Peca::Aba, 1, 0.0};
  if (alto.aba_download.Contain(x, y)) return {Peca::Aba, 2, 0.0};
  if (alto.botao_tocar.Contain(x, y)) return {Peca::Pausa, 0, 0.0};
  if (alto.botao_anterior.Contain(x, y)) return {Peca::Anterior, 0, 0.0};
  if (alto.botao_seguinte.Contain(x, y)) return {Peca::Proxima, 0, 0.0};
  if (alto.volume.Contain(x, y)) return {Peca::Volume, 0, 0.0};
  if (alto.embaralhar.Contain(x, y)) return {Peca::Embaralhar, 0, 0.0};
  if (alto.repetir.Contain(x, y)) return {Peca::Repetir, 0, 0.0};
  if (alto.trilho.Contain(x, y))
    return {Peca::Progresso, 0, fracao_na(alto.trilho, x)};
  // O indice sahe ABSOLUTO: a caixa é da linha VISIVEL, e a rolagem somma-se
  // aqui, uma vez só, no logar que sabe quanto ella vale.
  for (std::size_t i = 0; i < caixas.linhas.size(); ++i)
    if (caixas.linhas[i].Contain(x, y))
      return {Peca::Linha, caixas.primeira_linha + i, 0.0};
  if (caixas.capa.Contain(x, y)) return {Peca::Capa, 0, 0.0};
  return {};  // o nome, o tempo, o rodapé e o espectro não respondem ao rato
}

GestoDoRato gesto_do_alvo(const Alvo& alvo, ftxui::Mouse::Button botao,
                          ftxui::Mouse::Motion movimento,
                          const EstadoDoRato& estado) noexcept {
  // Só o botão a DESCER conta, e a guarda vem ANTES de toda comparação. O
  // soltar chega sempre: sem esta linha, cada clique valeria por dous.
  if (movimento != ftxui::Mouse::Pressed) return {};
  const bool roda =
      botao == ftxui::Mouse::WheelUp || botao == ftxui::Mouse::WheelDown;
  if (botao != ftxui::Mouse::Left && !roda) return {};
  // Com o campo aberto o clique é o Escape, e nada mais: o rato não escreve no
  // termo, e a tela não ha de mudar debaixo de quem está a digitar.
  if (estado.digitando) return {Gesto::FechaCampo, 0, 0.0};
  const bool sobe = botao == ftxui::Mouse::WheelUp;
  if (roda) {
    if (alvo.peca == Peca::Linha)
      return {sobe ? Gesto::RodaSobe : Gesto::RodaDesce, LINHAS_POR_DENTE, 0.0};
    // A roda sobre o cabeçalho fica MUDA. Sobre a barra ella andava um degrau,
    // que era andar n'uma collunha; sobre uma fita de tres abas seria trocar de
    // secção por acaso, com o dedo a caminho de outra peça.
    return {};
  }
  switch (alvo.peca) {
    case Peca::Aba: return {Gesto::VaiParaAba, alvo.indice, 0.0};
    case Peca::Embaralhar: return {Gesto::Embaralha, 0, 0.0};
    case Peca::Repetir: return {Gesto::Repete, 0, 0.0};
    // O VOLUME cala e devolve, como o F9: é o gesto que o segmento já mostra,
    // que elle troca o numero pela palavra MUDO. Numero se não arrasta com o
    // dedo n'este modo de rato, e por isso o clique n'elle não assenta valor.
    case Peca::Volume: return {Gesto::Muda, 0, 0.0};
    case Peca::Linha:
      // Indice além da vista é o quadro que envelheceu entre a pintura e o
      // clique. Não se elege ás cegas: o quadro seguinte já mostra o certo.
      if (alvo.indice >= estado.quantas) return {};
      return {alvo.indice == estado.eleito ? Gesto::Toca : Gesto::Elege,
              alvo.indice, 0.0};
    case Peca::Capa:
    case Peca::Pausa: return {Gesto::PausaOuRetoma, 0, 0.0};
    case Peca::Anterior: return {Gesto::Anterior, 0, 0.0};
    case Peca::Proxima: return {Gesto::Proxima, 0, 0.0};
    case Peca::Progresso:
      // Duração que não presta não se busca. Buscar o segundo zero seria
      // affirmar o principio, e o que ha é a Casa ainda não saber a duração.
      if (!std::isfinite(estado.duracao) || estado.duracao <= 0.0) return {};
      return {Gesto::Busca, 0, estado.duracao * alvo.fracao};
    case Peca::Nada: break;
  }
  return {};
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
