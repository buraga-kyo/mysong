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

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
