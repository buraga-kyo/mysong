// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA CAPA — src/nucleo/capa.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. As puras primeiro; o chafa e a taglib no fim.
//
// DOMÍNIO ......... o caminho de uma faixa, e a geometria do painel.
// CONTRA-DOMÍNIO .. linhas de texto prontas a pintar, ou a ausencia declarada.
// INVARIANTE ...... funcção alguma d'aqui lança, e capa ausente não é falha.
// Q.E.D. .......... a chave do cache não conhece o chafa, donde a prova do cache
//                   corre sem imagem alguma no disco.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/capa.hpp"

#include "nucleo/aquisicao.hpp"  // corre(): o fork e o exec sem shell
#include "nucleo/sonda.hpp"      // familia_com_glypho(): a prova do glypho

#include <taglib/attachedpictureframe.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <cctype>

namespace mysong::nucleo {

const std::vector<std::string>& nomes_de_capa() {
  // A ordem é a convenção que os ripadores usam, e não gosto: `cover` é o nome que
  // o Picard grava, `folder` o que o Windows Media Player grava, e `front` o que os
  // acervos antigos trazem. Procura-se por essa ordem para que o acervo que tenha
  // dous mostre o que o ripador teve por principal.
  static const std::vector<std::string> kNomes = {
      "cover.jpg",  "cover.jpeg", "cover.png",  "cover.webp",
      "folder.jpg", "folder.png", "front.jpg",  "front.png",
      "album.jpg",  "album.png",  "capa.jpg",   "capa.png",
  };
  return kNomes;
}

std::filesystem::path capa_ao_lado(const std::filesystem::path& faixa) {
  const std::filesystem::path pasta = faixa.parent_path();
  std::error_code erro;
  for (const std::string& nome : nomes_de_capa()) {
    const std::filesystem::path tentativa = pasta / nome;
    if (std::filesystem::is_regular_file(tentativa, erro) && !erro)
      return tentativa;
  }
  return {};
}

std::string chave_do_cache(const std::filesystem::path& faixa,
                           std::size_t collunas, std::size_t linhas) {
  // A PASTA, e não a faixa: as faixas de um album partilham a capa. E o tamanho
  // entra na chave porque a arte tem de encher o painel, donde redimensionar pede
  // render novo e não o de antes esticado.
  return faixa.parent_path().string() + "\x1f" + std::to_string(collunas) +
         "x" + std::to_string(linhas);
}

// O SEXTANTE, e a classe de fonte que o desenha. Medido n'esta machina:
// `fc-list ':charset=1fb00'` acha sómente a Noto Sans Symbols2, que Nerd Font
// não é; a JetBrainsMono NF tem os quadrantes (U+2596) e não tem os sextantes.
// Logo aqui sahem quadrantes, e o sextante accende-se sozinho na machina cuja
// fonte o tenha, sem que se lhe mexa n'uma linha.
bool ha_sextante_na_fonte() {
  // O `static` local inicializa-se UMA vez, e desde o C++11 a norma garante-o
  // contra fios (o «magic static»); esta Casa compila em C++17. Sem elle, o
  // pintor pediria ao fontconfig a taboa das fontes vinte vezes por segundo.
  static const bool desenha = familia_com_glypho("nerd", 0x1FB00);
  return desenha;
}

std::vector<std::string> argumentos_do_chafa(
    const std::filesystem::path& imagem, std::size_t collunas,
    std::size_t linhas, bool com_sextante) {
  // As bandeiras que importam, e todas por medição e não por leitura do
  // manual. Corri o chafa e li a sahida com `cat -v`:
  //
  //   `--polite=on`    inhibe o esconde-cursor `ESC[?25l` e o limpa-tela `ESC[2J`.
  //                    Sem ella, a primeira linha da capa vinha com um limpa-tela
  //                    dentro, e o FTXUI pintava-a apagando o quadro inteiro.
  //   `--relative=off` faz as linhas separarem-se por mudança de linha em vez de
  //                    por posicionamento de cursor. É o que permitte cortar a
  //                    sahida em linhas e entregá-las ao FTXUI uma a uma.
  //   `--animate=off`  uma imagem, e não um filme.
  //   `--colors=full`  côr de verdade, que é o que a paleta d'esta Casa pede.
  //
  // Nota: `--clear` NÃO toma argumento. Escrevi `--clear off` de inicio, e o
  // `off` virou nome de arquivo: «chafa: Failed to open 'off'».
  //
  // E as tres que NÃO entram, cada uma por medição (issue #94):
  //   `--dither`       o proprio chafa diz «no effect with 24-bit color», e a
  //                    Casa corre `--colors=full`. Conferi os quatro modos: dão
  //                    o mesmo arquivo, byte a byte.
  //   `--color-space`  serve á QUANTIZAÇÃO, e a 24 bits não ha quantização;
  //                    `din99d` sahe egual a `rgb`, byte a byte.
  //   `--stretch`      existe, ao contrario do que se suppunha, e é justamente
  //                    por NÃO se passar que a proporção se guarda: medido,
  //                    1280x720 em 40x21 sahe 40x12, e 200x200 sahe 40x20.
  const std::string symbolos =
      com_sextante ? "--symbols=block+half+quad+sextant"
                   : "--symbols=block+half+quad";
  return {"chafa",
          "--format=symbols",
          symbolos,
          "--work=9",
          "--size=" + std::to_string(collunas) + "x" + std::to_string(linhas),
          "--animate=off",
          "--relative=off",
          "--polite=on",
          "--colors=full",
          "--",
          imagem.string()};
}

std::string arte_embutida(const std::filesystem::path& faixa) {
  // Cada guarda cobre um caso MEDIDO, e não um receio. Arquivo que não é MP3 (o
  // `.mkv` do video, um `.webm`, lixo, arquivo vazio) faz a taglib dar `isValid()`
  // verdadeiro e etiqueta NÃO nula, que ella a cria a pedido; o que vem vazio é a
  // lista de quadros. D'onde a guarda que importa é a do APIC vazio, e não a do
  // ponteiro nulo, e é por ella que a busca sahe quieta em logar de lançar.
  TagLib::MPEG::File arquivo(faixa.c_str());
  if (!arquivo.isValid() || arquivo.ID3v2Tag() == nullptr) return {};
  // O PRIMEIRO quadro, e não o da frente: o typo não se consulta. Vindo a faixa de
  // ripador que gravou capa E contra-capa, sahe o que estiver á frente na etiqueta.
  const auto& quadros = arquivo.ID3v2Tag()->frameListMap()["APIC"];
  if (quadros.isEmpty()) return {};
  const auto* arte =
      dynamic_cast<const TagLib::ID3v2::AttachedPictureFrame*>(quadros.front());
  if (arte == nullptr || arte->picture().isEmpty()) return {};
  return std::string(arte->picture().data(),
                     static_cast<std::size_t>(arte->picture().size()));
}

bool embute_arte(const std::filesystem::path& faixa, std::string_view octetos) {
  // O mime sahe do CONTEUDO, e de extensão nenhuma: o CAA promette JPEG na
  // miniatura, e prometter não é medir. Fóra os dous formatos que o painel e
  // os leitores alheios sabem ler, recusa-se ANTES de abrir o arquivo.
  const bool jpeg = octetos.size() > 3 &&
                    static_cast<unsigned char>(octetos[0]) == 0xFF &&
                    static_cast<unsigned char>(octetos[1]) == 0xD8;
  const bool png = octetos.size() > 8 &&
                   octetos.compare(0, 8, "\x89PNG\r\n\x1a\n") == 0;
  if (!jpeg && !png) return false;
  TagLib::MPEG::File arquivo(faixa.c_str());
  if (!arquivo.isValid()) return false;
  auto* quadro = new TagLib::ID3v2::AttachedPictureFrame();
  quadro->setMimeType(jpeg ? "image/jpeg" : "image/png");
  quadro->setType(TagLib::ID3v2::AttachedPictureFrame::FrontCover);
  quadro->setPicture(
      TagLib::ByteVector(octetos.data(), static_cast<unsigned>(octetos.size())));
  // A etiqueta cria-se A PEDIDO (o `true`), que faixa antiga pode nem ter
  // ID3v2; e o quadro entregue passa a ser da taglib, que o desfaz no save.
  arquivo.ID3v2Tag(true)->addFrame(quadro);
  return arquivo.save();
}

namespace {

// extrahe_embutida — a mesma arte, posta n'um temporario para que o chafa a possa
// abrir. O chafa lê arquivo, e não memoria, e é só por isso que este passo existe.
//
// Grava-se em `$XDG_RUNTIME_DIR`, e não em `/tmp`: arte de album é dado do
// operador, e o directorio de corrida é privado d'elle por construcção. A extensão
// `.bin` não engana ferramenta alguma: o chafa conhece a imagem pelo CONTEUDO, e
// medi-o sobre PNG, JPEG e WebP com esse mesmo nome.
std::filesystem::path extrahe_embutida(const std::filesystem::path& faixa) {
  const std::string arte = arte_embutida(faixa);
  if (arte.empty()) return {};

  const char* corrida = std::getenv("XDG_RUNTIME_DIR");
  std::filesystem::path onde =
      corrida != nullptr && corrida[0] != '\0'
          ? std::filesystem::path(corrida)
          : std::filesystem::temp_directory_path();
  onde /= "mysong-capa.bin";
  std::ofstream sahida(onde, std::ios::binary | std::ios::trunc);
  if (!sahida) return {};
  sahida.write(arte.data(), static_cast<std::streamsize>(arte.size()));
  return sahida.good() ? onde : std::filesystem::path();
}

}  // namespace

const CapaPintada& Galeria::capa(const std::filesystem::path& faixa,
                                 std::size_t collunas, std::size_t linhas) {
  const std::string chave = chave_do_cache(faixa, collunas, linhas);
  const auto assento = guardadas_.find(chave);
  if (assento != guardadas_.end()) return assento->second;

  CapaPintada pintada;
  if (collunas > 0 && linhas > 0 && !faixa.empty()) {
    // A capa AO LADO ganha da embutida: ella é a que o operador pode trocar sem
    // reescrever a etiqueta, e por isso é a que elle manda.
    std::filesystem::path imagem = capa_ao_lado(faixa);
    if (imagem.empty()) imagem = extrahe_embutida(faixa);
    if (!imagem.empty()) {
      std::string colhido;
      if (corre(argumentos_do_chafa(imagem, collunas, linhas,
                                    ha_sextante_na_fonte()), &colhido) == 0 &&
          !colhido.empty()) {
        std::size_t principio = 0;
        while (principio < colhido.size()) {
          const std::size_t fim = colhido.find('\n', principio);
          const std::size_t ate = fim == std::string::npos ? colhido.size() : fim;
          pintada.linhas.push_back(
              analysa_sgr(std::string_view(colhido).substr(principio,
                                                           ate - principio)));
          if (fim == std::string::npos) break;
          principio = fim + 1;
        }
        pintada.achada = !pintada.linhas.empty();
        ++renders_;
      }
    }
  }
  // A AUSENCIA guarda-se tambem: sem isto, album sem capa faria a Casa procurar o
  // arquivo no disco vinte vezes por segundo para achar sempre o mesmo nada.
  return guardadas_.emplace(chave, std::move(pintada)).first->second;
}

std::size_t Galeria::quantos_renders() const noexcept { return renders_; }

namespace {

// aplica_sgr — lê UM escape do chafa e assenta a côr na corrida que vem. Sómente os
// codigos que o chafa emitte: 38;2;R;G;B, 48;2;R;G;B, 39, 49, 0 e 7. Codigo que não se
// conheça ignora-se, e não se lança: o chafa é ferramenta alheia e pode mudar.
void aplica_sgr(std::string_view escape, Corrida* corrida) {
  // O corpo entre `ESC[` e `m`, partido por ponto e virgula.
  const std::size_t abre = escape.find('[');
  if (abre == std::string_view::npos) return;
  std::string_view corpo = escape.substr(abre + 1);
  if (!corpo.empty() && corpo.back() == 'm') corpo.remove_suffix(1);

  std::vector<int> cifras;
  std::size_t principio = 0;
  while (principio <= corpo.size()) {
    const std::size_t ponto = corpo.find(';', principio);
    const std::size_t ate = ponto == std::string_view::npos ? corpo.size() : ponto;
    const std::string pedaco(corpo.substr(principio, ate - principio));
    cifras.push_back(pedaco.empty() ? 0 : std::atoi(pedaco.c_str()));
    if (ponto == std::string_view::npos) break;
    principio = ponto + 1;
  }

  for (std::size_t i = 0; i < cifras.size(); ++i) {
    if (cifras[i] == 0) {
      *corrida = Corrida{corrida->texto, -1, -1, -1, -1, -1, -1};
    } else if (cifras[i] == 39) {
      corrida->r_frente = corrida->g_frente = corrida->b_frente = -1;
    } else if (cifras[i] == 49) {
      corrida->r_fundo = corrida->g_fundo = corrida->b_fundo = -1;
    } else if ((cifras[i] == 38 || cifras[i] == 48) && i + 4 < cifras.size() &&
               cifras[i + 1] == 2) {
      const bool frente = cifras[i] == 38;
      int* alvo = frente ? &corrida->r_frente : &corrida->r_fundo;
      alvo[0] = cifras[i + 2];
      alvo[1] = cifras[i + 3];
      alvo[2] = cifras[i + 4];
      i += 4;
    }
  }
}

}  // namespace

std::vector<Corrida> analysa_sgr(std::string_view linha) {
  std::vector<Corrida> corridas;
  Corrida corrente;
  for (std::size_t i = 0; i < linha.size();) {
    if (linha[i] != 0x1b) {  // texto: junta-se, octeto a octeto, á corrida corrente
      // Octeto a octeto BASTA, e o multibyte não pede cuidado algum: acumulando-se
      // contiguamente na mesma cadeia, tomar um octeto ou tres dá o mesmo resultado,
      // e o caracter sómente se poderia partir se a corrida fechasse a meio d'elle,
      // o que não acontece porque sómente um escape a fecha e escape nunca vem no
      // meio de um caracter. Escrevi primeiro o laço que junta as continuações, e a
      // mutação provou-o inutil: tirá-lo não mata caso algum.
      corrente.texto += linha[i];
      ++i;
      continue;
    }
    // Um escape: a corrida corrente fecha-se, e a côr nova principia a seguinte.
    std::size_t fim = i + 1;
    while (fim < linha.size() && linha[fim] != 'm' && linha[fim] != 0x1b) ++fim;
    const std::string_view corpo = linha.substr(i, fim - i + 1);
    if (!corrente.texto.empty()) {
      corridas.push_back(corrente);
      corrente.texto.clear();
    }
    aplica_sgr(corpo, &corrente);
    i = fim < linha.size() ? fim + 1 : linha.size();
  }
  if (!corrente.texto.empty()) corridas.push_back(corrente);
  return corridas;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
