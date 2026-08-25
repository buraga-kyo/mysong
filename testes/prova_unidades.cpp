// ══════════════════════════════════════════════════════════════════════════
//   PROVA DAS UNIDADES — testes/prova_unidades.cpp
// ══════════════════════════════════════════════════════════════════════════
// Bateria de machina SURDA: barramento algum se abre, e todos os alvos vão escriptos
// á mão. É a lição que a issue mandou tirar, e é o que faz esta prova correr em
// qualquer logar.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

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

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
