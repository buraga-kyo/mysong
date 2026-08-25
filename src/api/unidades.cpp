// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS UNIDADES — src/api/unidades.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. Arquivo algum d'aqui inclue `dbus/dbus.h`, e é essa a promessa:
// a traducção prova-se em machina surda.
//
// DOMÍNIO ......... escalares dos dous lados.
// CONTRA-DOMÍNIO .. escalares aparados.
// INVARIANTE ...... funcção alguma lança, e nenhuma devolve fóra do arco.
// Q.E.D. .......... `grep dbus` neste arquivo sahe vazio, e é o que faz a promessa
//                   verificavel em vez de dita.
// ══════════════════════════════════════════════════════════════════════════
#include "api/unidades.hpp"

#include <cctype>
#include <cmath>

namespace mysong::api {

std::int64_t segundos_para_micros(double segundos) {
  if (!std::isfinite(segundos) || segundos <= 0.0) return 0;
  // Trunca por `static_cast`, que é o que a linguagem faz de inteiro: microssegundo a
  // mais faria o cliente pedir posição que a faixa já não tem.
  return static_cast<std::int64_t>(segundos * static_cast<double>(kMicrosPorSegundo));
}

double micros_para_segundos(std::int64_t micros) {
  if (micros <= 0) return 0.0;
  return static_cast<double>(micros) / static_cast<double>(kMicrosPorSegundo);
}

double porcento_para_volume(int porcento) {
  if (porcento <= 0) return 0.0;
  if (porcento >= 100) return 1.0;
  return static_cast<double>(porcento) / 100.0;
}

int volume_para_porcento(double volume) {
  if (!std::isfinite(volume) || volume <= 0.0) return 0;
  if (volume >= 1.0) return 100;
  return static_cast<int>(std::lround(volume * 100.0));
}

std::string_view estado_do_mpris(nucleo::Estado estado) {
  switch (estado) {
    case nucleo::Estado::Tocando: return "Playing";
    case nucleo::Estado::Pausado: return "Paused";
    case nucleo::Estado::Parado: break;
  }
  return "Stopped";
}

}  // namespace mysong::api

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
