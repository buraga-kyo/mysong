// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SALA — src/tui/sala.hpp
// ══════════════════════════════════════════════════════════════════════════
// A sala em tres paineis: a bibliotheca á esquerda, a colleção aberta no meio,
// e o TOCANDO AGORA á direita. Aqui moram as peças do meio e da direita, e os
// numeros da composição. Funcções PURAS de VALORES: nem Tocador, nem janella.
//
// DOMÍNIO ......... retractos (a colleção á vista, a ficha do que toca) e a
//                   geometria em collunhas e linhas.
// CONTRA-DOMÍNIO .. cadeias e `ftxui::Element`.
// INVARIANTE ...... numero algum da composição mora no pintor: sahem todos
//                   d'uma funcção só, que a bateria interroga sem terminal.
// Q.E.D. .......... sendo a sala funcção do estado, redimensionar e voltar dá
//                   a mesma sala, e a prova afere-a cella a cella em papel.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>

#include "nucleo/capa.hpp"
#include "nucleo/fila.hpp"
#include "tui/navegador.hpp"

namespace mysong::tui {

// O que se conta na linha da conta. A ESPECIE, e não a secção: MINHAS MÚSICAS,
// um album e uma lista contam a MESMA cousa, e tres nomes dariam tres erros.
enum class Especie { Faixas, Artistas, Albuns, Listas, Achados };

// A COLLECÇÃO Á VISTA: o que o cabeçalho do meio diz d'ella. Cópia de valores,
// e nunca punho: perguntar duas vezes no mesmo quadro dá tela a contradizer-se.
struct Colleccao {
  std::string nome;
  std::size_t quantas = 0;
  Especie especie = Especie::Faixas;
  int duracao = 0;  // somma das linhas á vista, em segundos; zero não se diz
  bool embaralhado = false;
  nucleo::Repeticao repeticao = nucleo::Repeticao::Nenhuma;
};

// texto_da_duracao — a somma POR EXTENSO: `1h23`, `23min`, `45s`, e vazia no
// que não é positivo. `MM:SS` ao lado de «4 FAIXAS» lê-se como o tempo D'ELLA.
std::string texto_da_duracao(int segundos);

// texto_da_conta — «4 FAIXAS, 14min». Um sahe no singular e sem o `s`.
std::string texto_da_conta(std::size_t quantas, Especie especie, int duracao);

// especie_da_secao — o que se conta em cada secção da barra lateral.
Especie especie_da_secao(Secao secao);

// nome_da_colleccao — o titulo do cabeçalho. O vocabulario FIXO vae em caixa
// alta; o nome vindo do acervo (artista, album, lista, catalogo) sahe VERBATIM,
// que caixa alta byte a byte estragaria o UTF-8 acentuado do portuguez
// («Canção» sahiria «CANçãO»), e taboa de caixa Unicode esta Casa não carrega.
std::string nome_da_colleccao(Secao secao,
                              const std::vector<std::string>& trilha,
                              const std::string& nome_do_catalogo);

// A GEOMETRIA da sala: quanto toma cada painel, dada a largura UTIL (a que o
// pintor tem depois da orla) e a altura da faixa do corpo. As collunhas da
// barra entram por PARAMETRO: quem sabe a largura d'ella é quem a pinta, e
// essa lavra é de outra tarefa d'esta mesma onda.
struct Geometria {
  std::size_t meio = 0;       // collunhas do meio: cabeçalho e tabella
  std::size_t painel = 0;     // collunhas do painel da direita; zero esconde-o
  std::size_t capa = 0;       // TECTO de linhas que se pede á Galeria
  std::size_t livre = 0;      // linhas do painel abaixo do titulo e da ficha
  std::size_t cabecalho = 0;  // linhas do cabeçalho do meio, separador incluso
  std::size_t tabella = 0;    // linhas que sobram para a tabella
};

// geometria_da_sala — todos os numeros da composição, n'uma conta só.
Geometria geometria_da_sala(std::size_t largura, std::size_t altura,
                            std::size_t collunhas_da_barra);

// A FICHA da faixa que toca. Valores, e não punho para o tocador nem para o
// indice: assim a bateria arma-a á mão, sem motor e sem banco.
struct Ficha {
  std::string titulo;
  std::string artista;
  std::string album;
};

// ficha_da_faixa — a ficha do caminho, com as etiquetas do indice quando as ha.
// Sem etiqueta, o nome do arquivo serve de titulo: buraco na ficha faria o
// painel dizer que nada toca na hora em que alguma cousa toca.
Ficha ficha_da_faixa(const std::string& caminho, const std::string& titulo,
                     const std::string& artista, const std::string& album);

// elemento_da_ficha — TRES linhas, sempre as tres: ficha que encolhe faria o
// espectro subir e descer a cada troca de faixa.
ftxui::Element elemento_da_ficha(const Ficha& ficha, std::size_t largura);

// A ARTE: a capa já pintada, cingida ao TECTO que se pediu á Galeria.
// `linhas_da_arte` diz quantas linhas ella toma de facto (as do chafa quando
// achada, as do marcador quando não), para que quem compõe saiba o que sobra.
//
// O desconto da orla mora AQUI, e não no `elemento_da_capa`: aquelle põe
// `border` POR FÓRA do marcador, duas linhas e duas collunhas a mais do que se
// lhe pede, e é lavra que a tarefa irmã do rato tambem edita.
std::size_t linhas_da_arte(const nucleo::CapaPintada& capa, std::size_t tecto);
ftxui::Element elemento_da_arte(const nucleo::CapaPintada& capa,
                                std::size_t largura, std::size_t linhas);

// elemento_do_cabecalho — a colleção á vista por cima da tabella: capa pequena,
// nome, conta com os chips, separador. SEIS linhas, as que a geometria reservou.
ftxui::Element elemento_do_cabecalho(const Colleccao& colleccao,
                                     const nucleo::CapaPintada& capa,
                                     std::size_t largura);

// elemento_do_painel — o TOCANDO AGORA: o titulo, a arte, a ficha logo abaixo
// d'ella SEM VÃO, e o que o chamador quizer por baixo (o espectro, ou a letra
// quando o `l` a pede). O de baixo entra já composto, para que esta peça não
// conheça nem um nem outra e a bateria lh'os arme á mão.
ftxui::Element elemento_do_painel(const Ficha& ficha, ftxui::Element arte,
                                  ftxui::Element baixo, std::size_t largura);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
