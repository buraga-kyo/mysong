// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO RATO, src/tui/rato.cpp
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

// fracao_na, onde, entre zero e um, o ponto cahiu dentro da caixa. A primeira
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
  if (alto.anima.Contain(x, y)) return {Peca::Anima, 0, 0.0};
  if (alto.ajuda.Contain(x, y)) return {Peca::Ajuda, 0, 0.0};
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
  const bool direito = botao == ftxui::Mouse::Right;
  if (botao != ftxui::Mouse::Left && !roda && !direito) return {};
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
  // O BOTÃO DIREITO abre o menu, e sómente sobre uma LINHA: no cabeçalho e na
  // capa não ha faixa alguma de que o menu fosse, e menu sem faixa alvo seria
  // caixa a perguntar sobre nada. Indice além da vista tambem se recusa, pela
  // razão do clique esquerdo: é o quadro que envelheceu entre a pintura e o
  // clique, e abrir ás cegas poria o nome de uma faixa por cima de outra.
  if (direito) {
    if (alvo.peca != Peca::Linha || alvo.indice >= estado.quantas) return {};
    return {Gesto::AbreMenu, alvo.indice, 0.0};
  }
  switch (alvo.peca) {
    case Peca::Aba: return {Gesto::VaiParaAba, alvo.indice, 0.0};
    case Peca::Embaralhar: return {Gesto::Embaralha, 0, 0.0};
    case Peca::Repetir: return {Gesto::Repete, 0, 0.0};
    case Peca::Anima: return {Gesto::Anima, 0, 0.0};
    // O VOLUME cala e devolve, como o F9: é o gesto que o segmento já mostra,
    // que elle troca o numero pela palavra MUDO. Numero se não arrasta com o
    // dedo n'este modo de rato, e por isso o clique n'elle não assenta valor.
    case Peca::Volume: return {Gesto::Muda, 0, 0.0};
    case Peca::Ajuda: return {Gesto::Ajuda, 0, 0.0};
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

RespostaDoArrasto gesto_do_arrasto(Arrasto& arrasto, const Alvo& alvo,
                                   ftxui::Mouse::Button botao,
                                   ftxui::Mouse::Motion movimento,
                                   bool pode) noexcept {
  // Sómente o botão ESQUERDO arrasta: o direito abre o menu, e a roda rola.
  if (botao != ftxui::Mouse::Left) return {};
  const bool na_linha = alvo.peca == Peca::Linha;
  switch (movimento) {
    case ftxui::Mouse::Pressed:
      if (!pode || !na_linha) {
        arrasto = Arrasto();
        return {};
      }
      arrasto = Arrasto{true, alvo.indice, alvo.indice, false};
      return {GestoDoArrasto::Pega, alvo.indice, alvo.indice};
    case ftxui::Mouse::Moved: {
      // A mão anda: o alvo segue o dedo emquanto elle correr a pauta. Sahindo
      // d'ella, o alvo FICA onde estava: assim quem passa por cima da capa e
      // torna não perde a faixa que trazia na mão.
      if (!arrasto.pegou || !na_linha || alvo.indice == arrasto.alvo) return {};
      arrasto.alvo = alvo.indice;
      // ANDOU quer dizer MAIS de uma linha (issue #169). O tremor de uma linha
      // é o que todo duplo clique tem, e tomá-lo por arrasto fazia a faixa
      // mudar de logar em vez de tocar. Quem quer mover uma linha só tem o `K`
      // e o `J`, que já servem e não tremem.
      const std::size_t d_onde = arrasto.origem;
      const std::size_t d_agora = arrasto.alvo;
      const std::size_t quanto =
          d_agora > d_onde ? d_agora - d_onde : d_onde - d_agora;
      arrasto.andou = quanto > 1;
      return {GestoDoArrasto::Arrasta, arrasto.origem, arrasto.alvo};
    }
    case ftxui::Mouse::Released: {
      if (!arrasto.pegou) return {};
      const Arrasto tinha = arrasto;
      arrasto = Arrasto();
      // A linha em que o dedo LARGOU, e não a ultima por onde elle passou
      // (issue #169): passar por cima de uma faixa e tornar á de origem é
      // largar na de origem, e era o contrario que se cumpria. Largando FÓRA
      // da pauta, vale o ultimo alvo conhecido, que é o que a mão trazia.
      const std::size_t onde = na_linha ? alvo.indice : tinha.alvo;
      // Largar onde se pegou não é movimento: é o clique de sempre, e quem o
      // cumpre é a taboada d'elle.
      if (!tinha.andou || onde == tinha.origem)
        return {GestoDoArrasto::Desiste, tinha.origem, tinha.origem};
      return {GestoDoArrasto::Larga, tinha.origem, onde};
    }
  }
  return {};
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO., buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
