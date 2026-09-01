// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA LINHA DE COMMANDO, LAVRA — src/nucleo/linha.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o que o cabeçalho promette. Varre o argv uma vez, e a RECUSA tem
// precedencia: opção que a taboa não conhece pára a leitura ali mesmo, porque
// acceitar-lhe a companhia calada é o defeito que esta lavra veio corrigir.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/linha.hpp"

#include <string_view>

#include "nucleo/marca.hpp"
#include "nucleo/versao.hpp"

namespace mysong::nucleo {
namespace {

// Opção principia por traço e tem mais de um byte; traço sozinho é caminho.
bool e_opcao(std::string_view arg) { return arg.size() > 1 && arg.front() == '-'; }

}  // namespace

Invocacao ler_linha(int argc, const char* const* argv) {
  Invocacao invocacao;
  bool encerradas = false;
  bool quer_ajuda = false, quer_versao = false, quer_sonda = false;
  bool quer_capa = false;

  for (int i = 1; i < argc; ++i) {
    const std::string_view arg(argv[i]);
    if (encerradas || !e_opcao(arg)) invocacao.faixas.emplace_back(arg);
    else if (arg == "--") encerradas = true;
    else if (arg == "--ajuda" || arg == "--help") quer_ajuda = true;
    else if (arg == "--versao" || arg == "--version") quer_versao = true;
    else if (arg == "--sonda") quer_sonda = true;
    else if (arg == "--capa") quer_capa = true;
    else {
      invocacao.modo = Modo::Recusa;
      invocacao.razao = "mysong: opcao desconhecida: " + std::string(arg) +
                        "\nCorra `mysong --ajuda` para ver as que existem.\n";
      return invocacao;
    }
  }

  if (quer_ajuda) invocacao.modo = Modo::Ajuda;
  else if (quer_versao) invocacao.modo = Modo::Versao;
  else if (quer_sonda) invocacao.modo = Modo::Sonda;
  else if (quer_capa) invocacao.modo = Modo::Capa;
  return invocacao;
}

// O nome e o numero, e mais nada: quem pergunta a versão costuma perguntal-a
// de dentro de um script, e linha de enfeite ali é linha a mais para cortar.
std::string texto_da_versao() {
  return std::string(marca()) + " " + std::string(versao()) + "\n";
}

// Texto PELADO, de linhas curtas: sem quadro, sem columna e sem glifo de Nerd
// Font. Assim o terminal estreito o reflue sem partir cousa alguma, e a ajuda
// se lê tambem na machina crua, onde o tocador nem chegaria a abrir.
std::string texto_da_ajuda() {
  return R"(mysong, tocador de musicas para o terminal.

Uso: mysong [opcao]... [faixa]...

  --versao, --version   diz o nome e o numero, e sahe
  --ajuda, --help       escreve estas linhas, e sahe
  --sonda               so o diagnostico dos requisitos, em texto
  --capa                busca no Cover Art Archive a capa que falta as
                        faixas do acervo, e a embute na etiqueta
  --                    encerra as opcoes; o que vem depois e caminho de
                        faixa, ainda que principie por traco

Sem faixa alguma, abre com a fila vazia. Opcao que nao esteja nesta
taboada e recusada, e a sahida vae differente de zero.

A opcao vale em qualquer logar da linha, e nao so antes das faixas.
Apparecendo mais de uma, a recusa manda em todas; depois della manda a
--ajuda, depois a --versao, depois o --sonda, e por fim o --capa.

O manual inteiro: man mysong
)";
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
