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

std::vector<std::string> argumentos_do_chafa(
    const std::filesystem::path& imagem, std::size_t collunas,
    std::size_t linhas) {
  // As quatro bandeiras que importam, e todas por medição e não por leitura do
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
  return {"chafa",
          "--format=symbols",
          "--symbols=block+half",
          "--size=" + std::to_string(collunas) + "x" + std::to_string(linhas),
          "--animate=off",
          "--relative=off",
          "--polite=on",
          "--colors=full",
          "--",
          imagem.string()};
}

namespace {

// extrahe_embutida — a arte que está DENTRO da etiqueta, gravada n'um temporario
// para que o chafa a possa abrir. O chafa lê arquivo, e não memoria.
//
// Grava-se em `$XDG_RUNTIME_DIR`, e não em `/tmp`: arte de album é dado do
// operador, e o directorio de corrida é privado d'elle por construcção.
std::filesystem::path extrahe_embutida(const std::filesystem::path& faixa) {
  TagLib::MPEG::File arquivo(faixa.c_str());
  if (!arquivo.isValid() || arquivo.ID3v2Tag() == nullptr) return {};
  const auto& quadros =
      arquivo.ID3v2Tag()->frameListMap()["APIC"];
  if (quadros.isEmpty()) return {};
  const auto* arte =
      dynamic_cast<const TagLib::ID3v2::AttachedPictureFrame*>(quadros.front());
  if (arte == nullptr || arte->picture().isEmpty()) return {};

  const char* corrida = std::getenv("XDG_RUNTIME_DIR");
  std::filesystem::path onde =
      corrida != nullptr && corrida[0] != '\0'
          ? std::filesystem::path(corrida)
          : std::filesystem::temp_directory_path();
  onde /= "mysong-capa.bin";
  std::ofstream sahida(onde, std::ios::binary | std::ios::trunc);
  if (!sahida) return {};
  sahida.write(arte->picture().data(), arte->picture().size());
  return sahida.good() ? onde : std::filesystem::path();
}

}  // namespace

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
