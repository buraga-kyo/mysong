// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DO ESPECTRO — testes/prova_espectro.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a MATHEMATICA em machina surda: sem PipeWire, sem placa de som, sem
// mpv e sem arquivo em disco. O sinal é synthetico, feito aqui mesmo, e é isso
// que torna a prova EXACTA: um seno de frequencia conhecida ha de acender a
// banda que o contém, e nenhuma outra ha de subir com elle.
//
// DOMÍNIO ......... senos de frequencia conhecida, silencio absoluto, blocos
//                   de tamanho absurdo, taxas que não são a d'esta machina, e
//                   fluxo de um canal.
// CONTRA-DOMÍNIO .. veredicto do doctest, e por elle o status do ctest.
// INVARIANTE ...... nenhum caso d'aqui abre socket, arquivo ou placa de som:
//                   roda igual em machina sem som e sem PipeWire algum, que é
//                   o que faz a bateria ser verde onde o aceite de mão não se
//                   pode nem tentar.
// Q.E.D. .......... a prova pergunta á obra QUAL banda contém a frequencia, em
//                   vez de repetir a conta do espaçamento logarithmico. Prova
//                   que repete a conta que quer aferir não prova nada: os dous
//                   erros iguaes se cancellam, e o caso fica verde com a obra
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cmath>
#include <vector>

#include "nucleo/espectro.hpp"

namespace {

namespace nu = mysong::nucleo;

constexpr float PI = 3.14159265358979323846f;

// Um seno entrelaçado, de amplitude e frequencia conhecidas, com o mesmo valor
// em todos os canaes: assim a mistura para mono devolve o proprio seno, e a
// prova afere a transformada sem a mistura no meio a mascarar o resultado.
std::vector<float> seno(float hertz, float amplitude, float taxa,
                        std::size_t quadros, int canaes = 2) {
  const auto largura = static_cast<std::size_t>(canaes);
  std::vector<float> bloco(quadros * largura, 0.0f);
  for (std::size_t q = 0; q < quadros; ++q) {
    const float angulo = 2.0f * PI * hertz * static_cast<float>(q) / taxa;
    const float valor = amplitude * std::sin(angulo);
    for (std::size_t c = 0; c < largura; ++c) bloco[q * largura + c] = valor;
  }
  return bloco;
}

}  // namespace
