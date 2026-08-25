// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO NAVEGADOR — src/tui/navegador.hpp
// ══════════════════════════════════════════════════════════════════════════
// A máquina de estados da navegação: em que secção se está, o que está á vista,
// qual a linha eleita, e o que acontece a cada ordem. Não pinta, não conhece
// FTXUI, não conhece Tocador. Quem pinta é a janella; quem toca é o nucleo.
//
// DOMÍNIO ......... uma Bibliotheca aberta (emprestada), e as ordens do
//                   operador: sobe, desce, entra, volta, filtra.
// CONTRA-DOMÍNIO .. a lista do que está á vista, o indice do eleito, e o
//                   caminho da faixa quando o operador manda tocar.
// INVARIANTE ...... o eleito está SEMPRE dentro da lista, ou a lista é vazia e
//                   elle é zero. Não ha ordem que o ponha fóra, e por isso quem
//                   pinta nunca ha de conferir o limite.
// Q.E.D. .......... sendo a navegação estado puro sobre consultas de leitura, a
//                   bateria percorre o caminho inteiro (Artistas, um artista, um
//                   album, uma faixa) sem erguer terminal e sem tocar som.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "nucleo/biblioteca.hpp"
#include "nucleo/rol.hpp"

namespace mysong::tui {

// As secções da barra lateral. A ordem é a do mockup, e é ella que a barra
// mostra de alto a baixo.
// A secção REDE (issue #12) é a unica cujas linhas NÃO vêm da bibliotheca: ellas
// vêm de fóra, por mostra_rede. Entra no mesmo enum porque a barra lateral é uma, e
// duas listas de secções fariam a barra depender de qual d'ellas se lesse primeiro.
// As LISTAS entram como DUAS secções, e não uma: a lista das listas e o dentro de
// uma lista são vistas differentes, com ordens differentes e teclas differentes.
// Uma só obrigaria a perguntar «estou dentro ou fóra?» em todo ramo.
enum class Secao { Artistas, Albuns, Faixas, Busca, Rede, Rois, NoRol };

// Uma LINHA do que está á vista. O texto é o que se mostra; a `chave` é o que a
// ordem `entra` consome, e nem sempre são a mesma cousa: o album mostra-se pelo
// nome e entra-se n'elle pelo par artista e album.
struct Linha {
  std::string texto;
  std::string chave;
  int numero = 0;      // sómente em faixas; zero é «sem numero»
  int duracao = 0;     // sómente em faixas, em segundos
  std::string autor;   // sómente em faixas: o artista, para a columna do meio
};

// A JANELLA da rolagem: qual a primeira linha a mostrar, dada a altura da
// tabella. Funcção PURA de (eleito, quantas, altura), e não estado guardado no
// navegador: guardada, ella divergiria da vista depois de um filtro, e a tela
// mostraria uma fatia que já não contem o eleito.
//
// A regra é MINIMA: rola-se o menos que baste para o eleito caber. Assim descer
// uma linha não salta meia tela, e voltar ao logar de antes devolve a mesma
// fatia, que é o que o aceite pede quando diz que a rolagem não perde a posição.
std::size_t primeira_a_mostrar(std::size_t eleito, std::size_t quantas,
                               std::size_t altura, std::size_t primeira_de_antes);

// O NAVEGADOR. A bibliotheca é EMPRESTADA e vive mais que elle: o navegador não a
// possue, e por isso não a fecha nem a reabre ás escondidas.
class Navegador {
 public:
  // O ROLEIRO entra por punho que pode ser NULLO, e não por referencia: a bateria da
  // navegação do acervo não ha de ser obrigada a erguer banco de listas para provar
  // que descer de artista para album funcciona. Nullo quer dizer «esta corrida não
  // tem listas», e as secções d'ellas ficam vazias em vez de estourarem.
  explicit Navegador(const nucleo::Biblioteca& livraria,
                     nucleo::Roleiro* roleiro = nullptr);

  Navegador(const Navegador&) = delete;
  Navegador& operator=(const Navegador&) = delete;

  Secao secao() const noexcept;
  const std::vector<Linha>& vista() const noexcept;
  std::size_t eleito() const noexcept;

  // A trilha do que se atravessou, para o titulo da tabella: vazia em Artistas,
  // com o artista em Albuns, com artista e album em Faixas.
  const std::vector<std::string>& trilha() const noexcept;

  void desce() noexcept;
  void sobe() noexcept;
  void ao_principio() noexcept;
  void ao_fim() noexcept;

  // entra — desce um degrau. VERDADEIRO quando o degrau era uma FAIXA, que é o
  // signal para quem chama mandar tocar; nesse caso `caminho_eleito` diz qual.
  bool entra();

  // volta — sobe um degrau. Falso quando já se está no alto, e ahi nada muda.
  bool volta();

  // mostra_rede — põe na tela uma lista que veio de FÓRA da bibliotheca, e passa á
  // secção Rede. A lista guarda-se, e é ella a fonte da vista enquanto se estiver
  // n'esta secção: o filtro applica-se sobre ella, como nas outras.
  void mostra_rede(std::vector<Linha> achados);

  // url_eleita — a URL da linha eleita, e SÓMENTE estando-se na Rede. Existe á parte
  // de caminho_eleito porque as duas cousas não se podem confundir: uma é caminho no
  // disco, e a outra é endereço na rede. Confundi-las poria uma URL na fila do motor.
  std::string url_eleita() const;

  // ── AS LISTAS (issue #10) ─────────────────────────────────────────────────

  // mostra_rois — passa á secção da lista das listas, relendo-a do banco.
  void mostra_rois();

  // O filtro. Cadeia vazia limpa-o. Filtra o que está Á VISTA, e não o acervo:
  // é o que o mockup mostra, e é o que o operador espera de uma barra de busca
  // que vive por cima de uma lista.
  void filtra(std::string termo);
  const std::string& termo() const noexcept;

  std::string caminho_eleito() const;

  // Relê o acervo da bibliotheca, conservando a secção e a trilha se ainda
  // existirem. Chama-se depois de a varredura concluir.
  void recarrega();

 private:
  void refaz_vista();

  // id_do_eleito — o id da lista eleita na secção Rois, e zero não havendo.
  int id_do_eleito() const;

  const nucleo::Biblioteca& livraria_;
  nucleo::Roleiro* roleiro_ = nullptr;
  int rol_corrente_ = 0;
  std::string nome_corrente_;
  Secao secao_ = Secao::Artistas;
  std::vector<Linha> vista_;
  std::vector<Linha> rede_;  // a fonte da vista na secção Rede, e sómente n'ella
  std::vector<std::string> trilha_;
  std::string termo_;
  std::size_t eleito_ = 0;
};

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
