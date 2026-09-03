// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO LETREIRO — src/nucleo/letreiro.cpp
// ══════════════════════════════════════════════════════════════════════════
// A lavra. As puras primeiro; o que corre o pango-view e escreve em disco fica
// no fim, pelo molde da lousa e o do arquivo da capa.
//
// DOMÍNIO ......... o pedido de chapa, e a medida que o pango-view devolveu.
// CONTRA-DOMÍNIO .. a linha de commando, a chave do cache, e o PNG em disco.
// INVARIANTE ...... funcção alguma d'aqui lança: programa que falhe devolve
//                   caminho vazio, e o cabeçalho fica com o mono de sempre.
// Q.E.D. .......... sendo a linha e a chave puras, a bateria afere o que se
//                   HA DE correr sem correr o pango-view uma vez.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/letreiro.hpp"

#include <string>

namespace mysong::nucleo {

// argumentos_do_letreiro — o `--font` leva a familia e o corpo n'UM argumento,
// que é como o Pango descreve fonte; o resto vae um por argumento, e shell
// alguma os torna a partir. Ordem d'elles pelo `argumentos_do_chafa`: o que
// governa primeiro, o alvo por ultimo.
std::vector<std::string> argumentos_do_letreiro(
    const Pedido& pedido, std::size_t margem,
    const std::filesystem::path& sahida) {
  return {"pango-view",
          "--font=" + pedido.familia + " " + std::to_string(pedido.corpo),
          "--foreground=" + pedido.tinta,
          "--background=" + pedido.fundo,
          // DOUS numeros, e não um: o primeiro é a folga de cima e de baixo, o
          // segundo a dos lados. A dos lados vae ZERO, que folga lateral
          // deslocaria a palavra dentro da caixa em vez de a levantar.
          "--margin=" + std::to_string(margem) + " 0",
          // Sem o `-q` o pango-view abre janella propria no meio da tela.
          "-q",
          "-o",
          sahida.string(),
          "-t",
          pedido.texto};
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
