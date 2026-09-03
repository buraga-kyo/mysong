// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO MENU DE CONTEXTO — src/tui/menu_contexto.hpp
// ══════════════════════════════════════════════════════════════════════════
// O menu que o botão direito e a tecla `m` abrem sobre uma faixa (issue #96).
// Vive á parte da janella pela razão do rato: a janella abre terminal, motor e
// som e não se prova; isto é estado e taboada, e a bateria afere-o em papel.
//
// DOMÍNIO ......... a faixa alvo, o nome d'ella, as listas que ha, e a tecla.
// CONTRA-DOMÍNIO .. o estado mudado, e um Pedido que a janella cumpre pelas
//                   ordens que já existem.
// INVARIANTE ...... aberto, o menu toma TODA tecla, sem excepção. Menu que
//                   deixasse a seta passar faria o dedo andar na pauta por
//                   baixo d'elle, com o alvo a mudar sem que ninguem o visse.
// Q.E.D. .......... sendo a taboada funcção do estado e da tecla, a bateria
//                   arma o caso á mão, sem navegador, sem tocador e sem tela.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <ftxui/component/event.hpp>

#include "nucleo/rol.hpp"

namespace mysong::tui {

// Os CINCO itens, na ordem em que se lêem. Enum, e não indice nú: o `switch` de
// quem os cumpre acende aviso no dia em que se lhes junte o sexto.
enum class ItemDoMenu { Toca, JuntaALista, NovaLista, Renomeia, Apaga };
inline constexpr std::size_t QUANTOS_ITENS = 5;

// O que o menu PEDE quando se escolhe. Não são verbos do tocador nem do banco:
// a janella traduz cada um na ordem que a tecla d'elle já cumpre, e assim o
// menu não ganha um segundo caminho para renomear nem para apagar.
enum class PedidoDoMenu { Nada, Toca, Junta, NovaLista, Renomeia, Apaga };

// O pedido, e o que elle carrega: `lista` é o id da lista escolhida, e sómente
// no Junta. Zero nos demais, de proposito, pela regra do gesto do rato: pedido
// que não tem alvo não ha de carregar numero que alguem possa vir a ler.
struct RespostaDoMenu {
  PedidoDoMenu pedido = PedidoDoMenu::Nada;
  int lista = 0;
};

// O ESTADO. Struct de valores, e não classe com punhos: quem o muta é a
// taboada abaixo, e quem o lê é a pintura; guarda alguma se lhe pede que o
// proprio compilador não faça.
struct MenuDeContexto {
  bool aberto = false;
  std::size_t faixa = 0;  // o indice ABSOLUTO da vista, que é o que acha a linha
  std::string titulo;     // o nome da faixa, que vae no titulo do menu
  std::size_t item = 0;   // qual dos cinco está eleito
  bool submenu = false;   // o das listas, aberto ou não
  std::size_t lista = 0;  // qual das listas está eleita dentro d'elle
  // As listas colhem-se ao ABRIR, e não a cada quadro: o relogio repinta vinte
  // vezes por segundo, e perguntá-las ao banco seriam vinte consultas por
  // segundo por uma cousa que sómente muda quando o operador cria ou apaga uma.
  std::vector<nucleo::Rol> listas;
};

// abre_o_menu — o menu de pé sobre a faixa, no primeiro item e com o submenu
// fechado. Abrir SEMPRE do principio é decisão, e não descuido: menu que
// lembrasse o item da vez passada poria o APAGAR debaixo do Enter de quem
// acabou de apagar, e a segunda faixa hia-se sem que se lesse o que se escolheu.
void abre_o_menu(MenuDeContexto& menu, std::size_t faixa, std::string titulo,
                 std::vector<nucleo::Rol> listas);

// tecla_no_menu — a taboada de dentro, e o unico logar que muta o estado. TODA
// tecla se consome: as que a taboada não conhece devolvem Nada, e Nada quer
// dizer «consumida, e nada mais ha a fazer». O evento do rato entra aqui
// tambem: o botão a DESCER fecha o menu, e o a subir não, que o modo 1000 manda
// sempre o soltar e o menu fechava no mesmo clique que o abriu.
RespostaDoMenu tecla_no_menu(MenuDeContexto& menu, const ftxui::Event& tecla);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
