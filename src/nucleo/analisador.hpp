// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ANALISADOR — src/nucleo/analisador.hpp
// ══════════════════════════════════════════════════════════════════════════
// Colhe o som da PROPRIA musica, e de mais nada. Acha no grafo do PipeWire o nó
// que o nosso mpv creou, e taponeia as portas de SAHIDA d'aquelle nó, com um
// segundo enlace em paralelo ao que já vae ao alto-falante. Notificação do
// systema, som de navegador e alerta de terminal não entram no espectro por
// CONSTRUCÇÃO, e não por filtro: elles são outros nós, e nós não os olhamos.
//
// DOMÍNIO ......... o grafo do PipeWire, tal como elle está: com nó, sem nó,
//                   com dous nós de mpv, com o serviço morto, e com o nó a
//                   morrer no meio de uma leitura.
// CONTRA-DOMÍNIO .. QUANTAS_BANDAS magnitudes em [0,1], a qualquer instante,
//                   por bandas(). Zeros enquanto não houver nó.
// INVARIANTE ...... o nó se acha pela IDENTIDADE e não pelo nome: o cliente do
//                   PipeWire cujo application.process.id é o nosso getpid(),
//                   porque a libmpv toca DENTRO do nosso processo. Nome «mpv»
//                   ha muitos na machina de quem ouve musica. E o alvo desce
//                   pelo object.serial, NUNCA pelo object.id: medido n'esta
//                   Casa que o gestor de sessão ignora o id e nos liga ao
//                   MICROPHONE, que se move com a musica por vasamento
//                   acustico e engana a prova ingenua.
// Q.E.D. .......... a prova é uma NEGATIVA: com a musica a tocar, dispara-se
//                   notify-send e um segundo tocador em 6000 Hz, e a banda de
//                   6000 Hz não se move. Medido antes de haver codigo:
//                   mag6000 = 0,00001 com intruso e sem elle.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace mysong::nucleo {

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
