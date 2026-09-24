// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO RATO, src/tui/rato.hpp
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

// caixa_por_pintar, a caixa de quem ainda se não pintou. `x_max` menor que
// `x_min` é o vazio que o FTXUI reconhece, e o `Contain` d'elle recusa tudo.
inline ftxui::Box caixa_por_pintar() noexcept { return {0, -1, 0, -1}; }

// As caixas do CABEÇALHO (issue #102). Uma por PEÇA e todas nomeadas, e não um
// vector indexado: as issues irmãs pendem d'esta linha (o letreiro põe imagem
// sobre a caixa do rotulo, o foco desenha a orla sobre a do botão), e indice
// n'um vector é endereço que a primeira peça nova desloca em silencio.
struct CaixasDoCabecalho {
  ftxui::Box aba_mysong = caixa_por_pintar();
  ftxui::Box aba_playlists = caixa_por_pintar();
  ftxui::Box aba_download = caixa_por_pintar();
  ftxui::Box botao_tocar = caixa_por_pintar();
  ftxui::Box botao_anterior = caixa_por_pintar();
  ftxui::Box botao_seguinte = caixa_por_pintar();
  ftxui::Box tempo = caixa_por_pintar();
  ftxui::Box volume = caixa_por_pintar();
  ftxui::Box embaralhar = caixa_por_pintar();
  ftxui::Box repetir = caixa_por_pintar();
  ftxui::Box anima = caixa_por_pintar();
  // O HELP (issue #133), na ponta direita, depois do REPETIR.
  ftxui::Box ajuda = caixa_por_pintar();
  // O MEIO da fita (issue #134): a onda da faixa, que é o trilho de sempre,
  // agora dentro da fita. O clique n'ella busca pela collunha.
  ftxui::Box trilho = caixa_por_pintar();
};

// CaixasDaTela, o que o quadro ANTERIOR deixou escripto. Enchem-se DENTRO de
// quem pinta cada peça, e não na composição da janella: assim quem move os
// paineis não move os cliques, e a assignatura de quem pinta ganha parametro
// de omissão, que é o que deixa as tarefas irmãs entrar sem quebrar nada.
struct CaixasDaTela {
  // As doze do cabeçalho (issue #102). Tomaram o logar do vector dos degraus:
  // a barra lateral já não existe, e com ella se foi o clique no degrau.
  CaixasDoCabecalho cabecalho;
  // Uma por linha VISIVEL da pauta, e sómente por linha que existe: a altura
  // que sobra abaixo da lista não é alvo de cousa alguma.
  std::vector<ftxui::Box> linhas;
  // A linha da vista que está no alto: é ella que faz o indice VISIVEL virar o
  // indice ABSOLUTO da vista do navegador, que é o que a eleição consome.
  std::size_t primeira_linha = 0;
  // A PAUTA INTEIRA (issue #107). Não é a somma das linhas: com a vista vazia
  // não ha linha alguma, e o foco não teria caixa a que voltar. É a caixa que a
  // tabella occupa, pintada ou vazia, e sómente o foco a lê: o dedo continua a
  // achar a LINHA, que é o que elle quer eleger.
  ftxui::Box pauta = caixa_por_pintar();
  ftxui::Box capa = caixa_por_pintar();
  // O BLOCO DA LETRA (issue #161). A sala conta a capa pelo TECTO d'ella, e a
  // capa de 16 por 9 sahe mais baixa: o bloco REAL sobe com ella. É d'esta
  // caixa, e não do rectangulo da sala, que a chapa tira o canto; medida pela
  // sala, a imagem cahia mais abaixo do bloco, e o verso apparecia duas vezes.
  ftxui::Box letra = caixa_por_pintar();
};

// As PEÇAS que o dedo pode achar. `Nada` não é falha: a orla, o rodapé dos
// atalhos e o espectro não respondem ao rato, e hão de dizer que não respondem.
enum class Peca {
  Nada, Aba, Linha, Capa, Anterior, Pausa, Proxima, Progresso,
  Embaralhar, Repetir, Anima,
  // O VOLUME (issue #107). Entrou quando o foco lhe passou a parar em cima: a
  // issue pede que todo botão aceite o clique do teclado, e peça que o teclado
  // aperta e o dedo não seriam duas verdades sobre o mesmo segmento.
  Volume,
  // O HELP (issue #133): o segmento que abre a janella da ajuda. Peça como
  // as outras, para que o Enter com o foco n'elle e o clique sejam um gesto só.
  Ajuda,
};

// Um ALVO: a peça, e o que ella precisa de dizer a mais. O `indice` é a aba no
// cabeçalho e o indice ABSOLUTO da vista na pauta; a `fracao` é sómente do
// trilho do progresso, e vae de zero, na primeira collunha, a um, na ultima.
struct Alvo {
  Peca peca = Peca::Nada;
  std::size_t indice = 0;
  double fracao = 0.0;
};

// alvo_do_ponto, a geometria, e nada mais: que peça está debaixo de (x, y).
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
  FechaCampo,  // com campo de digitar aberto, o clique fecha-o e pára ahi
  VaiParaAba,  // o mesmo caminho das teclas `1` `2` `3`, na aba clicada
  Elege,       // a linha clicada não era a eleita
  Toca,        // clicou-se na JÁ eleita: é o duplo clique, sem cronometro
  Anterior, PausaOuRetoma, Proxima, Busca,
  RodaSobe, RodaDesce,   // na pauta, LINHAS_POR_DENTE de cada vez
  Embaralha, Repete,     // os dous modos, pelo segmento que os mostra
  Anima,                 // trava e destrava a animação da tela
  Muda,                  // o segmento do volume cala a Casa e devolve-a
  AbreMenu,              // o botão direito n'uma linha: o menu de contexto
  Ajuda,                 // o segmento HELP: abre e fecha a janella da ajuda
};

// Tres linhas por dente. Uma seria a roda a arrastar-se; uma tela inteira seria
// perder o logar de vista. Tres é o passo que o dedo já conhece de outras casas.
inline constexpr std::size_t LINHAS_POR_DENTE = 3;

// O ESTADO da tela de que a taboada depende, em cópia de valores: assim a
// bateria arma o caso á mão, sem navegador, sem tocador e sem tela.
// O ARRASTO (issue #153): o que se pegou com o botão em baixo, e onde ella
// cahiria se se largasse agora. Valores, e não punho: a bateria arma-o á mão,
// e o pintor lê-o para dizer ao olho o que vae acontecer.
struct Arrasto {
  bool pegou = false;       // ha faixa na mão
  std::size_t origem = 0;   // o indice ABSOLUTO da vista de onde ella sahiu
  std::size_t alvo = 0;     // o indice ABSOLUTO em que ella cahiria
  bool andou = false;       // já sahiu MAIS de uma linha d'onde veio
};

struct CliqueNaLinha {
  bool premido = false;
  bool moveu = false;
  std::size_t origem = 0;
};
bool confirma_clique(CliqueNaLinha& clique, const Alvo& alvo,
                     ftxui::Mouse::Button botao,
                     ftxui::Mouse::Motion movimento) noexcept;

// O que a máquina do arrasto responde a cada evento do rato.
enum class GestoDoArrasto {
  Nada,      // evento que não é do arrasto
  Pega,      // o botão desceu n'uma linha: ella está na mão
  Arrasta,   // a mão anda: o alvo mudou, e o pintor ha de o dizer
  Larga,     // o botão subiu n'outra linha: ha movimento a cumprir
  Desiste,   // o botão subiu onde não se move nada: a mão esvazia-se
};

struct RespostaDoArrasto {
  GestoDoArrasto gesto = GestoDoArrasto::Nada;
  std::size_t de = 0;    // sómente no Larga
  std::size_t para = 0;  // sómente no Larga
};

// gesto_do_arrasto, a máquina, PURA salvo pelo `arrasto` que ella governa.
// `pode` diz se a vista corrente se deixa arrumar (o acervo e o dentro de uma
// lista sim; artistas, albuns e achados da rede não, que alli a ordem não é do
// operador). O botão que desce fóra de linha alguma não pega; o que sobe na
// MESMA linha desiste, e é assim que o clique simples continua a ser clique.
//
// E o arrasto sómente se dá por ANDADO passada MAIS de uma linha (issue #169):
// o tremor de uma linha é o que todo duplo clique tem, e tomá-lo por arrasto
// fazia a faixa mudar de logar em vez de tocar.
RespostaDoArrasto gesto_do_arrasto(Arrasto& arrasto, const Alvo& alvo,
                                   ftxui::Mouse::Button botao,
                                   ftxui::Mouse::Motion movimento,
                                   bool pode) noexcept;

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

// gesto_do_alvo, a taboada. SÓMENTE `Pressed` conta: o soltar chega sempre, que
// o modo 1000 manda o `m` do SGR, e a mexida não chega, que o 1003 se não liga;
// ignoram-se os dous, e o segundo por não depender de o terminal se comportar.
// O botão DIREITO abre o menu (issue #96), e sómente sobre uma linha da pauta:
// no cabeçalho e na capa não ha faixa alguma de que o menu fosse. O do meio
// continua mudo, que verbo algum d'esta obra o reclama.
GestoDoRato gesto_do_alvo(const Alvo& alvo, ftxui::Mouse::Button botao,
                          ftxui::Mouse::Motion movimento,
                          const EstadoDoRato& estado) noexcept;

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO., buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
