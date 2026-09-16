// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA ONDA NA TELA, src/tui/onda.hpp
// ══════════════════════════════════════════════════════════════════════════
// A onda da faixa n'UMA linha da fita, ao modo do SoundCloud: uma barra por
// collunha, em blocos de um a oito oitavos, o que já tocou n'uma côr e o que
// falta n'outra. COMPÕE e não colhe: recebe os pontos já feitos, e de ffmpeg,
// de cache e de fio nada sabe.
//
// DOMÍNIO ......... os pontos em [0,1], a posição, a duração, a largura.
// CONTRA-DOMÍNIO .. `largura` valores dobrados, e um `ftxui::Element` de UMA
//                   linha, o mesmo sempre para a mesma entrada.
// INVARIANTE ...... dobrar funde por MÁXIMO e nunca amostra: pico algum some
//                   ao estreitar. A linha tem SEMPRE a largura pedida.
// Q.E.D. .......... sendo funcção da entrada, a bateria lê-a cella a cella.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <vector>

#include <ftxui/dom/elements.hpp>

namespace mysong::tui {

// dobrar, os pontos fundidos á largura pedida, e devolve EXACTAMENTE
// `largura` valores. Funde por MÁXIMO e não amostra, pela razão medida no
// espectro: amostrar faria um pico desapparecer só porque o operador
// estreitou a janella, e onda que apaga ao redimensionar lê-se como defeito.
// Largura maior que a conta estica. Pontos vazios, ou largura zero, dão vazio.
std::vector<float> dobrar(const std::vector<float>& pontos, std::size_t largura);

// elemento_da_onda, a linha de `largura` cellas, uma de altura, fundo
// `tokens::panel`. Com pontos, cada cella é o bloco U+2581 a U+2588 do valor
// dobrado, e NUNCA menos de um oitavo: sem esse piso, o trecho calado sahiria
// em cella vazia e a base da onda desapparecia. Sem pontos, a cella é o traço
// pesado do trilho, que é o que a fita mostrava antes d'ella.
//
// As collunhas até o `enchimento` do transporte vestem `v600` (ou `glow_core`
// com foco, que é o que esta peça tem para accender); as demais `line_dim`.
// Largura zero dá elemento vazio; a `caixa` recebe o reflect, por onde o
// clique ha de buscar a posição.
ftxui::Element elemento_da_onda(const std::vector<float>& pontos,
                                double posicao, double duracao,
                                std::size_t largura, bool com_foco,
                                ftxui::Box* caixa = nullptr);

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BURAGA KYO., buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
