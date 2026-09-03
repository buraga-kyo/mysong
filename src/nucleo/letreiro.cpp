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

#include <fstream>
#include <string>
#include <system_error>

#include <unistd.h>

#include "nucleo/aquisicao.hpp"  // corre(): o exec sem shell
#include "nucleo/sonda.hpp"      // familia_installada: o fontconfig

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

Parecer parecer_do_letreiro(ModoDaLousa modo, bool ha_pango,
                            bool ha_familia) {
  if (modo == ModoDaLousa::Nao)
    return {false, "desligado com a lousa: lousa = nao"};
  // O PROGRAMA antes da FONTE, pela razão do parecer da lousa: quem não tem o
  // pacote ha de ler o remedio do pacote, e não o da fonte.
  if (!ha_pango) return {false, "falta o pango-view (pango1.0-tools)"};
  if (!ha_familia)
    return {false, "falta a fonte Xirod em ~/.local/share/fonts"};
  return {true, "Xirod, pango-view"};
}

bool ha_pango_view() {
  // Corre-se-lhe o `--version`, pelo molde do `versao_da_lousa`: perguntar ao
  // PATH á mão daria a mesma resposta por caminho que esta Casa já tem.
  std::string colhido;
  return corre({"pango-view", "--version"}, &colhido) == 0;
}

bool ha_familia_da_marca() { return familia_installada(FAMILIA_DA_MARCA); }

// texto_do_letreiro — a razão vae de pé ou deitado, e é ella a linha inteira:
// de pé ella diz «Xirod, pango-view», que é o que a issue pede á lettra.
std::string texto_do_letreiro(const Parecer& parecer) {
  return "\n  letreiro: " + parecer.razao + "\n";
}

namespace {

// cabeca_do_arquivo — os primeiros octetos, que é quanto o `medida_da_imagem`
// da capa precisa para dizer a largura e a altura sem decodificar imagem.
std::string cabeca_do_arquivo(const std::filesystem::path& onde) {
  std::ifstream entrada(onde, std::ios::binary);
  if (!entrada) return {};
  std::string cabeca(64, '\0');
  entrada.read(cabeca.data(), static_cast<std::streamsize>(cabeca.size()));
  cabeca.resize(static_cast<std::size_t>(entrada.gcount()));
  return cabeca;
}

// desfaz — apaga o temporario e responde vazio, que é o que toda queda d'aqui
// tem a fazer: chapa a meio no cache seria chapa rota para sempre.
std::filesystem::path desfaz(const std::filesystem::path& meio) {
  std::error_code erro;
  std::filesystem::remove(meio, erro);
  return {};
}

// rasteriza — o pango-view DUAS vezes, e é aqui que a margem se cumpre: folga
// não se adivinha sem medir, e a medida só apparece depois de a palavra estar
// desenhada. As duas corridas dão-se UMA vez na vida do cache.
//
// Por TEMPORARIO e RENAME, pelo molde do arquivo da capa: o rename no mesmo
// systema de arquivos é atomico, e resolve de graça a corrida entre duas
// instancias do tocador sobre a mesma chapa.
std::filesystem::path rasteriza(const Pedido& pedido) {
  const std::filesystem::path onde = caminho_da_chapa_em_cache(pedido);
  if (onde.empty()) return {};
  std::error_code erro;
  if (std::filesystem::is_regular_file(onde, erro) && !erro) return onde;
  std::filesystem::create_directories(onde.parent_path(), erro);
  if (erro) return {};
  // O `.png` fica na PONTA do temporario: o pango-view escolhe o formato da
  // sahida pela extensão, e nome acabado em `.parte` fal-o-hia recusar.
  const std::filesystem::path meio =
      onde.string() + "." + std::to_string(::getpid()) + ".parte.png";
  if (corre(argumentos_do_letreiro(pedido, 0, meio), nullptr) != 0)
    return desfaz(meio);
  const std::size_t margem =
      margem_da_chapa(medida_da_imagem(cabeca_do_arquivo(meio)),
                      pedido.cellulas, CELLULA_DA_CASA);
  // A segunda corrida sómente HAVENDO folga: chapa que já nasceu na proporção
  // da caixa não tem o que corrigir, e tornar a correr seria gasto por nada.
  if (margem > 0 &&
      corre(argumentos_do_letreiro(pedido, margem, meio), nullptr) != 0)
    return desfaz(meio);
  std::filesystem::rename(meio, onde, erro);
  return erro ? desfaz(meio) : onde;
}

}  // namespace

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
