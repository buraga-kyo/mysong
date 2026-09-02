// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO RATO — src/tui/rato.hpp
// ══════════════════════════════════════════════════════════════════════════
// A tradução de COORDENADA em alvo, e de alvo em GESTO (issue #95). Vive á
// parte da janella pela razão do commando: a janella abre terminal, motor e som
// e não se prova; isto é funcção de valores, e a bateria afere-a em papel.
//
// DOMÍNIO ......... as caixas que o `reflect` do FTXUI encheu no ultimo quadro,
//                   o ponto onde o botão desceu, o botão, o movimento, e o
//                   estado da tela (digitando, eleito, quantas, duração).
// CONTRA-DOMÍNIO .. um Alvo (que peça, que indice, que fracção) e um Gesto.
// INVARIANTE ...... caixa por pintar não casa com ponto algum. O `ftxui::Box`
//                   nasce {0,0,0,0}, e essa caixa CONTÉM o ponto (0,0): quem
//                   nascesse assim casaria com o clique no canto antes do
//                   primeiro quadro. Nasce-se pois VAZIO, e é estructural.
// Q.E.D. .......... sendo as taboadas puras, o rato prova-se em papel.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <vector>

#include <ftxui/component/mouse.hpp>
#include <ftxui/screen/box.hpp>

namespace mysong::tui {

// caixa_por_pintar — a caixa de quem ainda se não pintou. `x_max` menor que
// `x_min` é o vazio que o FTXUI reconhece, e o `Contain` d'elle recusa tudo.
inline ftxui::Box caixa_por_pintar() noexcept { return {0, -1, 0, -1}; }

// As caixas do TRANSPORTE. As metades da barra de progresso guardam-se á parte
// porque o enchimento se pinta em DOUS elementos, o cheio e o vazio: no
// principio e no fim um d'elles tem largura zero e a caixa d'esse sahe vazia.
struct CaixasDoTransporte {
  ftxui::Box anterior = caixa_por_pintar();
  ftxui::Box pausa = caixa_por_pintar();
  ftxui::Box proxima = caixa_por_pintar();
  ftxui::Box barra_cheia = caixa_por_pintar();
  ftxui::Box barra_vazia = caixa_por_pintar();

  // progresso — a barra inteira, união das metades. Não se guarda em campo:
  // campo seria estado em duplicata, e o `reflect` só enche as metades.
  ftxui::Box progresso() const noexcept;
};

// CaixasDaTela — o que o quadro ANTERIOR deixou escripto. Enchem-se DENTRO de
// quem pinta cada peça, e não na composição da janella: assim quem move os
// paineis não move os cliques, e a assignatura de quem pinta ganha parametro
// de omissão, que é o que deixa as tarefas irmãs entrar sem quebrar nada.
struct CaixasDaTela {
  // Uma por degrau da barra lateral, na ordem em que ella os pinta.
  std::vector<ftxui::Box> degraus;
  // Uma por linha VISIVEL da tabella, e sómente por linha que existe: a altura
  // que sobra abaixo da lista não é alvo de cousa alguma.
  std::vector<ftxui::Box> linhas;
  // A linha da vista que está no alto: é ella que faz o indice VISIVEL virar o
  // indice ABSOLUTO da vista do navegador, que é o que a eleição consome.
  std::size_t primeira_linha = 0;
  ftxui::Box capa = caixa_por_pintar();
  CaixasDoTransporte transporte;
};

// As PEÇAS que o dedo pode achar. `Nada` não é falha: a orla, o rodapé dos
// atalhos e o espectro não respondem ao rato, e hão de dizer que não respondem.
enum class Peca {
  Nada, Degrau, Linha, Capa, Anterior, Pausa, Proxima, Progresso,
};

// Um ALVO: a peça, e o que ella precisa de dizer a mais. O `indice` é o degrau
// na barra e o indice ABSOLUTO da vista na tabella; a `fracao` é sómente da
// barra de progresso, e vae de zero, na primeira collunha, a um, na ultima.
struct Alvo {
  Peca peca = Peca::Nada;
  std::size_t indice = 0;
  double fracao = 0.0;
};

// alvo_do_ponto — a geometria, e nada mais: que peça está debaixo de (x, y).
// O ponto é o que o `Event::Mouse` entrega, e elle JÁ chega na conta do `Box`:
// medido no FTXUI v7.0.3, o parser guarda o argumento cru do SGR, que conta de
// um, e o laço da tela tira-lhe o `cursor_x_`, que vale um em tela cheia. Nada
// se soma nem se tira aqui, e quem o fizesse erraria por uma collunha.
Alvo alvo_do_ponto(const CaixasDaTela& caixas, int x, int y) noexcept;

// Os GESTOS que o rato pede. Não são verbos do tocador: `Toca`, `Anterior`,
// `Proxima`, `Busca` e `PausaOuRetoma` viram Ordem na janella, que é quem tem o
// tocador na mão; os demais governam o navegador e o menu.
enum class Gesto {
  Nada,
  FechaCampo,     // com campo de digitar aberto, o clique fecha-o e pára ahi
  EntraNoDegrau,  // o mesmo caminho do Enter na barra, no degrau clicado
  Elege,          // a linha clicada não era a eleita
  Toca,           // clicou-se na JÁ eleita: é o duplo clique, sem cronometro
  Anterior, PausaOuRetoma, Proxima, Busca,
  RodaSobe, RodaDesce,      // na tabella, LINHAS_POR_DENTE de cada vez
  DegrauSobe, DegrauDesce,  // sobre a barra, um degrau de cada vez
};

// Tres linhas por dente. Uma seria a roda a arrastar-se; uma tela inteira seria
// perder o logar de vista. Tres é o passo que o dedo já conhece de outras casas.
inline constexpr std::size_t LINHAS_POR_DENTE = 3;

// O ESTADO da tela de que a taboada depende, em cópia de valores: assim a
// bateria arma o caso á mão, sem navegador, sem tocador e sem tela.
struct EstadoDoRato {
  bool digitando = false;   // ha campo de digitar aberto
  std::size_t eleito = 0;   // a linha eleita, em indice absoluto da vista
  std::size_t quantas = 0;  // quantas linhas tem a vista
  double duracao = 0.0;     // a da faixa, que é o que a busca multiplica
};

// O gesto e o que elle carrega: o `indice` é o degrau ou a linha, e o `alvo` é
// a posição em SEGUNDOS da busca. Zero nos demais, de proposito: gesto que não
// tem alvo não ha de carregar numero que alguem possa vir a ler.
struct GestoDoRato {
  Gesto gesto = Gesto::Nada;
  std::size_t indice = 0;
  double alvo = 0.0;
};

// gesto_do_alvo — a taboada. SÓMENTE `Pressed` conta: o soltar chega sempre, que
// o modo 1000 manda o `m` do SGR, e a mexida não chega, que o 1003 se não liga;
// ignoram-se os dous, e o segundo por não depender de o terminal se comportar.
// Botão direito e do meio não fazem nada: o direito é o menu de contexto da
// issue #96, e prometter aqui seria prometter a mesma cousa duas vezes.
GestoDoRato gesto_do_alvo(const Alvo& alvo, ftxui::Mouse::Button botao,
                          ftxui::Mouse::Motion movimento,
                          const EstadoDoRato& estado) noexcept;

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
