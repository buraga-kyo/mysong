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

// margem_da_chapa — regra de tres, e nada mais: a caixa mede `cellulas` vezes
// a largura da cella por UMA altura d'ella, e a chapa ha de ter essa razão;
// d'onde a altura ALVO é a largura crua dividida pela razão da caixa. O que
// falta reparte-se em duas, que a folga vae em cima e em baixo.
std::size_t margem_da_chapa(Medida crua, std::size_t cellulas,
                            Medida cellula) {
  if (crua.largura == 0 || cellulas == 0 || cellula.largura == 0) return 0;
  const std::size_t alvo =
      crua.largura * cellula.altura / (cellulas * cellula.largura);
  // Chapa JÁ mais alta que o alvo não pede folga: alli é a ALTURA que manda na
  // reducção, e a chapa sahe mais estreita que a caixa. Isso é sobra de fundo
  // n'uma ponta, e não palavra cortada; folga negativa não ha.
  return alvo > crua.altura ? (alvo - crua.altura) / 2 : 0;
}

// chave_do_letreiro — o FNV-1a da Casa, o mesmo que dá nome á arte em cache.
std::string chave_do_letreiro(const Pedido& pedido) {
  // Os campos costuram-se com o octeto NULLO, que em nenhum d'elles apparece:
  // sem costura, texto «AB» com tinta «C» e texto «A» com tinta «BC» dariam a
  // mesma somma, e o que se veria era a chapa de outra aba.
  std::string tudo = pedido.texto;
  for (const std::string& campo :
       {pedido.familia, pedido.tinta, pedido.fundo,
        std::to_string(pedido.corpo), std::to_string(pedido.cellulas)}) {
    tudo.push_back('\0');
    tudo += campo;
  }
  return somma_dos_octetos(tudo);
}

std::filesystem::path caminho_da_chapa_em_cache(const Pedido& pedido) {
  // `letreiro/` ao lado de `capas/`, e não misturado com ellas: o que se guarda
  // aqui é palavra, e apagar uma pasta não ha de levar a outra.
  const std::filesystem::path raiz = raiz_do_cache();
  if (raiz.empty()) return {};
  return raiz / "letreiro" / (chave_do_letreiro(pedido) + ".png");
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
