// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROMPT — src/tui/prompt.hpp
// ══════════════════════════════════════════════════════════════════════════
// O MODO de digitar, e o TOPO da tela que d'elle depende. A trilha diz ONDE se
// está; o prompt diz o QUE se digita. Duas linhas, e não uma disputada: até a
// issue #79 o prompt escrevia por cima da trilha, e quem não reparasse
// continuava a navegar com as teclas a virarem lettras.
//
// DOMÍNIO ......... o modo, a trilha já montada, o termo que se vae digitando,
//                   e a largura em collunhas.
// CONTRA-DOMÍNIO .. um `ftxui::Element` de uma ou duas linhas, e as taboadas
//                   puras que a janella consulta.
// INVARIANTE ...... a trilha entra INTACTA e sahe intacta: a cadeia d'ella e a
//                   do prompt são parâmetros differentes, e caminho algum
//                   d'este modulo as junta. Estructural, e não vigilancia.
// Q.E.D. .......... sendo o topo funcção pura de (trilha, modo, termo), a
//                   bateria pinta-o em papel e afere as duas linhas, a tinta e
//                   a collunha do cursor, sem erguer terminal.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <ftxui/dom/elements.hpp>

namespace mysong::tui {
// Os DEZ estados. Sete DIGITAM, e são SEIS officios (NomeNovo e NomeOutro dão
// nome á mesma lista); os dous Confirma são pergunta de uma tecla, que captura
// sem digitar; o Nada é a navegação.
//
// ConfirmaFaixa é modo PROPRIO, e não uma bandeira ao lado do Confirma: quem
// responde «s» tem de saber se apaga lista ou faixa, e o estado que vive n'uma
// variavel só é estado que não se pode desencontrar de si mesmo.
enum class Modo { Nada, Busca, Url, Procura, NomeNovo, NomeOutro, Confirma,
                  Lista, TituloOutro, ConfirmaFaixa };

// aceita_letra — o modo escreve no termo? Falso em Nada e em Confirma.
bool aceita_letra(Modo modo) noexcept;

// linhas_do_topo — UMA linha (a trilha) fechado o prompt, DUAS aberto.
std::size_t linhas_do_topo(Modo modo) noexcept;

// assenta_novidade — a tela pode tomar novidade de fio de fundo? SÓMENTE em
// Nada: com o campo de pé a secção CONGELA, e o que chega espera (issue #79).
bool assenta_novidade(Modo modo) noexcept;

// rotulo_do_prompt — o que se escreve á esquerda do campo. O `contexto` é a
// fonte na Procura e o nome da lista no Confirma; os mais ignoram-no.
std::string rotulo_do_prompt(Modo modo, std::string_view contexto);

// elemento_do_topo — a trilha e, havendo prompt, o prompt POR BAIXO d'ella.
ftxui::Element elemento_do_topo(const std::string& trilha, Modo modo,
                                std::string_view contexto,
                                const std::string& termo, std::size_t largura);

}  // namespace mysong::tui
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
