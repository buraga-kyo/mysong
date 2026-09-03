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
#include <unistd.h>

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <cctype>
#include <utility>

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

namespace {

// de_dous e de_quatro — inteiros GRANDES-PRIMEIRO, que é a ordem do JPEG e a do
// PNG. Sem guarda de tamanho: os dous chamadores conferem-no antes.
std::size_t de_dous(std::string_view octetos, std::size_t onde) {
  return (static_cast<std::size_t>(static_cast<unsigned char>(octetos[onde]))
          << 8) |
         static_cast<unsigned char>(octetos[onde + 1]);
}

std::size_t de_quatro(std::string_view octetos, std::size_t onde) {
  return (de_dous(octetos, onde) << 16) | de_dous(octetos, onde + 2);
}


// medida_do_jpeg — anda pelos segmentos até o SOF, que é o unico que traz o
// quadro. SOF são as marcas C0 a CF menos a C4, a C8 e a CC, que carregam
// taboas de Huffman, extensão e taboas arithmeticas: corpo d'outra especie.
// Andar é preciso porque o APIC do yt-dlp traz o JFIF, e ás vezes o EXIF,
// ANTES do quadro: quem lesse a posição fixa leria a miniatura da camera.
Medida medida_do_jpeg(std::string_view octetos) {
  std::size_t i = 2;  // a guarda FFD8 já se conferiu
  while (i + 9 < octetos.size()) {
    if (static_cast<unsigned char>(octetos[i]) != 0xFF) return {};
    const auto marca = static_cast<unsigned char>(octetos[i + 1]);
    // Enchimento, e as marcas sem corpo algum: andam de dous em dous.
    if (marca == 0xFF || marca == 0x01 || (marca >= 0xD0 && marca <= 0xD9)) {
      ++i;
      if (marca != 0xFF) ++i;
      continue;
    }
    if (marca >= 0xC0 && marca <= 0xCF && marca != 0xC4 && marca != 0xC8 &&
        marca != 0xCC)
      return {de_dous(octetos, i + 7), de_dous(octetos, i + 5)};
    const std::size_t tamanho = de_dous(octetos, i + 2);
    if (tamanho < 2) return {};
    i += 2 + tamanho;
  }
  return {};
}

}  // namespace

Medida medida_da_imagem(std::string_view octetos) {
  if (octetos.size() > 3 && static_cast<unsigned char>(octetos[0]) == 0xFF &&
      static_cast<unsigned char>(octetos[1]) == 0xD8)
    return medida_do_jpeg(octetos);
  // O PNG diz o quadro no IHDR, que a norma manda ser o PRIMEIRO pedaço: a
  // largura no octeto dezaseis e a altura no vinte, contando da guarda.
  if (octetos.size() >= 24 && octetos.compare(0, 8, "\x89PNG\r\n\x1a\n") == 0 &&
      octetos.compare(12, 4, "IHDR") == 0)
    return {de_quatro(octetos, 16), de_quatro(octetos, 20)};
  return {};  // WebP e o mais: quem chama toma isto por «não sei»
}

Retangulo rectangulo_da_capa(Medida imagem, std::size_t tecto_collunas,
                             std::size_t tecto_linhas, Medida cellula) {
  if (tecto_collunas == 0 || tecto_linhas == 0) return {};
  if (imagem.largura == 0 || imagem.altura == 0 || cellula.largura == 0 ||
      cellula.altura == 0)
    return {tecto_collunas, tecto_linhas};
  // Em inteiro GRANDE, e arredondando para cima: capa de mil e oitenta por mil
  // e oitenta em cento e vinte collunhas passa dos dous milhões, e o
  // rectangulo curto de uma linha deixaria a imagem a pingar sobre o espectro.
  using Conta = unsigned long long;
  const Conta por_alto = Conta(imagem.largura) * cellula.altura;
  const Conta linhas =
      (Conta(tecto_collunas) * imagem.altura * cellula.largura + por_alto - 1) /
      por_alto;
  if (linhas <= tecto_linhas)
    return {tecto_collunas, static_cast<std::size_t>(linhas < 1 ? 1 : linhas)};
  const Conta por_largo = Conta(imagem.altura) * cellula.largura;
  const Conta collunas =
      (Conta(tecto_linhas) * imagem.largura * cellula.altura + por_largo - 1) /
      por_largo;
  return {static_cast<std::size_t>(collunas > tecto_collunas ? tecto_collunas
                                   : collunas < 1            ? 1
                                                             : collunas),
          tecto_linhas};
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
  static const bool desenha = familia_com_glypho(CLASSE_DA_FONTE, 0x1FB00);
  return desenha;
}

bool sextante_de(Sextantes ajuste) {
  switch (ajuste) {
    case Sextantes::Sim: return true;
    case Sextantes::Nao: return false;
    case Sextantes::Auto: break;
  }
  return ha_sextante_na_fonte();
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

std::string somma_dos_octetos(std::string_view octetos) {
  // A semente e o primo são os da norma do FNV-1a de 64 bits.
  std::uint64_t somma = 14695981039346656037ULL;
  for (const char letra : octetos) {
    somma ^= static_cast<unsigned char>(letra);
    somma *= 1099511628211ULL;
  }
  std::string hexadecimal(16, '0');
  for (std::size_t i = 16; i > 0; --i) {
    hexadecimal[i - 1] = "0123456789abcdef"[somma & 0xF];
    somma >>= 4;
  }
  return hexadecimal;
}

std::string_view extensao_da_capa(std::string_view octetos) {
  if (octetos.size() > 3 && static_cast<unsigned char>(octetos[0]) == 0xFF &&
      static_cast<unsigned char>(octetos[1]) == 0xD8)
    return "jpg";
  if (octetos.size() > 8 && octetos.compare(0, 8, "\x89PNG\r\n\x1a\n") == 0)
    return "png";
  return {};
}

std::filesystem::path caminho_da_capa_em_cache(std::string_view octetos) {
  const std::string_view extensao = extensao_da_capa(octetos);
  if (extensao.empty()) return {};
  // O CACHE, e não o directorio de corrida em que o chafa recebia a arte:
  // aquelle morre no fim da sessão, e a lousa quer o arquivo enquanto a janella
  // estiver de pé. E cache é o logar certo, que isto se apaga sem perda.
  const char* const posto = std::getenv("XDG_CACHE_HOME");
  std::filesystem::path raiz;
  if (posto != nullptr && posto[0] != '\0') {
    raiz = std::filesystem::path(posto);
  } else {
    const char* const casa = std::getenv("HOME");
    if (casa == nullptr) return {};
    raiz = std::filesystem::path(casa) / ".cache";
  }
  return raiz / "mysong" / "capas" /
         (somma_dos_octetos(octetos) + "." + std::string(extensao));
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

CapaPintada pinta_imagem(const std::filesystem::path& imagem,
                         std::size_t collunas, std::size_t linhas,
                         bool com_sextante) {
  CapaPintada pintada;
  if (imagem.empty() || collunas == 0 || linhas == 0) return pintada;
  std::string colhido;
  if (corre(argumentos_do_chafa(imagem, collunas, linhas, com_sextante),
            &colhido) != 0 || colhido.empty()) return pintada;
  std::size_t principio = 0;
  while (principio < colhido.size()) {
    const std::size_t fim = colhido.find('\n', principio);
    const std::size_t ate = fim == std::string::npos ? colhido.size() : fim;
    pintada.linhas.push_back(analysa_sgr(
        std::string_view(colhido).substr(principio, ate - principio)));
    if (fim == std::string::npos) break;
    principio = fim + 1;
  }
  pintada.achada = !pintada.linhas.empty();
  return pintada;
}

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
    pintada = pinta_imagem(imagem, collunas, linhas, com_sextante_);
    if (pintada.achada) ++renders_;
  }
  // A AUSENCIA guarda-se tambem: sem isto, album sem capa faria a Casa procurar o
  // arquivo no disco vinte vezes por segundo para achar sempre o mesmo nada.
  return guardadas_.emplace(chave, std::move(pintada)).first->second;
}

std::size_t Galeria::quantos_renders() const noexcept { return renders_; }

namespace {

// cabeca_do_arquivo — o que a medida pede: sessenta e quatro mil octetos, que
// chegam para o SOF do JPEG, que vem depois do JFIF e ás vezes do EXIF.
std::string cabeca_do_arquivo(const std::filesystem::path& caminho) {
  std::ifstream entrada(caminho, std::ios::binary);
  if (!entrada) return {};
  std::string cabeca(64 * 1024, '\0');
  entrada.read(cabeca.data(), static_cast<std::streamsize>(cabeca.size()));
  cabeca.resize(static_cast<std::size_t>(entrada.gcount()));
  return cabeca;
}

// escreve_no_cache — a arte em arquivo UMA vez: o nome vem do CONTEUDO, d'onde
// arquivo de mesmo nome já tem os octetos e não se torna a escrever.
//
// Por TEMPORARIO e RENAME, e não direito ao nome final: quem acha o arquivo
// toma-o por bom sem lhe conferir octeto, d'onde um arquivo cortado ao meio
// (queda a meio da escripta, disco cheio, duas instancias sobre a mesma capa)
// envenenaria esse endereço de conteudo para SEMPRE, e a capa d'aquelle album
// nunca mais voltaria. O rename no mesmo systema de arquivos é atomico, e
// resolve de graça a corrida entre duas instancias.
std::filesystem::path escreve_no_cache(std::string_view arte, bool* escreveu) {
  const std::filesystem::path onde = caminho_da_capa_em_cache(arte);
  if (onde.empty()) return {};
  std::error_code erro;
  if (std::filesystem::is_regular_file(onde, erro) && !erro) return onde;
  std::filesystem::create_directories(onde.parent_path(), erro);
  if (erro) return {};
  const std::filesystem::path meio =
      onde.string() + "." + std::to_string(::getpid()) + ".parte";
  std::ofstream sahida(meio, std::ios::binary | std::ios::trunc);
  if (!sahida) return {};
  sahida.write(arte.data(), static_cast<std::streamsize>(arte.size()));
  // O `close` ANTES do `good`: o ultimo despejo corre no fecho, e afervel
  // antes d'elle daria por bom o erro que mora justamente na cauda.
  sahida.close();
  if (sahida.good()) std::filesystem::rename(meio, onde, erro);
  if (!sahida.good() || erro) {
    std::filesystem::remove(meio, erro);
    return {};
  }
  *escreveu = true;
  return onde;
}

}  // namespace

const ArquivoDaCapa& Arquivario::de(const std::filesystem::path& faixa) {
  const auto assento = guardados_.find(faixa);
  if (assento != guardados_.end()) return assento->second;
  ArquivoDaCapa achado;
  // A capa AO LADO ganha da embutida, pela ordem declarada da Galeria.
  if (!faixa.empty()) achado.caminho = capa_ao_lado(faixa);
  if (!achado.caminho.empty()) {
    achado.medida = medida_da_imagem(cabeca_do_arquivo(achado.caminho));
  } else if (!faixa.empty()) {
    const std::string arte = arte_embutida(faixa);
    bool escreveu = false;
    achado.caminho = escreve_no_cache(arte, &escreveu);
    if (escreveu) ++escriptos_;
    if (!achado.caminho.empty()) achado.medida = medida_da_imagem(arte);
  }
  // A AUSENCIA guarda-se, pela regra da Galeria: sem ella, faixa sem capa
  // faria a Casa procurar no disco vinte vezes por segundo.
  return guardados_.emplace(faixa, std::move(achado)).first->second;
}

std::size_t Arquivario::quantos_escriptos() const noexcept { return escriptos_; }

namespace {

// aplica_sgr — lê UM escape do chafa e assenta a côr na corrida que vem. Sómente os
// codigos que o chafa emitte: 38;2;R;G;B, 48;2;R;G;B, 39, 49, 0 e 7. Codigo que não se
// conheça ignora-se, e não se lança: o chafa é ferramenta alheia e pode mudar.
void aplica_sgr(std::string_view escape, Corrida* corrida,
                bool* invertida) {
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
      *invertida = false;
    } else if (cifras[i] == 7) {
      // O VIDEO INVERTIDO, e não se ignora mais. O chafa emitte-o em célulla de
      // côr chapada: diz «espaço invertido» com a TINTA posta, em vez de dizer
      // «fundo cheio». Marca-se aqui e troca-se quando a corrida FECHA, que a
      // ordem d'elle é `ESC[7m` ANTES da côr: trocando já, trocar-se-ia nada.
      *invertida = true;
    } else if (cifras[i] == 27) {
      // O FIM do invertido. Desfaz a MARCA, e sómente ella: as côres ficam onde
      // estavam, que o 27 nada diz d'ellas. O chafa 1.19 não o emitte, e é por
      // isso que elle cahia no ramo do desconhecido; tratado, a inversão deixa
      // de depender de o chafa fechar sempre com `ESC[0m`.
      *invertida = false;
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

// assenta — fecha a corrida na lista. Invertida, a troca faz-se n'uma CÓPIA, e
// jamais no `corrente`: aquelle é a côr que ATRAVESSA para a corrida seguinte, e
// trocá-lo alli faria a tinta de uma vazar por fundo da outra. Entrada que o
// expunha: `ESC[7m ESC[38;2;1;2;3m A ESC[38;2;9;9;9m B`, onde o B sahia com o
// (1,2,3) do A no fundo. O chafa 1.19 fecha toda célulla invertida com `ESC[0m`
// e por isso nada sahia errado hoje; mas o analysa_sgr é funcção PUBLICA e pura,
// e o tractado d'este modulo promette tolerar o que o chafa venha a emittir.
void assenta(std::vector<Corrida>* corridas, const Corrida& corrente,
             bool invertida) {
  Corrida fechada = corrente;
  if (invertida) {
    std::swap(fechada.r_frente, fechada.r_fundo);
    std::swap(fechada.g_frente, fechada.g_fundo);
    std::swap(fechada.b_frente, fechada.b_fundo);
  }
  corridas->push_back(std::move(fechada));
}

}  // namespace

std::vector<Corrida> analysa_sgr(std::string_view linha) {
  std::vector<Corrida> corridas;
  Corrida corrente;
  bool invertida = false;
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
      assenta(&corridas, corrente, invertida);
      corrente.texto.clear();
    }
    aplica_sgr(corpo, &corrente, &invertida);
    i = fim < linha.size() ? fim + 1 : linha.size();
  }
  if (!corrente.texto.empty()) assenta(&corridas, corrente, invertida);
  return corridas;
}

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
