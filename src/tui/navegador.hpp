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

#include "nucleo/aquisicao.hpp"
#include "nucleo/biblioteca.hpp"
#include "nucleo/catalogo.hpp"
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
enum class Secao { Artistas, Albuns, Faixas, Busca, Rede, Rois, NoRol, Lista };

// Uma LINHA do que está á vista. O texto é o que se mostra; a `chave` é o que a
// ordem `entra` consome, e nem sempre são a mesma cousa: o album mostra-se pelo
// nome e entra-se n'elle pelo par artista e album.
struct Linha {
  std::string texto;
  std::string chave;
  int numero = 0;      // sómente em faixas; zero é «sem numero»
  int duracao = 0;     // sómente em faixas, em segundos
  std::string autor;   // sómente em faixas: o artista, para a columna do meio
  // O INDICE do achado de que a linha veio (Rede; menos um nas demais). Nunca se
  // exibe: é por elle que o achado eleito se acha com o filtro posto (issue #56).
  int origem = -1;
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

  // mostra_rede — põe na tela os ACHADOS que vieram de FÓRA da bibliotheca, e
  // passa á secção Rede: o texto é a faixa canonica quando a fonte a deu, senão o
  // titulo; o autor é o artista, senão o canal (issue #56). A lista guarda-se
  // inteira, é ella a fonte da vista n'esta secção, e o `origem` de cada linha
  // liga-a ao seu achado.
  void mostra_rede(std::vector<nucleo::Achado> achados);

  // ha_achado / achado_eleito — o achado da linha eleita, e SÓMENTE na Rede. O
  // laço é o `origem` da linha, e não o indice da vista: com filtro posto os dous
  // desencontram-se, e encommendar-se-hia o achado errado. E URL não é caminho:
  // confundi-los poria endereço de rede na fila do motor.
  bool ha_achado() const;
  nucleo::Achado achado_eleito() const;

  // ── O CATALOGO DO SPOTIFY (issue #13) ─────────────────────────────────────

  // mostra_catalogo — põe na tela o catalogo que se leu, ANTES de se baixar cousa
  // alguma: é o que a tarefa pede quando manda devolver a lista para se conferir.
  void mostra_catalogo(nucleo::Catalogo catalogo);

  // O nome da lista lida, que é o que vae por album nas etiquetas. Vazio fóra d'esta
  // secção.
  const std::string& nome_do_catalogo() const noexcept;

  // A faixa eleita do catalogo, e TODAS ellas. A eleita acha-se pelo indice que a
  // linha carrega, e não pelo indice da vista: com filtro posto os dous
  // desencontram-se, e baixar-se-hia a faixa errada.
  bool ha_faixa_de_catalogo() const;
  nucleo::FaixaDoCatalogo faixa_de_catalogo_eleita() const;
  const std::vector<nucleo::FaixaDoCatalogo>& faixas_do_catalogo() const noexcept;

  // ── AS LISTAS (issue #10) ─────────────────────────────────────────────────

  // mostra_rois — passa á secção da lista das listas, relendo-a do banco.
  void mostra_rois();

  // O ALVO: a ultima lista em que se entrou. Sobrevive a sahir d'ella, e é isso que
  // faz `a` poder juntar do ACERVO: para juntar uma faixa é preciso estar onde a
  // faixa está, e a faixa não está dentro da lista. Zero antes de se entrar na
  // primeira, e zero outra vez quando a lista alvo se apaga.
  int rol_corrente() const noexcept;
  const std::string& nome_corrente() const noexcept;

  // As sete operações. Todas relêem a vista depois de mutar, e todas devolvem
  // falso quando não ha roleiro, quando não ha lista eleita, ou quando a camada de
  // baixo recusou. A tela não ha de adivinhar qual dos tres foi.
  bool cria_rol(const std::string& nome);
  bool renomeia_rol(const std::string& nome);
  bool apaga_rol();
  bool junta_ao_rol(const std::string& caminho);
  bool retira_do_rol();
  bool sobe_no_rol();
  bool desce_no_rol();

  // O nome da lista eleita, para a tela poder perguntar «apagar «tal»?». Vazio
  // fóra da secção das listas.
  std::string nome_do_rol_eleito() const;

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
  std::vector<nucleo::Achado> achados_;  // os achados de que as linhas vieram
  nucleo::Catalogo catalogo_;  // a fonte da vista na secção Lista
  std::vector<std::string> trilha_;
  std::string termo_;
  std::size_t eleito_ = 0;
};

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
