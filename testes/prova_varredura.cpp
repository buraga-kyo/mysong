// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA VARREDURA — testes/prova_varredura.cpp
// ══════════════════════════════════════════════════════════════════════════
// Duas metades. A derivação do caminho prova-se em cadeias, sem disco algum; a
// varredura prova-se sobre acervos que esta bateria FABRICA em directorio
// temporario. Caso algum toca `~/Música` nem `~/.local/share/mysong`.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <filesystem>
#include <string>

#include "nucleo/varredura.hpp"

namespace nu = mysong::nucleo;

TEST_CASE("a extensão de audio aceita-se em qualquer caixa") {
  CHECK(nu::extensao_de_audio(".mp3"));
  CHECK(nu::extensao_de_audio(".MP3"));
  CHECK(nu::extensao_de_audio(".FlAc"));
  CHECK(nu::extensao_de_audio(".opus"));
  CHECK_FALSE(nu::extensao_de_audio(".txt"));
  CHECK_FALSE(nu::extensao_de_audio(".png"));
  CHECK_FALSE(nu::extensao_de_audio("mp3"));   // sem o ponto não é extensão
  CHECK_FALSE(nu::extensao_de_audio(""));
}

// A derivação, contra alvo ESCRIPTO Á MÃO. Nenhum caso pergunta á obra o que
// ella derivou para depois conferir que o derivou: os quatro campos vão
// escriptos aqui, letra por letra.
TEST_CASE("a derivação lê artista, album, numero e titulo do caminho") {
  const nu::Faixa cheia =
      nu::deriva_do_caminho("/acervo/Ada Lovelace/Máquina/03 - Tear.mp3",
                            "/acervo");
  CHECK(cheia.artista == "Ada Lovelace");
  CHECK(cheia.album == "Máquina");
  CHECK(cheia.numero == 3);
  CHECK(cheia.titulo == "Tear");
  CHECK(cheia.raiz == "/acervo");
  // Todos os quatro bits: a dedução é o piso, e a etiqueta apaga o que disser.
  CHECK(cheia.deduzido == (nu::kDeduziuArtista | nu::kDeduziuAlbum |
                           nu::kDeduziuTitulo | nu::kDeduziuNumero));
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
