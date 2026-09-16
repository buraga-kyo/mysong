// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO EXEMPLO DA FITA DO ESPECTRO, exemplos/fita_espectro.cpp
// ══════════════════════════════════════════════════════════════════════════
// Pinta a fita de barras verticaes para se OLHAR, e para se INSPECCIONAR. Não é
// prova: é a peça que o olho do operador ha de julgar, n'um terminal com
// truecolor, e é tambem a sahida cujos bytes se conferem com o grep.
//
// Não colhe som algum: as bandas vêm do argumento, de sorte que a mesma linha de
// commando dá sempre os mesmos bytes. Fosse ella a colher do PipeWire, a sahida
// mudaria a cada corrida e prova alguma se poderia fazer d'ella.
//
// DOMÍNIO ......... largura e altura em célullas, e as bandas em [0,1]. Sem
//                   bandas, arma-se uma rampa determinística.
// CONTRA-DOMÍNIO .. `altura` linhas de fita, mais DUAS linhas de legenda (o
//                   nome da familia e a côr da batida d'ella), na sahida
//                   padrão; status zero.
// INVARIANTE ...... a tinta sahe IMMEDIATAMENTE antes do glifo que veste, sem
//                   repouso pelo meio, e cada linha remata em repouso.
// Q.E.D. .......... redigida a sahida a um arquivo, os bytes dizem quaes glifos
//                   sahiram, em que linha, e com que tinta. Que a fonte os
//                   resolva sem filete é materia que sómente o olho decide.
// ══════════════════════════════════════════════════════════════════════════
#include <cstdio>
#include <string_view>
#include <cstdlib>
#include <string>
#include <vector>

#include "nucleo/analisador.hpp"
#include "tui/espectro.hpp"
#include "tui/tokens.hpp"

namespace es = mysong::tui;
namespace tk = mysong::tui::tokens;

// glifos, a cadeia partida em pontos de codigo, um por CÉLULLA. MÉDIOS-GRAVES
// leva acento: contado em octetos, o nome sahiria mais largo do que occupa.
std::vector<std::string> glifos(std::string_view texto) {
  std::vector<std::string> saida;
  for (std::size_t i = 0; i < texto.size();) {
    const unsigned char oct = static_cast<unsigned char>(texto[i]);
    std::size_t quantos =
        oct < 0x80 ? 1u : (oct < 0xe0 ? 2u : (oct < 0xf0 ? 3u : 4u));
    if (i + quantos > texto.size()) quantos = 1;
    saida.emplace_back(texto.substr(i, quantos));
    i += quantos;
  }
  return saida;
}

// legenda, uma linha de rotulos por baixo, cada um na côr da batida do registro
// e sob as columnas que o vestem. O rotulo vem de FÓRA porque são DUAS linhas: o
// nome da familia e a côr da batida d'ella não cabem juntos n'um bloco de quinze
// collunhas, que é o que os medios-agudos occupão n'uma fita de setenta e duas.
std::string legenda(const es::Quadro& quadro,
                    std::string_view (*rotulo)(es::Registro)) {
  std::vector<std::string> celulas(quadro.largura, " ");
  std::size_t c = 0;
  while (c < quadro.largura) {
    std::size_t fim = c;
    while (fim < quadro.largura && quadro.registros[fim] == quadro.registros[c])
      ++fim;
    const std::vector<std::string> nome = glifos(rotulo(quadro.registros[c]));
    // Centrado no bloco, e CORTADO quando o bloco é mais estreito que o nome:
    // fita estreita mostra o principio do nome, e não nome nenhum.
    const std::size_t largo = fim - c;
    const std::size_t posto =
        nome.size() < largo ? c + (largo - nome.size()) / 2 : c;
    for (std::size_t k = 0; k < nome.size() && posto + k < fim; ++k)
      celulas[posto + k] = nome[k];
    c = fim;
  }
  std::string linha;
  for (std::size_t i = 0; i < quadro.largura; ++i)
    linha += tk::tinta(es::tinta_do_registro(quadro.registros[i])) + celulas[i];
  return linha + std::string(tk::repouso) + '\n';
}

// batidas, põe UMA banda de cada familia no alto, para que as quatro côres da
// batida appareção n'uma corrida só. Toma a banda do MEIO de cada familia, que
// junto da fronteira a côr da vizinha encostaria n'ella. O valor passa do
// limiar ABSOLUTO, que é a regra que vale sem picos, e picos não os ha aqui:
// uma corrida sósinha não tem quadro anterior de que os colher.
void batidas(std::vector<float>& bandas) {
  const std::vector<float> centros = es::centros_da_escala(bandas.size());
  for (const es::Registro registro :
       {es::Registro::Graves, es::Registro::MediosGraves,
        es::Registro::MediosAgudos, es::Registro::Agudos}) {
    std::size_t principio = bandas.size(), fim = 0;
    for (std::size_t b = 0; b < centros.size(); ++b)
      if (es::registro_da_banda(centros[b]) == registro) {
        if (b < principio) principio = b;
        fim = b + 1;
      }
    if (principio < fim) bandas[(principio + fim - 1) / 2] = 0.95f;
  }
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr,
                 "uso: fita_espectro <largura> <altura> [--mudo] [banda...]\n"
                 "  sem bandas, arma-se uma rampa de %zu degraus.\n",
                 mysong::nucleo::QUANTAS_BANDAS);
    return 2;
  }
  const int largura = std::atoi(argv[1]);
  const int altura = std::atoi(argv[2]);
  if (largura < 0 || altura < 0) return 2;

  bool mudo = false;
  std::vector<float> bandas;
  for (int i = 3; i < argc; ++i) {
    if (std::string(argv[i]) == "--mudo") { mudo = true; continue; }
    bandas.push_back(static_cast<float>(std::atof(argv[i])));
  }
  // A rampa determinística: a banda b vale b sobre QUANTAS_BANDAS, de sorte que
  // a fita sobe da esquerda para a direita e o gradiente se lê em toda a altura.
  if (bandas.empty()) {
    for (std::size_t b = 0; b < mysong::nucleo::QUANTAS_BANDAS; ++b)
      bandas.push_back(static_cast<float>(b) /
                       static_cast<float>(mysong::nucleo::QUANTAS_BANDAS));
    batidas(bandas);
  }

  const es::Quadro quadro =
      es::compor(bandas, static_cast<std::size_t>(largura),
                 static_cast<std::size_t>(altura), mudo);

  std::string tela;
  for (std::size_t l = 0; l < quadro.altura; ++l) {
    for (std::size_t c = 0; c < quadro.largura; ++c)
      tela += es::sequencia_da_celula(quadro.em(l, c));
    tela += tk::repouso;
    tela += '\n';
  }
  // A legenda por BAIXO da fita, e não por cima: o nome fica ao pé do pé das
  // columnas que nomeia, que é onde o olho o vae buscar.
  // UMA linha de legenda (issue #167): a côr da batida é uma só, o rosa, e
  // nomeá-la por familia diria quatro côres que a fita já não tem.
  tela += legenda(quadro, es::nome_do_registro);
  std::fputs(tela.c_str(), stdout);
  return 0;
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
