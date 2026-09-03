// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA TABELLA — src/tui/tabella.hpp
// ══════════════════════════════════════════════════════════════════════════
// Pinta o que o Navegador diz: a pauta com a fatia que a rolagem elegeu, a
// letra e a capa. Não decide nada; a decisão toda vive no navegador, e é lá
// que se prova.
//
// DOMÍNIO ......... o Navegador (por leitura), e a largura e a altura em
//                   collunhas e linhas.
// CONTRA-DOMÍNIO .. `ftxui::Element`.
// INVARIANTE ...... não chama ordem alguma do navegador: pinta e sahe. Uma
//                   funcção que pintasse E andasse na lista faria o quadro
//                   depender de quantas vezes se pintou.
// Q.E.D. .......... sendo a pintura funcção do estado, redesenhar não muda cousa
//                   alguma, que é o que um laço a vinte quadros por segundo pede.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "nucleo/capa.hpp"
#include "nucleo/letra.hpp"
#include "tui/navegador.hpp"

namespace mysong::tui {

// apara_collunhas — a cadeia em EXACTAMENTE `collunhas` cellas do terminal: o
// que sobeja corta-se e a ultima cella leva «…», e o que falta enche-se de
// espaço. Conta-se por CELLA, e não por codepoint: o glypho CJK toma duas, e a
// conta por codepoint dava-lhe uma, d'onde a columna do titulo transbordava e
// empurrava as de baixo. Foi o debito que a issue #93 deixou escripto.
//
// Cabendo a cadeia inteira, «…» algum se põe: a reticencia é signal de que se
// cortou, e pô-la sem corte seria mentir ao olho.
std::string apara_collunhas(const std::string& crua, std::size_t collunhas);

// As MEDIDAS de uma linha da pauta, em cellas. Sahem d'uma conta só, e não do
// pintor, para que a bateria as interrogue sem terminal: é a mesma regra da
// sala. Zero quer dizer «esta columna não se abre n'esta largura».
struct Medidas {
  std::size_t marcador = 0;  // a cella do «▶» da que sôa
  std::size_t numero = 0;    // o № á direita, tres cellas
  std::size_t titulo = 0;    // o titulo, ou o nome; toma o que sobra
  std::size_t artista = 0;   // um terço do que sobra ao titulo
  std::size_t regua = 0;     // a régua da duração, seis cellas
  std::size_t conta = 0;     // MM:SS na faixa, a conta de faixas no nome
  bool pela_conta = false;   // a vista que conta nomes: artistas, albuns, listas
};

// medidas_da_pauta — as columnas na largura que ha. As de largura fixa cedem
// por ORDEM DE SERVIÇO, da menos util para a mais: primeiro o artista, depois a
// régua, depois o tempo, e por fim o №. O titulo fica até ao fim, que sem elle
// a linha não diz cousa alguma.
//
// `ha_autor` decide-se pela FATIA á vista, e não por linha: por linha, as
// columnas de baixo desalinhavam das de cima e a pauta parecia quebrada.
Medidas medidas_da_pauta(std::size_t largura, bool ha_autor, bool pela_conta);

// cheias_da_regua — quantas cellas da régua se pintam cheias, dado o que a
// linha mede e o que mede a MAIOR da fatia á vista. É textura de HUD: o olho
// compara as linhas entre si, e não com relogio algum.
//
// Arredonda ao MAIS PROXIMO, e nunca a zero: faixa que existe ha de mostrar ao
// menos uma cella, que régua toda vazia lê-se como faixa sem duração. Sem
// medida (zero ou negativo), porém, ella sahe vazia de proposito.
std::size_t cheias_da_regua(int quanto, int maior, std::size_t cellas);

// Um PEDAÇO da linha da pauta: o texto já aparado á cella, e a tinta com que se
// pinta. A linha sahe em pedaços, e não n'uma cadeia só, porque as columnas não
// levam a mesma tinta: o titulo da que sôa accende, e o resto da linha não.
//
// Os vãos e as margens tambem são pedaços. Assim a somma das larguras É a
// largura da pauta, e a bateria cobra-a sem écran algum; e o bloco da eleita,
// que veste a linha de orla a orla, não tem vão por onde o fundo escape.
struct Pedaco {
  std::string texto;
  std::string_view tinta;
  bool negrito = false;
};

// pedacos_da_linha — a linha inteira, columna a columna, na ordem em que se
// pinta. `maior` é o que mede a maior da fatia á vista, que é o que dá a régua;
// `soa` diz se é esta a faixa que o motor toca, que lhe põe o «▶» e lhe accende
// o titulo. A ELEITA não entra aqui: ella é tinta, e não texto.
std::vector<Pedaco> pedacos_da_linha(const Linha& linha, const Medidas& medidas,
                                     int maior, bool soa);

// elemento_da_linha — os pedaços vestidos de tinta. A ELEITA vira BLOCO: fundo
// de orla a orla, e TODO o texto n'uma tinta só. É o gesto do sitio do Plano
// Artistico, onde o cursor sobre a palavra do menu a engole n'um bloco solido;
// aqui o bloco é violeta, que é a palheta d'esta Casa.
//
// Eleita que TAMBEM sôa troca o violeta pelo glow_core: dous signaes na mesma
// linha hão de dar um bloco só, e não dous fundos a brigar pela mesma cella.
ftxui::Element elemento_da_linha(const std::vector<Pedaco>& pedacos, bool eleita,
                                 bool soa, std::size_t largura);

// A tabella do meio, com a fatia que cabe em `altura` linhas. `primeira` é o que
// `primeira_a_mostrar` devolveu, e entra por parâmetro para que a pintura não
// guarde estado de rolagem que pudesse divergir da vista.
//
// `tocando` é o CAMINHO da faixa que o motor toca (issue #92). A linha cuja
// chave casar com elle accende em glow_core, com «▶» no logar do numero: são
// DOUS signaes, que a eleita (o v900) diz onde o dedo está e este diz o que
// sôa. Cadeia vazia, que é o padrão, pinta a tabella de sempre byte por byte,
// e a prova que já existe o afere.
//
// As CAIXAS (issue #95) são UMA por linha que se PINTOU, e nunca por linha que
// se não pintou: a altura que sobra abaixo da lista não é alvo de clique algum,
// e vista vazia limpa o vector. Quem lê o vector somma-lhe a `primeira` para ir
// da linha visivel á linha da vista.
ftxui::Element elemento_da_tabella(const Navegador& navegador,
                                   std::size_t primeira, std::size_t altura,
                                   std::size_t largura,
                                   const std::string& tocando = {},
                                   std::vector<ftxui::Box>* caixas = nullptr);

// A LETRA no painel (issue #15). Mostra a linha corrente em destaque, com as
// vizinhas apagadas em volta: `altura` linhas ao todo, e a corrente no meio d'ellas.
// `corrente` menos um quer dizer «antes do primeiro verso», e ahi mostram-se as
// primeiras linhas apagadas, para que o operador veja que ha letra a chegar.
ftxui::Element elemento_da_letra(const std::vector<nucleo::LinhaDaLetra>& linhas,
                                 int corrente, std::size_t altura,
                                 std::size_t largura);

// A CAPA no painel (issue #16). Achada, pinta-se linha a linha, com os escapes que o
// chafa produziu passados intactos. Não achada, desenha-se o MARCADOR com os tokens
// d'esta Casa: um buraco não diz nada, e o marcador diz «este album não tem capa».
//
// A CAIXA (issue #95) é UMA, a do quadro inteiro: o clique n'ella pausa e
// retoma. Sem capa que caiba, ella sahe VAZIA, e não a do quadro anterior.
ftxui::Element elemento_da_capa(const nucleo::CapaPintada& capa,
                                std::size_t collunas, std::size_t linhas,
                                ftxui::Box* caixa = nullptr);

// caret_do_campo — a cella de UMA collunha onde o cursor do terminal pousa
// emquanto ha prompt aberto. É o UNICO logar d'esta obra que pede foco, e é de
// proposito: o `Render` do FTXUI elege UM nó focado por quadro e cala os outros
// sem aviso, donde dous pedidos seriam um pedido a perder-se em silencio.
//
// Barra QUIETA, e não a piscar: a queixa que abriu a issue #78 foi «o meu cursor
// fica piscando», e dar-lhe um caret que pisca seria responder á queixa com a
// queixa. O FTXUI usa o foco tambem para rolar dentro de um `frame`; esta obra
// não tem `frame` algum, e quem puser um ha de saber que herda esta linha.
ftxui::Element caret_do_campo();

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
