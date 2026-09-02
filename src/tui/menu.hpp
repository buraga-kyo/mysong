// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MENU — src/tui/menu.hpp
// ══════════════════════════════════════════════════════════════════════════
// A machina de FOCO da barra lateral (issue #80): quem manda na tecla, a barra
// ou a lista. Não pinta, não conhece Navegador por dentro, não toca tocador:
// traduz tecla em gesto e guarda o degrau eleito. A ordem do despacho fica
// DECLARADA, para que ninguem a tome por accidente: o prompt trata primeiro
// (issue #79), a barra aberta depois, e a taboada geral do commando por fim.
// Tecla que a barra não conhece FECHA-A e segue á taboada de sempre, e é assim
// que o atalho continua a levar á mesma secção que a barra leva.
//
// DOMÍNIO ......... a tecla que o terminal entrega, e a secção corrente na
//                   hora de abrir.
// CONTRA-DOMÍNIO .. um GestoDaBarra, e o degrau eleito da barra.
// INVARIANTE ...... o degrau eleito está SEMPRE entre zero e o ultimo: as
//                   ordens saturam nos extremos, e ordem alguma o põe fóra. E
//                   o menu não pede cursor de terminal: o unico nó com foco
//                   d'esta obra segue sendo o caret do campo (issue #78).
// Q.E.D. .......... sendo a taboada e a machina funcções puras de tecla e de
//                   estado, a bateria prova o foco inteiro sem erguer terminal.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <ftxui/component/event.hpp>

#include "nucleo/rol.hpp"
#include "tui/navegador.hpp"

namespace mysong::tui {

// Os SETE degraus da barra, na ordem do mockup. A taboada degrau↔secção vive
// aqui e na pintura da tabella; sete é a conta das duas.
inline constexpr std::size_t DEGRAUS_DA_BARRA = 7;

// A taboada. Dentro de uma lista (NoRol) o degrau é o das LISTAS, como na
// pintura de hoje: é lá que se está, um degrau abaixo.
Secao secao_do_degrau(std::size_t degrau) noexcept;
std::size_t degrau_da_secao(Secao secao) noexcept;

// ── A BARRA DA BIBLIOTHECA (issue #93) ──────────────────────────────────────
// Os degraus deixaram de ser sete: são as MINHAS MÚSICAS, uma fileira por lista
// do operador, e os quatro de navegar. Quem determina a conta é ELLE, donde ella
// entra por parametro em vez de morar n'uma constante d'este arquivo.

// O ALVO de um degrau: ou uma SECÇÃO, ou UMA lista pelo seu ID. As duas cousas
// n'um só logar porque a barra é uma só, e taboada apartada obrigaria quem entra
// a perguntar «de qual das duas veio este degrau?» em todo ramo. E é o ID, e não
// o indice na barra: apagada uma lista entre abrir a barra e teclar Enter, o
// indice desloca-se e entrar-se-hia na lista errada; o id não se desloca.
struct AlvoDaBarra {
  Secao secao = Secao::Busca;
  int rol = 0;  // sómente quando a secção é NoRol; zero nas demais
};

std::size_t degraus_da_barra(std::size_t listas) noexcept;

// A taboada degrau→alvo, na ordem da barra: é a MESMA ordem da pintura, e mudar
// uma sem a outra faria o Enter abrir cousa que o marcador não diz. Degrau fóra
// da conta cahe no primeiro, e não estoura.
AlvoDaBarra alvo_do_degrau(std::size_t degrau,
                           const std::vector<nucleo::Rol>& listas);

// O caminho de volta, para a barra acordar onde se ESTÁ. A secção que a barra
// não mostra acorda no alto: as LISTAS sahiram d'ella (abrem-se pelo `P`), e as
// FAIXAS de um album acendem ÁLBUNS, que é o degrau de que se veio. Dentro de
// uma lista o degrau é o d'ELLA, achado pelo id.
std::size_t degrau_da_secao(Secao secao, const std::vector<nucleo::Rol>& listas,
                            int rol_corrente);

// O ROTULO de um degrau: em caixa alta e em portuguez, salvo o da lista, que vae
// como o operador o escreveu, que nome dado por elle é dado d'elle e não rotulo
// d'esta Casa. Mora aqui, ao lado da taboada, e não na pintura: rotulo n'um
// arquivo e alvo n'outro é o que faz o Enter abrir cousa que o marcador não diz.
std::string rotulo_do_degrau(std::size_t degrau,
                             const std::vector<nucleo::Rol>& listas);

// tecla_abre_menu — o Tab, e o Shift+Tab com elle: havendo sómente dous focos,
// avançar e voltar são o mesmo gesto, e tecla morta não se dá a quem explora.
bool tecla_abre_menu(const ftxui::Event& tecla) noexcept;

// Os GESTOS da barra aberta. O Alheio é toda tecla que a barra não conhece:
// quem o recebe FECHA a barra e deixa a tecla seguir á taboada de sempre. É
// assim que o espaço pausa, o «q» sahe, e o atalho de secção leva ao mesmo
// logar a que a barra levaria, sem taboada paralela de trinta verbos.
enum class GestoDaBarra { Alheio, Fecha, Sobe, Desce, AoPrincipio, AoFim,
                          Entra };

// gesto_da_barra — a taboada da barra aberta, espelho da navegação da lista:
// seta e vogal do vi andam, Enter e seta direita entram, e o que na lista
// VOLTA (seta esquerda, Escape, Backspace) aqui fecha sem trocar secção.
GestoDaBarra gesto_da_barra(const ftxui::Event& tecla) noexcept;

// A MACHINA. Dous campos e nada mais: se o menu está aberto, e qual degrau
// está eleito. Não conhece o Navegador por dentro: quem abre DIZ-LHE a secção
// corrente, e quem entra pergunta-lhe o alvo e vae elle proprio ao navegador.
// Fechar não esquece o degrau de proposito: o estado que fica é inerte, e
// abrir torna a assentá-lo na secção corrente.
class Menu {
 public:
  bool aberto() const noexcept;
  std::size_t degrau() const noexcept;

  // abre — o Tab vindo da lista, com as listas do operador na mão. O degrau
  // eleito nasce na secção CORRENTE, que é o que o esboço aprovado mostra: o
  // marcador acorda onde se está. E o ALVO já não é d'esta classe: quem entra
  // pergunta-o á taboada, que é quem sabe qual degrau é lista e qual é secção.
  //
  // A CONTA dos
  // degraus guarda-se, e as ordens saturam por ella. Guardá-la não envelhece:
  // com a barra aberta, toda tecla que cria, renomeia ou apaga uma lista é
  // alheia á barra, e o ramo Alheio fecha-a antes de a tecla correr.
  void abre(Secao corrente, const std::vector<nucleo::Rol>& listas,
            int rol_corrente);
  void fecha() noexcept;

  void sobe() noexcept;  // saturam nos extremos: menu não é carrossel
  void desce() noexcept;
  void ao_principio() noexcept;
  void ao_fim() noexcept;

 private:
  bool aberto_ = false;
  std::size_t degrau_ = 0;
  std::size_t degraus_ = DEGRAUS_DA_BARRA;
};

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
