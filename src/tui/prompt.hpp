// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROMPT — src/tui/prompt.hpp
// ══════════════════════════════════════════════════════════════════════════
// O MODO de digitar, e a LINHA do campo que d'elle depende. O campo tem linha
// PROPRIA, e não uma disputada: até a issue #79 elle escrevia por cima da
// trilha, e quem não reparasse continuava a navegar com as teclas a virarem
// lettras. A trilha sahiu na issue #102; a linha propria fica.
//
// DOMÍNIO ......... o modo, o termo que se vae digitando, e a largura.
// CONTRA-DOMÍNIO .. um `ftxui::Element` de UMA linha, e as taboadas puras que
//                   a janella consulta.
// INVARIANTE ...... o campo é o UNICO nó d'esta obra que pede foco, e ha de
//                   continuar a ser: o Render do FTXUI elege um nó focado por
//                   quadro e cala os outros sem aviso.
// Q.E.D. .......... sendo a linha funcção pura de (modo, termo), a bateria
//                   pinta-a em papel e afere a tinta e a collunha do cursor.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <ftxui/dom/elements.hpp>

namespace mysong::tui {
// Os ONZE estados. Oito DIGITAM, e são SETE officios (NomeNovo e NomeOutro dão
// nome á mesma lista); os dous Confirma são pergunta de uma tecla, que captura
// sem digitar; o Nada é a navegação.
//
// ConfirmaFaixa é modo PROPRIO, e não uma bandeira ao lado do Confirma: quem
// responde «s» tem de saber se apaga lista ou faixa, e o estado que vive n'uma
// variavel só é estado que não se pode desencontrar de si mesmo.
//
// NomeComEsta é o do menu de contexto (issue #96): pede o nome de uma lista
// NOVA e junta-lhe a faixa alvo. Modo proprio, e não o NomeNovo reusado, porque
// o Enter d'elle faz DUAS cousas em vez de uma; e rotulo que dissesse «LISTA
// NOVA» calaria a segunda, que é a que o operador não ha de descobrir depois.
enum class Modo { Nada, Busca, Url, Procura, NomeNovo, NomeOutro, Confirma,
                  Lista, TituloOutro, ConfirmaFaixa, NomeComEsta };

// aceita_letra — o modo escreve no termo? Falso em Nada e em Confirma.
bool aceita_letra(Modo modo) noexcept;

// assenta_novidade — a tela pode tomar novidade de fio de fundo? SÓMENTE em
// Nada: com o campo de pé a secção CONGELA, e o que chega espera (issue #79).
bool assenta_novidade(Modo modo) noexcept;

// rotulo_do_prompt — o que se escreve á esquerda do campo. O `contexto` é a
// fonte na Procura e o nome da lista no Confirma; os mais ignoram-no.
std::string rotulo_do_prompt(Modo modo, std::string_view contexto);

// elemento_do_campo — a linha do prompt, e sómente ella. A trilha sahiu da
// composição na issue #102: quem diz onde se está é a chapa por cima da pauta,
// e o campo ganha linha propria abaixo do trilho do progresso. Modo Nada dá
// elemento vazio, que a sala não lhe reserva linha alguma.
ftxui::Element elemento_do_campo(Modo modo, std::string_view contexto,
                                 const std::string& termo,
                                 std::size_t largura);

}  // namespace mysong::tui
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
