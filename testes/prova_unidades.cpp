// ══════════════════════════════════════════════════════════════════════════
//   PROVA DAS UNIDADES — testes/prova_unidades.cpp
// ══════════════════════════════════════════════════════════════════════════
// Bateria de machina SURDA: barramento algum se abre, e todos os alvos vão escriptos
// á mão. É a lição que a issue mandou tirar, e é o que faz esta prova correr em
// qualquer logar.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cctype>
#include <limits>
#include <string>

#include "api/unidades.hpp"

namespace ap = mysong::api;
namespace nu = mysong::nucleo;

TEST_CASE("a posição vae e volta em microssegundos, e trunca") {
  CHECK(ap::segundos_para_micros(0.0) == 0);
  CHECK(ap::segundos_para_micros(1.0) == 1000000);
  CHECK(ap::segundos_para_micros(2.5) == 2500000);
  CHECK(ap::segundos_para_micros(185.208) == 185208000);
  // TRUNCA, e não arredonda: um microssegundo a mais pediria posição que não ha.
  CHECK(ap::segundos_para_micros(0.0000019) == 1);
  // Negativo e não-numero dão zero, que a fonte é o mpv e o mpv dá os dous.
  CHECK(ap::segundos_para_micros(-1.0) == 0);
  CHECK(ap::segundos_para_micros(std::numeric_limits<double>::quiet_NaN()) == 0);
  CHECK(ap::segundos_para_micros(std::numeric_limits<double>::infinity()) == 0);

  CHECK(ap::micros_para_segundos(0) == doctest::Approx(0.0));
  CHECK(ap::micros_para_segundos(1000000) == doctest::Approx(1.0));
  CHECK(ap::micros_para_segundos(2500000) == doctest::Approx(2.5));
  CHECK(ap::micros_para_segundos(-5) == doctest::Approx(0.0));
}

TEST_CASE("o volume vae e volta, aparado, e ARREDONDA na volta") {
  CHECK(ap::porcento_para_volume(0) == doctest::Approx(0.0));
  CHECK(ap::porcento_para_volume(50) == doctest::Approx(0.5));
  CHECK(ap::porcento_para_volume(100) == doctest::Approx(1.0));
  // Apara-se nas duas pontas: cliente algum recebe volume de um vírgula dous.
  CHECK(ap::porcento_para_volume(150) == doctest::Approx(1.0));
  CHECK(ap::porcento_para_volume(-10) == doctest::Approx(0.0));

  CHECK(ap::volume_para_porcento(0.0) == 0);
  CHECK(ap::volume_para_porcento(0.5) == 50);
  CHECK(ap::volume_para_porcento(1.0) == 100);
  CHECK(ap::volume_para_porcento(1.5) == 100);
  CHECK(ap::volume_para_porcento(-0.5) == 0);
  // ARREDONDA. É a decisão oposta á da posição, e o caso que a prende: o cliente que
  // põe zero vírgula quatrocentos e noventa e nove espera cincoenta, e truncar daria
  // quarenta e nove.
  CHECK(ap::volume_para_porcento(0.499) == 50);
  CHECK(ap::volume_para_porcento(0.494) == 49);
  CHECK(ap::volume_para_porcento(std::numeric_limits<double>::quiet_NaN()) == 0);
}

TEST_CASE("o estado sahe nas tres cadeias que a especificação fixa") {
  CHECK(ap::estado_do_mpris(nu::Estado::Tocando) == "Playing");
  CHECK(ap::estado_do_mpris(nu::Estado::Pausado) == "Paused");
  CHECK(ap::estado_do_mpris(nu::Estado::Parado) == "Stopped");
}

TEST_CASE("o trackid é caminho de objecto, e a fila vazia dá NoTrack") {
  CHECK(ap::caminho_da_faixa(0, true) == "/br/us/braga/mysong/faixa/0");
  CHECK(ap::caminho_da_faixa(17, true) == "/br/us/braga/mysong/faixa/17");
  CHECK(ap::caminho_da_faixa(0, false) ==
        "/org/mpris/MediaPlayer2/TrackList/NoTrack");
  // Todo caminho principia por barra, e traz sómente o que o D-Bus admitte n'um
  // caminho de objecto: letras, digitos, sublinhado e barra.
  for (const bool ha : {true, false}) {
    const std::string caminho = ap::caminho_da_faixa(3, ha);
    REQUIRE_FALSE(caminho.empty());
    CHECK(caminho.front() == '/');
    for (const char letra : caminho)
      CHECK((std::isalnum(static_cast<unsigned char>(letra)) != 0 ||
             letra == '_' || letra == '/'));
  }
}

TEST_CASE("a URL escapa tudo menos a barra e o arco livre") {
  CHECK(ap::url_do_arquivo("/a/b.mp3") == "file:///a/b.mp3");
  // O espaço escapa-se; a barra NÃO, que ella é a estructura do caminho.
  CHECK(ap::url_do_arquivo("/a/Ada Lovelace/x.mp3") ==
        "file:///a/Ada%20Lovelace/x.mp3");
  // Acento em dous grupos, que é UTF-8 por octeto.
  CHECK(ap::url_do_arquivo("/a/Máquina/x.mp3") ==
        "file:///a/M%C3%A1quina/x.mp3");
  // Os que enganam n'um nome de arquivo.
  CHECK(ap::url_do_arquivo("/a/A&B#1+2.mp3") ==
        "file:///a/A%26B%231%2B2.mp3");
  // O arco livre passa intacto.
  CHECK(ap::url_do_arquivo("/a-b/c.d_e~f") == "file:///a-b/c.d_e~f");
}

// A taboa do LoopStatus, nos DOUS sentidos. Vae pelos dous de proposito: taboa
// que se prova n'um sentido só pode estar torta do outro lado sem que a prova
// accuse, e é o outro lado que recebe o que o playerctl escreve.
TEST_CASE("o repetir vae e volta nos tres nomes do MPRIS") {
  CHECK(ap::repeticao_do_mpris(nu::Repeticao::Nenhuma) == "None");
  CHECK(ap::repeticao_do_mpris(nu::Repeticao::Uma) == "Track");
  CHECK(ap::repeticao_do_mpris(nu::Repeticao::Todas) == "Playlist");

  CHECK(ap::repeticao_do_nome("None") == nu::Repeticao::Nenhuma);
  CHECK(ap::repeticao_do_nome("Track") == nu::Repeticao::Uma);
  CHECK(ap::repeticao_do_nome("Playlist") == nu::Repeticao::Todas);

  // Nome que a especificação não tem devolve VAZIO, e nunca «nenhuma»: a
  // differença é a Casa recusar o pedido em vez de desligar o modo por si.
  CHECK_FALSE(ap::repeticao_do_nome("Girar").has_value());
  CHECK_FALSE(ap::repeticao_do_nome("").has_value());
  CHECK_FALSE(ap::repeticao_do_nome("track").has_value());  // a caixa importa
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
