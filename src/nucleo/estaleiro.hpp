// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ESTALEIRO — src/nucleo/estaleiro.hpp
// ══════════════════════════════════════════════════════════════════════════
// A FILA DAS BAIXAS, com limite declarado. Baixar é lento e é de rede, e o
// operador que elege cinco faixas de seguida não ha de esperar pela primeira
// para encommendar a segunda. Mas tambem não se abrem cinco processos de yt-dlp
// ao mesmo tempo: a rede é uma, e cinco a disputá-la acabam todas mais tarde do
// que duas acabariam.
//
// DOMÍNIO ......... encommendas (Pedido), e a OBRA que as cumpre, que entra por
//                   parametro e não por chamada directa.
// CONTRA-DOMÍNIO .. o andamento: quantas correm, quantas esperam, quantas
//                   colheram, quantas falharam, e a razão da ultima.
// INVARIANTE ...... nunca correm mais que `obreiros` obras ao mesmo tempo, e o
//                   PICO fica registrado para que a promessa se possa aferir em
//                   vez de se acreditar.
// Q.E.D. .......... entrando a obra por parametro, a bateria põe no logar d'ella
//                   uma obra de mentira que se deixa segurar, e afere o limite
//                   sem tocar a rede e sem esperar por relogio.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <string>

namespace mysong::nucleo {

// O LIMITE. Dous, e não um: um serializa, e a segunda baixa esperaria pela
// primeira inteira. Dous, e não cinco: a rede é uma só.
inline constexpr std::size_t OBREIROS_DA_BAIXA = 2;

// O ANDAMENTO n'um instante. Cópia, e não punho para dentro: quem pergunta lê um
// retracto coherente, e não campos colhidos em instantes differentes.
struct Andamento {
  std::size_t em_curso = 0;
  std::size_t na_espera = 0;
  std::size_t colhidas = 0;
  std::size_t falhadas = 0;
  std::string ultima;  // a razão do ultimo desfecho; vazia quando nada acabou
};

// texto_do_andamento — a linha que a tela mostra. Funcção PURA, e por isso vive
// aqui e não na janella. Estaleiro quieto e sem historia devolve cadeia VAZIA:
// é o que faz a tela calar-se em vez de mostrar «0 a baixar».
std::string texto_do_andamento(const Andamento& andamento);

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
